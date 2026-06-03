# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging
from collections import deque
from typing import Tuple

from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.sqe_type_map import SqeType
from msmodel.stars.fusion_task_model import FusionTaskModel
from msparser.stars.log_base_parser import LogBaseParser
from profiling_bean.stars.fusion_task_bean import FusionTaskBean


class FusionTaskParser(LogBaseParser):
    """
    class to parse fusion task log type data
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__(result_dir)
        self._model = FusionTaskModel(result_dir, db, table_list)
        self._decoder = FusionTaskBean
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = StarsConstant.FUSION_TASK_START_FUNCTYPE
        self._end_functype = StarsConstant.FUSION_TASK_END_FUNCTYPE

    def preprocess_data(self: any) -> None:
        """
        preprocess data list, skip _set_stream_id_by_host for fusion task
        """
        self._data_list, self._mismatch_task = self.get_task_time()

    def handle(self: any, _: any, data: bytes) -> None:
        """
        decode and buffer, validate magic number before decoding
        """
        if len(self._data_list) >= self.MAX_DATA_LEN:
            self.flush()
        bean = self._decoder.decode(data)
        if bean.magic != FusionTaskBean.MAGIC_NUM:
            logging.warning("Fusion task magic mismatch, expected 0x%X, got 0x%X", FusionTaskBean.MAGIC_NUM, bean.magic)
            return
        self._data_list.append(bean)

    def get_task_time(self: any) -> Tuple[list, list]:
        """
        Categorize data_list into start log and end log, and calculate the task time.
        Override to include fusion_task_type in the result.
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
            start_que = data_dict.get(self._start_functype, [])
            end_que = data_dict.get(self._end_functype, [])
            while start_que and end_que:
                start_task = start_que[0]
                end_task = end_que[0]
                if start_task.sys_cnt > end_task.sys_cnt:
                    mismatch_count += 1
                    _ = end_que.popleft()
                    continue
                start_task = start_que.popleft()
                end_task = end_que.popleft()
                res = [
                    start_task.stream_id,
                    start_task.task_id,
                    start_task.acc_id,
                    SqeType().instance(start_task.task_type).name,
                    start_task.sys_cnt,
                    end_task.sys_cnt,
                    end_task.sys_cnt - start_task.sys_cnt,
                    start_task.fusion_task_type,
                ]
                matched_result.append(res)
            if len(start_que) > 1 or end_que:
                logging.debug(
                    "Fusion task mismatch happen in %s, start_que size: %d, end_que size: %d",
                    data_key,
                    len(start_que),
                    len(end_que),
                )
                mismatch_count += len(start_que)
                mismatch_count += len(end_que)
                continue
            while start_que:
                start_task = start_que.popleft()
                remaining_data.append(start_task)
        if mismatch_count > 0:
            logging.error("There are %d fusion tasks mismatching.", mismatch_count)

        return sorted(matched_result, key=lambda data: data[4]), remaining_data
