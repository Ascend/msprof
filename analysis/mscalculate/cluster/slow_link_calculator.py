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

from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.ms_constant.str_constant import StrConstant
from mscalculate.cluster.meta_calculator import MetaCalculator


class SlowLinkProf:
    PROF_TYPE_BOTTLENECK = "{} communication takes most of the time, and is the dominated bottleneck. \n"
    PROF_GOOD_STATE = "The bandwidth is fully utilized."
    PROF_SMALL_PACKET = (
        "{} bandwidth is inefficient, and the bandwidth utilization is {:.2f}. "
        "Because it transported too many small packets, the big packet ratio is only {:.2f}. \n "
    )
    PROF_HCCS_ISSUE = (
        "HCCS Bandwidth is inefficient, and the bandwidth utilization is {:.2f}. Please check the HCCS config. \n "
    )
    PROF_PCIE_ISSUE = (
        "PCIE Bandwidth between P2P is inefficient, and the bandwidth utilization is {}. "
        "Please check the PCIE bandwidth contention issue. \n "
    )
    PROF_RDMA_ISSUE = (
        "RDMA Bandwidth is inefficient, and the bandwidth utilization is {:.2f}."
        " Please check the switch configuration. \n "
    )


class SlowLinkCalculator(MetaCalculator):
    def __init__(self, data: list, op_rank_list: list):
        super().__init__()
        self.data = data
        self.op_rank_list = op_rank_list

    @staticmethod
    def slow_link_rule(utilization_ratio: float, large_packet_ratio: float, trans_type: str):
        suggestion = ''
        if utilization_ratio < NumberConstant.BANDWIDTH_THRESHOLD:
            if large_packet_ratio < NumberConstant.LARGE_MESSAGE_RATE:
                suggestion += SlowLinkProf.PROF_SMALL_PACKET.format(trans_type, utilization_ratio, large_packet_ratio)
            else:
                if trans_type == StrConstant.RDMA:
                    suggestion += SlowLinkProf.PROF_RDMA_ISSUE.format(utilization_ratio)
                if trans_type == StrConstant.HCCS:
                    suggestion += SlowLinkProf.PROF_HCCS_ISSUE.format(utilization_ratio)
                if trans_type == StrConstant.PCIE:
                    suggestion += SlowLinkProf.PROF_PCIE_ISSUE.format(utilization_ratio)
        return suggestion

    def run(self):
        for com_dict in self.data:
            self.suggestions.append(self.calculate(com_dict))

    def add_suggestions(self: any, op_info: dict) -> None:
        """
        add suggestion to dict
        """
        for idx, item in enumerate(self.op_rank_list):
            hccl_name = item[0]
            rank_id = item[1]
            op_info[hccl_name][rank_id][StrConstant.SLOW_LINK_SUGGESTION] = self.suggestions[idx]

    def calculate(self: any, com_dict: dict) -> str:
        suggestion_bottelnek = ''
        bottle_neck_list = []
        if (
            com_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS] * NumberConstant.DOMINATED_BOTTLENECK_THRESHOLD
            > com_dict[StrConstant.RDMA][OpBandWidthType.TRANSIT_TIME_MS]
        ):
            bottle_neck_list.append(StrConstant.SDMA)
        if (
            com_dict[StrConstant.RDMA][OpBandWidthType.TRANSIT_TIME_MS] * NumberConstant.DOMINATED_BOTTLENECK_THRESHOLD
            > com_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS]
        ):
            bottle_neck_list.append(StrConstant.RDMA)
        for transport_type in bottle_neck_list:
            suggestion_bottelnek += SlowLinkProf.PROF_TYPE_BOTTLENECK.format(transport_type)
        if not bottle_neck_list:
            bottle_neck_list = [StrConstant.RDMA, StrConstant.SDMA]
        if StrConstant.SDMA in bottle_neck_list:
            bottle_neck_list.remove(StrConstant.SDMA)
            bottle_neck_list.append(StrConstant.HCCS)
            bottle_neck_list.append(StrConstant.PCIE)
        suggestion_slow_reason = ''
        for transport_type in bottle_neck_list:
            if com_dict[transport_type][OpBandWidthType.TRANSIT_SIZE_MB] <= 0:
                continue
            utilization_ratio = com_dict[transport_type][OpBandWidthType.BANDWIDTH_UTILIZATION]
            large_packet_ratio = com_dict[transport_type][OpBandWidthType.LARGE_PACKET_RATIO]
            suggestion_slow_reason += self.slow_link_rule(utilization_ratio, large_packet_ratio, transport_type)
        if not suggestion_slow_reason:
            suggestion_slow_reason = SlowLinkProf.PROF_GOOD_STATE
        return suggestion_bottelnek + suggestion_slow_reason
