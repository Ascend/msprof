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
import logging

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_manager import TraceViewManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from msmodel.hardware.qos_model import QosViewModel
from viewer.interface.base_viewer import BaseViewer


class QosViewer(BaseViewer, ABC):
    """
    class for get qos data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {"qos": QosViewModel}
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    def get_column_trace_data(self, datas: list, key_list: list) -> list:
        column_trace_data = []
        timestamp_index = 0
        bandwidth_index = 2
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[timestamp_index])
            for key, value in zip(key_list, data[bandwidth_index:]):
                column_trace_data.append([key, local_time, self.pid, self.tid, {"bandwidth(MB/s)": value}])
        return column_trace_data

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        qos_events = InfoConfReader().get_qos_events()
        if not qos_events:
            logging.error("qosEvents is not invalid in sample.json.")
            return []
        key_list = ["QoS {}".format(i) for i in qos_events.split(",")]
        column_trace_data = self.get_column_trace_data(datas, key_list)
        meta_data = [["process_name", self.pid, self.tid, "QoS"]]
        result = TraceViewManager.metadata_event(meta_data)
        result.extend(
            TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, column_trace_data)
        )
        return result


class StarsQosViewer(QosViewer):
    DIE_ID_KEY = {0: "die 0", 1: "die 1"}

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {"qos": QosViewModel}

    def get_column_trace_data(self, datas: list, key_list: list) -> list:
        timestamp_index = 0
        die_id_index = 1
        bandwidth_index = 4
        args_index = 4
        event_dict = {}
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[timestamp_index])
            die_id = data[die_id_index]
            for key, value in zip(key_list, data[bandwidth_index:]):
                tmp_key = "{}_{}".format(key, local_time)
                if tmp_key not in event_dict:
                    event_dict[tmp_key] = [key, local_time, self.pid, self.tid, {self.DIE_ID_KEY.get(die_id): value}]
                else:
                    event_dict.get(tmp_key)[args_index][self.DIE_ID_KEY.get(die_id)] = value
        column_trace_data = list(event_dict.values())
        return column_trace_data
