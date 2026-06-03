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

import os

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.fusion_task_model import FusionTaskModel


class FusionTaskViewer:
    """
    class used to get fusion task timeline and summary data
    """

    def __init__(self: any, configs: dict, params: dict = None) -> None:
        self.configs = configs
        self.params = params or {}
        self.result_dir = configs.get('result_dir', self.params.get('result_dir', ''))
        self._model = FusionTaskModel(
            self.result_dir, DBNameConstant.DB_FUSION_TASK, [DBNameConstant.TABLE_FUSION_TASK]
        )

    @staticmethod
    def get_timeline_header() -> list:
        pid = InfoConfReader().get_json_pid_data()
        return [
            ["process_name", pid, InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_FUSION_TASK]
        ]

    @staticmethod
    def get_trace_timeline(data_list: list) -> list:
        """
        get time timeline
        """
        result = []
        pid = InfoConfReader().get_json_pid_data()
        for data in data_list:
            start_time = InfoConfReader().trans_into_local_time(data[4])
            end_time = InfoConfReader().trans_into_local_time(data[5])
            task_dur = float(end_time) - float(start_time)
            task_name = "Fusion {}".format(data[7])
            is_ccu = data[7] == 'CCU'
            args = {
                "STREAM ID": data[0],
                "TASK ID": data[1],
                "ACC ID": data[2],
                "TASK TYPE": data[3],
                "FUSION TASK TYPE": data[7],
            }
            if is_ccu:
                args["MISSION ID"] = data[8]
                if data[9] is not None:
                    args["CCU DIE ID"] = data[9]
            result.append([task_name, pid, data[0], start_time, task_dur, args])
        _trace = TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, result)
        meta = TraceViewManager.metadata_event(FusionTaskViewer.get_timeline_header())
        meta.extend(_trace)
        return meta

    def get_timeline_data(self: any) -> list:
        """
        get fusion task timeline data
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_FUSION_TASK)
        if not os.path.exists(db_path):
            return []
        with self._model as _model:
            data_list = _model.get_timeline_data()
        if not data_list:
            return []
        return self.get_trace_timeline(data_list)
