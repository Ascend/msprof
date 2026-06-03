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

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.hardware.ub_model import UBViewModel
from viewer.interface.base_viewer import BaseViewer


class UBViewer(BaseViewer, ABC):
    """
    class for get ub data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            "ub": UBViewModel
        }
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    def get_column_trace_data(self, datas: list) -> list:
        column_trace_data = []
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(
                    raw_timestamp=InfoConfReader().get_host_time_by_sampling_timestamp(data.time_stamp),
                    use_us=True)
            port = "Port" + f"{data.port_id:03d}"
            column_trace_data.append(
                ["UB " + port, local_time, self.pid, self.tid, {"bandwidth_rx(MB/s)": data.udma_rx_bind,
                "bandwidth_tx(MB/s)": data.udma_tx_bind}])
        return column_trace_data

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        column_trace_data = self.get_column_trace_data(datas)
        meta_data = [["process_name", self.pid, self.tid, "UB"]]
        result = TraceViewManager.metadata_event(meta_data)
        result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                          column_trace_data))
        return result

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        formatted_data = self.format_ub_summary_data(summary_data)
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_data, len(formatted_data)

    def format_ub_summary_data(self, summary_data: list) -> list:
        return [
            (
                data.port_id,
                format_high_precision_for_csv(
                    InfoConfReader().trans_into_local_time(
                        raw_timestamp=InfoConfReader().get_host_time_by_sampling_timestamp(data.time_stamp),
                        use_us=True)
                ),
                data.udma_rx_bind,
                data.udma_tx_bind
            ) for data in summary_data
        ]
