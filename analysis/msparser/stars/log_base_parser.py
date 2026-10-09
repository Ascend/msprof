# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

# pylint: disable=duplicate-code

import logging
from collections import deque
from typing import Tuple

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.constant import Constant
from mscalculate.ascend_task.host_task_collector import HostTaskCollector
from msmodel.sqe_type_map import SqeType
from msmodel.add_info.kfc_info_model import KfcInfoViewModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.block_log_bean import BlockLogBean


class LogBaseParser(IStarsParser):
    """
    class used to parser task and block log
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__()
        self._result_dir = result_dir
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = None
        self._end_functype = None

    def preprocess_data(self: any) -> None:
        """
        preprocess data list
        :return: NA
        """
        self._set_stream_id_by_host()
        self._data_list, self._mismatch_task = self.get_task_time()

    def get_task_time(self: any) -> Tuple[list, list]:
        """
        Categorize data_list into start log and end log, and calculate the task time
        :return: result data list
        """
        task_map = {}
        self._data_list.sort(key=lambda x: x.sys_cnt)
        for data in self._data_list:
            task_key = "{0},{1}".format(str(data.task_id), str(data.stream_id))
            task_map.setdefault(task_key, {}).setdefault(data.func_type, deque([])).append(data)

        matched_result = []
        remaining_data = []
        mismatch_count = 0
        for data_key, data_dict in task_map.items():
            start_que = data_dict.get(self._start_functype, deque())
            end_que = data_dict.get(self._end_functype, deque())
            group_mismatch = 0

            # Greedy + lookahead: match end e with start s only when
            #   e.timestamp >= s.timestamp  AND
            #   (s is the last start  OR  e.timestamp < next_start.timestamp)
            # If e falls after next_start, s is an orphan (its end was lost).
            while start_que and end_que:
                start_task = start_que[0]
                end_task = end_que[0]
                if end_task.sys_cnt < start_task.sys_cnt:
                    _ = end_que.popleft()
                    group_mismatch += 1
                    continue
                # end >= start
                last_start = len(start_que) == 1
                if last_start or end_task.sys_cnt < start_que[1].sys_cnt:
                    # end belongs to current start
                    start_que.popleft()
                    end_que.popleft()
                    if isinstance(start_task, BlockLogBean):
                        block_time = InfoConfReader().time_from_syscnt(
                            end_task.sys_cnt
                        ) - InfoConfReader().time_from_syscnt(start_task.sys_cnt)
                        res = [
                            start_task.stream_id,
                            start_task.task_id,
                            start_task.block_id,
                            SqeType().instance(start_task.task_type).name,
                            start_task.sys_cnt,
                            end_task.sys_cnt,
                            block_time,
                            start_task.core_type,
                            start_task.core_id,
                        ]
                    else:
                        res = [
                            start_task.stream_id,
                            start_task.task_id,
                            start_task.acc_id,
                            SqeType().instance(start_task.task_type).name,
                            start_task.sys_cnt,
                            end_task.sys_cnt,
                            end_task.sys_cnt - start_task.sys_cnt,
                        ]
                    matched_result.append(res)
                else:
                    # end >= next_start: current start's end is lost
                    _ = start_que.popleft()
                    group_mismatch += 1

            group_mismatch += max(0, len(start_que) - 1)
            if start_que:
                remaining_data.append(start_que.pop())

            if group_mismatch:
                logging.debug("Mismatch in %s, count: %d", data_key, group_mismatch)
            mismatch_count += group_mismatch

        if mismatch_count > 0:
            logging.error("There are %d tasks or block mismatching.", mismatch_count)

        return sorted(matched_result, key=lambda data: data[4]), remaining_data

    def flush(self: any) -> None:
        """
        flush all buffer data to db
        :return: NA
        """
        if not self._data_list:
            return
        if self._model.init():
            self.preprocess_data()
            self._model.flush(self._data_list)
            self._model.finalize()
            self._data_list.clear()
            self._data_list.extend(self._mismatch_task)
            self._mismatch_task.clear()

    def _set_stream_id_by_host(self):
        if not ChipManager().is_chip_v6():
            return
        device_id = InfoConfReader().get_device_id()
        host_task_dict = HostTaskCollector(self._result_dir).get_host_task_stream_table(int(device_id))
        with KfcInfoViewModel(self._result_dir, []) as model:
            hccl_data_dict = model.get_kfc_info_dict()
        failed_count = 0
        for data in self._data_list:
            if data.task_id not in host_task_dict and data.task_id not in hccl_data_dict:
                failed_count += 1
                data.stream_id = Constant.UINT16_MAX
                continue
            data.stream_id = host_task_dict.get(data.task_id, hccl_data_dict.get(data.task_id))
        if failed_count > 0:
            logging.warning("%d task items not found in host task", failed_count)
