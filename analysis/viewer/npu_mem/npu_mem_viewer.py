# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.npu_mem.npu_mem_model import NpuMemModel
from viewer.get_trace_timeline import TraceViewer


class NpuMemViewer:
    NPU_MEM_TYPE_APP = "0"
    NPU_MEM_TYPE_DEVICE = "1"

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._model = NpuMemModel(self._project_path, DBNameConstant.DB_NPU_MEM, [DBNameConstant.TABLE_NPU_MEM])
        self._npu_mem_events = {self.NPU_MEM_TYPE_APP: "APP", self.NPU_MEM_TYPE_DEVICE: "Device"}

    def get_summary_data(self: any) -> tuple:
        """
        get summary data from npu mem data
        :return: summary data
        """
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Maybe npu mem data parse failed, please check the data parsing log.")
                return MsvpConstant.MSVP_EMPTY_DATA
            summary_data = self._model.get_summary_data()
            if summary_data:
                summary_data = [
                    [
                        self._npu_mem_events.get(datum.event),
                        round(datum.ddr / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                        round(datum.hbm / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                        round(datum.memory / NumberConstant.KILOBYTE, NumberConstant.ROUND_THREE_DECIMAL),
                        format_high_precision_for_csv(InfoConfReader().format_absolute_ns_to_us(datum.timestamp)),
                    ]
                    for datum in summary_data
                ]
                return self._configs.get(StrConstant.CONFIG_HEADERS), summary_data, len(summary_data)
            return MsvpConstant.MSVP_EMPTY_DATA

    def get_timeline_data(self: any) -> any:
        with self._model as _model:
            if not _model.check_db() or not _model.check_table():
                logging.error("Failed to connect %s", DBNameConstant.DB_NPU_MEM)
                return []

            timeline_data = _model.get_timeline_data()
            if not timeline_data:
                logging.error("Unable to get npu mem data.")
                return []
            pid = InfoConfReader().get_json_pid_data()
            tid = InfoConfReader().get_json_tid_data()
            trace_parser = TraceViewer("NPU MEM")
            meta_data = [["process_name", pid, tid, trace_parser.scope]]
            _result = TraceViewManager.metadata_event(meta_data)
            column_trace_data = []
            for datum in timeline_data:
                timestamp = InfoConfReader().format_absolute_ns_to_us(datum.timestamp)
                column_trace_data.append(
                    [
                        '{}/DDR'.format(self._npu_mem_events.get(datum.event)),
                        timestamp,
                        pid,
                        tid,
                        OrderedDict([("KB", datum.ddr / NumberConstant.KILOBYTE)]),
                    ]
                )
                column_trace_data.append(
                    [
                        '{}/HBM'.format(self._npu_mem_events.get(datum.event)),
                        timestamp,
                        pid,
                        tid,
                        OrderedDict([("KB", datum.hbm / NumberConstant.KILOBYTE)]),
                    ]
                )
                column_trace_data.append(
                    [
                        '{}/Memory'.format(self._npu_mem_events.get(datum.event)),
                        timestamp,
                        pid,
                        tid,
                        OrderedDict([("KB", datum.memory / NumberConstant.KILOBYTE)]),
                    ]
                )
            _result += TraceViewManager.column_graph_trace(
                TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, column_trace_data
            )
            if _result:
                return _result
            logging.error("No data is collected.")
            return []
