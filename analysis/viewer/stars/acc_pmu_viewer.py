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

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.acc_pmu_model import AccPmuModel
from viewer.interface.base_viewer import BaseViewer


class AccPmuViewer(BaseViewer, ABC):
    """
    class for get acc_pmu data
    """

    DATA_TYPE = 'Acc PMU'
    SAMPLE_BASED = 'sample_based'

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()
        self.model_list = {
            'acc_pmu': AccPmuModel,
        }

    def get_timeline_header(self: any) -> list:
        """
        to get chrome trace json header
        """
        acc_header = [["process_name", self.pid, self.tid, self.DATA_TYPE]]
        return acc_header

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        to format data to chrome trace json
        """
        if not datas:
            return []
        result = []

        # datas ordered by timestamp
        headers_list = ["read_bandwidth", "write_bandwidth", "read_ost", "write_ost"]
        result_list = [0, 0, 0, 0]
        prev_time = 0.0
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data.timestamp, use_us=True)
            data_list = [data.read_bandwidth, data.write_bandwidth, data.read_ost, data.write_ost]
            if not prev_time:
                result_list = data_list
                prev_time = local_time
                continue
            if prev_time == local_time:
                result_list = [max(value, data_list[i]) for i, value in enumerate(result_list)]
            else:
                for i, value in enumerate(result_list):
                    result.append([headers_list[i], prev_time, self.pid, self.tid, {"value": value}])
                result_list = [max(0, data_list[i]) for i, value in enumerate(result_list)]
                prev_time = local_time
        # inject last data to result
        for i, value in enumerate(result_list):
            result.append([headers_list[i], prev_time, self.pid, self.tid, {"value": value}])

        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event(self.get_timeline_header())
        result.extend(_trace)
        return result
