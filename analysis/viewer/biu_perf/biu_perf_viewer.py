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

from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.biu_perf.biu_perf_model import BiuPerfModel


class BiuPerfViewer:
    """
    biu perf viewer
    """

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._model = BiuPerfModel(self._project_path, [DBNameConstant.TABLE_BIU_FLOW, DBNameConstant.TABLE_BIU_CYCLES])

    def get_timeline(self: any) -> str:
        meta_timeline = self.get_meta_timeline()
        biu_flow_timeline = self.get_biu_flow_timeline()
        biu_cycles_timeline = self.get_biu_cycles_timeline()
        return meta_timeline + biu_flow_timeline + biu_cycles_timeline

    def get_meta_timeline(self: any) -> list:
        meta_timeline = []
        with self._model as _model:
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_flow_process()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_flow_thread()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_cycles_process()))
            meta_timeline.extend(TraceViewManager.metadata_event(self._model.get_biu_cycles_thread()))
        return meta_timeline

    def get_biu_flow_timeline(self: any) -> list:
        with self._model as _model:
            biu_flow_data = self._model.get_biu_flow_data()
        trace_data = [
            [
                biu_flow_datum[0],
                InfoConfReader().trans_into_local_time(biu_flow_datum[1], use_us=True),
                biu_flow_datum[2],
                biu_flow_datum[3],
                OrderedDict([("flow", biu_flow_datum[4])]),
            ]
            for biu_flow_datum in biu_flow_data
        ]
        return TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)

    def get_biu_cycles_timeline(self: any) -> list:
        with self._model as _model:
            biu_cycles_data = self._model.get_biu_cycles_data()
        trace_data = [
            [
                "",
                biu_cycles_datum[0],
                biu_cycles_datum[1],
                InfoConfReader().trans_into_local_time(biu_cycles_datum[2], use_us=True),
                biu_cycles_datum[3],
                OrderedDict([("cycle_num", biu_cycles_datum[4]), ("ratio", biu_cycles_datum[5])]),
            ]
            for biu_cycles_datum in biu_cycles_data
        ]
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data)
