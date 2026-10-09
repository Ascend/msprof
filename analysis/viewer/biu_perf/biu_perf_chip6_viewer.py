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
import itertools
from abc import ABC

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.biu_perf.biu_perf_chip6_model import BiuPerfChip6ViewerModel
from viewer.interface.base_viewer import BaseViewer


class BiuPerfChip6Viewer(BaseViewer, ABC):
    """
    biu perf chip6 viewer
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {"biu_perf": BiuPerfChip6ViewerModel}
        self.result_dir = self.params.get(StrConstant.PARAM_RESULT_DIR)
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    def get_trace_timeline(self: any, data_list: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        column_trace_data = []
        meta_data = [["process_name", self.pid, self.tid, TraceViewHeaderConstant.PROCESS_BIU_PERF]]
        data_list.sort(key=lambda x: (x.group_id, x.core_type))
        grouped_data = itertools.groupby(data_list, key=lambda x: (x.group_id, x.core_type))
        for key, datas in grouped_data:
            thread_name = "Group{}-{}".format(key[0], key[1])
            meta_data.append(["thread_name", self.pid, self.tid, thread_name])
            for data in datas:
                start_time = InfoConfReader().trans_syscnt_into_local_time(data.timestamp)
                duration = InfoConfReader().duration_from_syscnt(data.duration)
                args = {"Core Type": data.core_type, "Block Id": data.block_id}
                if data.checkpoint_info is not None:
                    args["Checkpoint Info"] = data.checkpoint_info
                column_trace_data.append([data.instruction, self.pid, self.tid, start_time, duration, args])
            self.tid += 1
        result = TraceViewManager.metadata_event(meta_data)
        result.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, column_trace_data)
        )
        return result

    def get_timeline_data(self: any) -> list:
        """
        get model list timeline data
        @return:timeline trace data
        """
        with BiuPerfChip6ViewerModel(
            self.result_dir, DBNameConstant.DB_BIU_PERF, [DBNameConstant.TABLE_BIU_INSTR_STATUS]
        ) as model:
            timeline_data = model.get_timeline_data()
        result = self.get_trace_timeline(timeline_data)
        return result
