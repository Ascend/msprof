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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.sio_model import SioModel
from viewer.interface.base_viewer import BaseViewer


class SioViewer(BaseViewer, ABC):
    """
    class for get sio data
    """

    DATA_TYPE = 'SIO'
    ARGS_INDEX = 4
    # 标号为在sio.db中的列索引
    ACC_ID = 0
    REQ_RX = 1
    TIME_STAMP = 9
    BANDWIDTH_TYPE = ["req_rx", "rsp_rx", "snp_rx", "dat_rx", "req_tx", "rsp_tx", "snp_tx", "dat_tx"]
    ACC_ID_KEY = {
        0: "die 0",
        1: "die 1",
    }
    ACC_ID_KEY_V6 = {
        0: "D-DIE0",
        2: "U-DIE0",
        3: "U-DIE1",
    }

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {'sio': SioModel}
        self.pid = InfoConfReader().get_json_pid_data()
        self.tid = InfoConfReader().get_json_tid_data()

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not datas:
            return []
        timestamp_dict = {}
        event_dict = {}
        for data in datas:
            self._compute_bandwidth(
                data, event_dict, timestamp_dict, self.ACC_ID_KEY_V6 if ChipManager().is_chip_v6() else self.ACC_ID_KEY
            )
        result = list(event_dict.values())
        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event([["process_name", self.pid, self.tid, self.DATA_TYPE]])
        result.extend(_trace)
        return result

    def _compute_bandwidth(self, data: list, event_dict: dict, timestamp_dict: dict, die_id_key_map: dict):
        acc_id = data[self.ACC_ID]
        timestamp = data[self.TIME_STAMP]
        if acc_id in timestamp_dict and timestamp_dict[acc_id] != timestamp:
            data_size_list = data[self.REQ_RX : self.TIME_STAMP]
            bandwidth_data = [
                data_size
                / (NumberConstant.BYTES_TO_KB**2)
                / ((timestamp - timestamp_dict[acc_id]) / NumberConstant.NANO_SECOND)
                for data_size in data_size_list
            ]
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[self.TIME_STAMP], use_us=True)
            for key, value in zip(self.BANDWIDTH_TYPE, bandwidth_data):
                tmp_key = "{}_{}".format(key, local_time)
                if tmp_key not in event_dict:
                    event_dict[tmp_key] = [key, local_time, self.pid, self.tid, {die_id_key_map.get(acc_id): value}]
                else:
                    event_dict.get(tmp_key)[self.ARGS_INDEX][die_id_key_map.get(acc_id)] = value
        timestamp_dict[acc_id] = timestamp
