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
from collections import defaultdict

from common_func.common import print_msg
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from mscalculate.cluster.meta_calculator import MetaCalculator
from mscalculate.cluster.slow_link_calculator import SlowLinkCalculator
from msparser.cluster.meta_parser import HcclAnalysisTool


class MatrixProf:
    PROF_TIME_RATIO = (
        "The time ratio of transiting data in HCCS, PCIE and RDMA are {:.2f}, {:.2f} and {:.2f} respectively. \n"
    )
    PROF_SUM_TIME = "The total transit time is {:.2f} ms. \n"
    PROF_GOOD_STATE = "{} bandwidth is fully utilized."
    PROF_GENERAL_INFO = '{} general information and optimization suggestion: \n'
    PROF_AVERAGE_BANDWIDTH = "The average bandwidth is {:.2f}GB/S, and theoretical bandwidth is {:.2f}GB/S. \n"
    PROF_AVERAGE_PACKET_RATIO = "The average large packet ratio is {:.2f}. \n"
    PROF_SLOWEST_LINK = (
        "The slowest link is from rank.{} to rank.{}, "
        "whose transit size is {:.2f}MB, transit time is {:.2f}ms, bandwidth is {:.2f}GB/S, "
        "bandwidth utilization is {:.2f} and large packet ratio is {:.2f}. \n"
    )


class CommunicationMatrixCalculator(MetaCalculator):
    def __init__(self, data: list, op_name_list: list):
        super().__init__()
        self.data = data
        self.op_name_list = op_name_list
        self.total_time_dict = defaultdict(float)

    @staticmethod
    def matrix_slow_link_rule(utilization_ratio: float, large_packet_ratio: float, trans_type: str):
        suggestion_header = StrConstant.SUGGESTION_HEADER
        suggestion = SlowLinkCalculator.slow_link_rule(utilization_ratio, large_packet_ratio, trans_type)
        if not suggestion:
            suggestion = MatrixProf.PROF_GOOD_STATE.format(trans_type)
        return suggestion_header + suggestion

    @staticmethod
    def sum_by_transport_type(sum_link_dict: dict, link_dict: dict, slowest_dict: dict, trans_type: str) -> None:
        sum_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS] += link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS]
        sum_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] += link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S]
        sum_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO] += link_dict[
            CommunicationMatrixInfo.LARGE_PACKET_RATIO
        ]
        sum_link_dict['count'] += 1
        slowest_dict[trans_type] = min(
            link_dict, slowest_dict[trans_type], key=lambda x: x[CommunicationMatrixInfo.BANDWIDTH_GB_S]
        )

    @staticmethod
    def print_second_level_info(transport_dict: dict, msg_key: str) -> None:
        info_list = ['\t\t' + sug for sug in transport_dict.get(msg_key, [])]
        if not info_list:
            return
        print_msg('\t' + msg_key + '\n')
        for info in info_list:
            print_msg(info)

    def run(self):
        for link_info in self.data:
            self.suggestions.append(self.calculate(link_info))

    def calculate(self: any, link_info: list) -> tuple:
        transport_types = [
            StrConstant.HCCS,
            StrConstant.PCIE,
            StrConstant.RDMA,
            StrConstant.LOCAL,
            StrConstant.SIO,
            StrConstant.UB,
        ]
        sum_link_info = {trans_type: defaultdict(float) for trans_type in transport_types}
        slowest_link = {trans_type: defaultdict(float) for trans_type in transport_types}
        for slowest_dict in slowest_link.values():
            slowest_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] = float('inf')
        # calculate average info and slowest link for different transport type respectively
        for link_dict in link_info:
            trans_type = link_dict.get(CommunicationMatrixInfo.TRANSPORT_TYPE)
            if trans_type not in sum_link_info:
                continue
            self.sum_by_transport_type(sum_link_info[trans_type], link_dict, slowest_link, trans_type)
        # give suggestions
        suggestions = []
        for trans_type in transport_types:
            suggestion_for_trans_type = {}
            sum_link_dict = sum_link_info[trans_type]
            slowest_link_dict = slowest_link[trans_type]
            suggestion_for_trans_type[StrConstant.TRANSPORT_TYPE_INFO] = MatrixProf.PROF_GENERAL_INFO.format(trans_type)
            suggestion_for_trans_type[StrConstant.AVERAGE_INFO] = self.average_rule(sum_link_dict, trans_type)
            suggestion_for_trans_type[StrConstant.SLOWEST_LINK_INFO] = self.slowest_rule(
                sum_link_dict, slowest_link_dict, trans_type
            )
            suggestions.append(suggestion_for_trans_type)
        total_time = sum(self.total_time_dict.values())
        if total_time == 0:
            logging.warning('No link cost any time!')
            return '', []
        # time_ratio 文案只统计 HCCS/PCIE/RDMA 三类,占比分母需只取三者之和,
        # 避免 LOCAL/SIO/UB 的传输耗时使三者占比之和小于 1,给用户造成误导
        hccs_pcie_rdma_total = (
            self.total_time_dict[StrConstant.HCCS]
            + self.total_time_dict[StrConstant.PCIE]
            + self.total_time_dict[StrConstant.RDMA]
        )
        if hccs_pcie_rdma_total == 0:
            # 仅存在 LOCAL/SIO/UB 等其它类型流量时,不输出三类占比
            return '', suggestions
        time_ratio_info = MatrixProf.PROF_TIME_RATIO.format(
            self.total_time_dict[StrConstant.HCCS] / hccs_pcie_rdma_total,
            self.total_time_dict[StrConstant.PCIE] / hccs_pcie_rdma_total,
            self.total_time_dict[StrConstant.RDMA] / hccs_pcie_rdma_total,
        )
        return time_ratio_info, suggestions

    def average_rule(self: any, sum_link_dict: dict, trans_type: str):
        suggestion = []
        if sum_link_dict['count'] == 0:
            return suggestion
        utilization_ratio = (
            sum_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S]
            / sum_link_dict['count']
            / HcclAnalysisTool.get_standard_bandwidth().get(trans_type, -1)
        )
        large_packet_ratio = sum_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO] / sum_link_dict['count']
        self.total_time_dict[trans_type] = sum_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS]
        suggestion.append(MatrixProf.PROF_SUM_TIME.format(sum_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS]))
        suggestion.append(
            MatrixProf.PROF_AVERAGE_BANDWIDTH.format(
                sum_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] / sum_link_dict['count'],
                HcclAnalysisTool.get_standard_bandwidth().get(trans_type, -1),
            )
        )
        suggestion.append(MatrixProf.PROF_AVERAGE_PACKET_RATIO.format(large_packet_ratio))
        suggestion.append(self.matrix_slow_link_rule(utilization_ratio, large_packet_ratio, trans_type))
        return suggestion

    def slowest_rule(self: any, sum_link_dict: dict, slowest_link_dict: dict, trans_type: str):
        suggestion = []
        if sum_link_dict['count'] == 0:
            return suggestion
        if (
            slowest_link_dict[CommunicationMatrixInfo.TRANSIT_SIZE_MB] == 0
            or slowest_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S]
            > (sum_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] / sum_link_dict['count'])
            * NumberConstant.BANDWIDTH_THRESHOLD
        ):
            return suggestion
        suggestion.append(
            MatrixProf.PROF_SLOWEST_LINK.format(
                slowest_link_dict[CommunicationMatrixInfo.SRC_RANK],
                slowest_link_dict[CommunicationMatrixInfo.DST_RANK],
                slowest_link_dict[CommunicationMatrixInfo.TRANSIT_SIZE_MB],
                slowest_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS],
                slowest_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S],
                slowest_link_dict[CommunicationMatrixInfo.BANDWIDTH_UTILIZATION],
                slowest_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO],
            )
        )
        suggestion.append(
            self.matrix_slow_link_rule(
                slowest_link_dict[CommunicationMatrixInfo.BANDWIDTH_UTILIZATION],
                slowest_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO],
                trans_type,
            )
        )
        return suggestion

    def add_suggestions(self: any, op_info: list) -> None:
        """
        add suggestion to dict
        """
        for idx, op_dict in enumerate(op_info):
            op_dict[StrConstant.TIME_RATIO], op_dict[StrConstant.MATRIX_SUGGESTION] = self.suggestions[idx]

    def print_suggestion(self: any, op_info: list) -> None:
        if not op_info or op_info[-1].get(StrConstant.OP_NAME) != StrConstant.TOTAL:
            message = "No Data for Total HCCL Operators in communication Matrix"
            logging.error(message)
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, message)
        total_dict = op_info[-1]
        print_msg(total_dict.get(StrConstant.TIME_RATIO, ''))
        for transport_dict in total_dict.get(StrConstant.MATRIX_SUGGESTION, []):
            print_msg(transport_dict.get(StrConstant.TRANSPORT_TYPE_INFO, ''))
            self.print_second_level_info(transport_dict, StrConstant.AVERAGE_INFO)
            self.print_second_level_info(transport_dict, StrConstant.SLOWEST_LINK_INFO)
