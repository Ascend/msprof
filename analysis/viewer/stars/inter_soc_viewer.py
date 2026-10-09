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
from msmodel.stars.inter_soc_model import InterSocModel
from viewer.interface.base_viewer import BaseViewer


class InterSocViewer(BaseViewer, ABC):
    """
    class for get inter soc transmission data
    """

    DATA_TYPE = 'data_type'
    TIME_STAMP = 2
    BUFFER_BW_LEVEL = 0
    MATA_BW_LEVEL = 1

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = 0
        self.model_list = {
            'inter_soc_time': InterSocModel,
            'inter_soc_transmission': InterSocModel,
        }

    def get_timeline_header(self: any) -> list:
        """
        get timeline trace header
        :return: list
        """
        soc_header = [["process_name", self.pid, InfoConfReader().get_json_tid_data(), self.params.get(self.DATA_TYPE)]]
        return soc_header

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not datas:
            return []
        result = []
        for data in datas:
            result.append(["Buffer BW Level", data[self.TIME_STAMP], {'Value': data[self.BUFFER_BW_LEVEL]}])
            result.append(["Mata BW Level", data[self.TIME_STAMP], {'Value': data[self.MATA_BW_LEVEL]}])
        for data in result:
            data[2:2] = [self.pid, InfoConfReader().get_json_tid_data()]
        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event(self.get_timeline_header())
        result.extend(_trace)
        self.pid += 1
        return result
