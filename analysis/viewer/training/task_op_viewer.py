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

import logging
import os
from typing import List, Tuple
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_common import format_high_precision_for_csv, float_calculate
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel
from msmodel.task_time.ascend_task_model import AscendTaskModel
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer
from viewer.task_time_viewer import TaskTimeViewer


class TaskOpViewer:
    """
    viewer of training trace data
    """

    INVALID_CONTEXT_ID = 4294967295

    @staticmethod
    def get_task_op_summary(message: dict) -> Tuple[List[str], List, int]:
        """
        @param message
        Rewrite gRPC task op method.
        """
        headers = [
            "kernel_name",
            "kernel_type",
            "stream_id",
            "task_id",
            "task_time(us)",
            "task_start(us)",
            "task_stop(us)",
        ]
        if not message:
            logging.error("get_task_op_summary message empty")
            return headers, [], 0
        data, _ = TaskOpViewer.get_task_data_summary(message)
        if not data:
            return headers, [], 0
        start_ts, _ = InfoConfReader().get_collect_time()
        task_start_index = 5
        task_duration_index = 4
        logging.info("There are %d records before task_time data filtering, timestamp is %s", len(data), start_ts)
        filtered_data = Utils.filter_data_by_start_time_condition(
            data,
            start_ts,
            lambda d: (d[task_start_index], float_calculate([d[task_start_index], d[task_duration_index]])),
        )
        logging.info("There are %d records after task_time data filtering.", len(filtered_data))
        data = TaskOpViewer._add_memcpy_data(message['result_dir'], filtered_data)
        return headers, data, len(data)

    @staticmethod
    def get_task_data_summary(message: dict) -> Tuple[List, int]:
        """
        get task info csv
        """
        with AscendTaskModel(message['result_dir'], [DBNameConstant.TABLE_ASCEND_TASK]) as ascendTaskModel:
            task_infos = TaskOpViewer._reformat_task_info(
                TaskOpViewer._group_task_info(ascendTaskModel.get_ascend_task_data_without_unknown()), message
            )
            return task_infos, len(task_infos)

    @staticmethod
    def _add_memcpy_data(result_dir: str, data: List) -> List:
        memcpy_viewer = MemoryCopyViewer(result_dir)
        memcpy_data = memcpy_viewer.get_memory_copy_non_chip0_summary()
        data.extend(memcpy_data)

        return data

    @staticmethod
    def _group_task_info(task_data: List):
        if not task_data:
            return []
        groups_dict = {}
        for item in task_data:
            key = (item.stream_id, item.task_id, item.batch_id)
            if key not in groups_dict:
                groups_dict[key] = []
            groups_dict[key].append(item)
        return list(groups_dict.values())

    @staticmethod
    def operate_type(group_task_data: list) -> Tuple[bool, bool, bool, bool]:
        """
        :param group_task_data:
        :return:  (regular, mix, ffts, static_graph)
        """
        regular, mix, ffts, static_graph = False, False, False, False
        if len(group_task_data) <= 1:
            regular = True
            return regular, mix, ffts, static_graph
        # ctx > 0 为ffts+
        for item in group_task_data:
            if item.context_id > 0 and item.context_id != TaskOpViewer.INVALID_CONTEXT_ID:
                ffts = True
                return regular, mix, ffts, static_graph
            if item.context_id == 0:
                mix = True
        static_graph = not mix
        return regular, mix, ffts, static_graph

    @staticmethod
    def _get_ge_info_map(message: dict):
        task_info_dict = {}
        if not os.path.exists(PathManager.get_db_path(message['result_dir'], DBNameConstant.DB_GE_INFO)):
            return task_info_dict
        with GeInfoModel(message['result_dir']) as geModel:
            task_info = geModel.get_task_info([message['device_id']])
            task_info_dict = {
                (row.stream_id, row.task_id, row.batch_id, row.context_id): {
                    "op_name": row.op_name,
                    "task_type": row.task_type,
                }
                for row in task_info
            }
        return task_info_dict

    @staticmethod
    def _get_runtime_map(message: dict):
        host_task_dict = {}
        if not os.path.exists(PathManager.get_db_path(message['result_dir'], DBNameConstant.DB_RUNTIME)):
            return host_task_dict
        with RuntimeHostTaskModel(message['result_dir']) as hostTaskModel:
            host_task = hostTaskModel.get_host_tasks(True, 0, 0, message['device_id'])
            for row in host_task:
                stream_id, task_id, batch_id, context_id, kernel_name = row[2], row[3], row[5], row[4], row[7]
                for ctx in list(map(int, context_id.split(","))):
                    host_task_dict[(stream_id, task_id, batch_id, ctx)] = {"kernel_name": kernel_name}
        return host_task_dict

    @staticmethod
    def _reformat_task_info(task_data: List, message: dict) -> List:
        task_info_dict, host_task_dict = TaskOpViewer._get_ge_info_map(message), TaskOpViewer._get_runtime_map(message)
        task_info_result = []
        for task_data_arr in task_data:
            regular, mix, ffts, static_graph = TaskOpViewer.operate_type(task_data_arr)
            if ffts:
                continue
            if mix:
                task_data_arr = [i for i in task_data_arr if i.context_id == 0]
            for item in task_data_arr:
                stream_id, task_id, batch_id, context_id, host_task_type, start_time, duration, device_task_type = (
                    item.stream_id,
                    item.task_id,
                    item.batch_id,
                    item.context_id,
                    item.host_task_type,
                    item.start_time,
                    item.duration,
                    item.device_task_type,
                )
                op_info = task_info_dict.get((stream_id, task_id, batch_id, context_id), {})
                host_task_info = host_task_dict.get((stream_id, task_id, batch_id, context_id), {})
                op_name: str = (
                    host_task_info.get("kernel_name")
                    if host_task_info.get("kernel_name")
                    else op_info.get("op_name", Constant.NA)
                )
                default_task_type = TaskTimeViewer.get_task_type(host_task_type, device_task_type)
                op_info_task_type = op_info.get("task_type")
                task_type = op_info_task_type if op_info_task_type not in (None, Constant.NA) else default_task_type
                task_time: float = round(duration / DBManager.NSTOUS, NumberConstant.ROUND_THREE_DECIMAL)

                task_start = format_high_precision_for_csv(InfoConfReader().trans_into_local_time(start_time))
                task_stop = format_high_precision_for_csv(InfoConfReader().trans_into_local_time(start_time + duration))
                task_info_result.append(
                    (
                        op_name,
                        task_type,
                        stream_id,
                        task_id,
                        task_time,
                        task_start,
                        task_stop,
                    )
                )
        # sort task time data by [task_start]
        task_info_result.sort(key=lambda i: i[5])
        return task_info_result
