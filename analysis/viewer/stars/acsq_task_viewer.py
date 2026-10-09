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

import os

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.sqe_type_map import SqeType
from msmodel.stars.acsq_task_model import AcsqTaskModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class AcsqTaskViewer:
    """
    class used to get acsq task timeline and op_summary data
    """

    def __init__(self: any, configs: dict) -> None:
        self.configs = configs
        self._model = AcsqTaskModel(configs.get('result_dir'), DBNameConstant.DB_SOC_LOG, [])

    @staticmethod
    def get_timeline_header() -> list:
        pid = InfoConfReader().get_json_pid_data()
        result = [["process_name", pid, InfoConfReader().get_json_tid_data(), "AcsqTask"]]

        for sqe in SqeType().instance:
            result.append(["thread_name", pid, sqe.value, sqe.name])
            result.append(["thread_sort_index", pid, sqe.value, sqe.value])
        return result

    @staticmethod
    def get_trace_timeline(data_list: list) -> list:
        """
        get time timeline
        :return: timeline_trace data
        """
        result = []
        pid = InfoConfReader().get_json_pid_data()
        for data in data_list:
            start_time = InfoConfReader().time_from_syscnt(data[4], NumberConstant.MICRO_SECOND)
            task_dur = InfoConfReader().time_from_syscnt(data[5], NumberConstant.MICRO_SECOND) - start_time
            task_name = "{} {}".format(str(data[1]), str(data[0]))
            result.append([task_name, pid, SqeType().instance[data[3]].value, start_time, task_dur])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, result)
        result = TraceViewManager.metadata_event(AcsqTaskViewer.get_timeline_header())
        result.extend(_trace)
        return result

    def get_summary_data(self: any, headers: list) -> tuple:
        """
        get acsq task op_summary data,
        :return: headers, data, count of data
        """
        db_path = PathManager.get_db_path(self.configs.get('result_dir'), DBNameConstant.DB_SOC_LOG)
        if not os.path.exists(db_path):
            return headers, [], 0
        with self._model as _model:
            data_list = _model.get_summary_data()
        self._update_kernel_name(data_list)
        res_list = [
            [
                data.op_name,
                data.task_type,
                data.stream_id,
                data.task_id,
                data.task_time,
                "\"" + str(data.start_time) + "\"",
                "\"" + str(data.end_time) + "\"",
            ]
            for data in data_list
        ]
        return headers, res_list, len(res_list)

    def _update_kernel_name(self: any, data_list: list):
        conn, cur = DBManager.check_connect_db(self.configs.get('result_dir'), DBNameConstant.DB_AICORE_OP_SUMMARY)
        if not (cur and conn and DBManager.judge_table_exist(cur, DBNameConstant.TABLE_SUMMARY_GE)):
            return
        ge_sql = "select op_name, stream_id, task_id from {}".format(DBNameConstant.TABLE_SUMMARY_GE)
        ge_data = DBManager.fetch_all_data(cur, ge_sql, dto_class=GeTaskDto)
        op_name_dict = {"{0}-{1}".format(data.stream_id, data.task_id): data.op_name for data in ge_data}
        for data in data_list:
            key = "{0}-{1}".format(data.stream_id, data.task_id)
            data.op_name = op_name_dict.get(key, 'N/A')
