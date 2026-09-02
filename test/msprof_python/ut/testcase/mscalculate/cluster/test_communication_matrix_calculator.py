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
import unittest
from unittest import mock

from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from common_func.ms_constant.str_constant import StrConstant
from mscalculate.cluster.communication_matrix_calculator import CommunicationMatrixCalculator
from mscalculate.cluster.communication_matrix_calculator import MatrixProf
from mscalculate.cluster.slow_link_calculator import SlowLinkProf
from msparser.cluster.meta_parser import HcclAnalysisTool
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'mscalculate.cluster.communication_matrix_calculator'


class TestCommunicationMatrixCalculator(unittest.TestCase):
    def test_calculate(self):
        with mock.patch(NAMESPACE + '.CommunicationMatrixCalculator.sum_by_transport_type'), \
            mock.patch(NAMESPACE + '.CommunicationMatrixCalculator.average_rule', return_value=''), \
            mock.patch(NAMESPACE + '.CommunicationMatrixCalculator.slowest_rule', return_value=''):
            cal = CommunicationMatrixCalculator([], [])
            cal.total_time_dict[StrConstant.HCCS] = 6
            cal.total_time_dict[StrConstant.PCIE] = 3
            cal.total_time_dict[StrConstant.RDMA] = 1
            ret = cal.calculate([])
            sug = MatrixProf.PROF_TIME_RATIO.format(
                cal.total_time_dict[StrConstant.HCCS] / 10,
                cal.total_time_dict[StrConstant.PCIE] / 10,
                cal.total_time_dict[StrConstant.RDMA] / 10
            )
            self.assertEqual(ret[0], sug)

    def test_sum_by_transport_type(self):
        sum_link_dict = {
                CommunicationMatrixInfo.TRANSIT_TIME_MS: 100,
                CommunicationMatrixInfo.LARGE_PACKET_RATIO: 5,
                CommunicationMatrixInfo.BANDWIDTH_GB_S: 100,
                'count': 10
        }
        link_dict = {
            CommunicationMatrixInfo.TRANSPORT_TYPE: StrConstant.HCCS,
            CommunicationMatrixInfo.TRANSIT_TIME_MS: 10,
            CommunicationMatrixInfo.LARGE_PACKET_RATIO: 0.5,
            CommunicationMatrixInfo.BANDWIDTH_GB_S: 10
        }
        slowest_dict = {
            StrConstant.HCCS: {
                CommunicationMatrixInfo.TRANSPORT_TYPE: StrConstant.HCCS,
                CommunicationMatrixInfo.TRANSIT_TIME_MS: 8,
                CommunicationMatrixInfo.LARGE_PACKET_RATIO: 0.8,
                CommunicationMatrixInfo.BANDWIDTH_GB_S: 12
            }
        }
        CommunicationMatrixCalculator.sum_by_transport_type(sum_link_dict, link_dict, slowest_dict, StrConstant.HCCS)
        self.assertEqual(sum_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS], 110)
        self.assertEqual(sum_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO], 5.5)
        self.assertEqual(sum_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S], 110)
        self.assertEqual(sum_link_dict['count'], 11)
        self.assertEqual(slowest_dict[StrConstant.HCCS], link_dict)

    def test_average_rule(self):
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        with mock.patch(NAMESPACE + '.CommunicationMatrixCalculator.matrix_slow_link_rule', return_value=''), \
                mock.patch('msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth',
                           return_value=standard_bandwidth):
            sum_link_dict = {
                CommunicationMatrixInfo.TRANSIT_TIME_MS: 100,
                CommunicationMatrixInfo.LARGE_PACKET_RATIO: 5,
                CommunicationMatrixInfo.BANDWIDTH_GB_S: 100,
                'count': 10
            }
            ret = CommunicationMatrixCalculator([], []).average_rule(sum_link_dict, StrConstant.HCCS)
            sug = list()
            sug.append(MatrixProf.PROF_SUM_TIME.format(sum_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS]))
            sug.append(MatrixProf.PROF_AVERAGE_BANDWIDTH.format(
                10, HcclAnalysisTool.StandardBandWidth.get(ChipModel.CHIP_V2_1_0).get(StrConstant.HCCS, -1)))
            sug.append(MatrixProf.PROF_AVERAGE_PACKET_RATIO.format(0.5))
            sug.append('')
            self.assertEqual(ret, sug)

    def test_slowest_rule(self):
        slowest_dict = {
            CommunicationMatrixInfo.TRANSPORT_TYPE: StrConstant.HCCS,
            CommunicationMatrixInfo.SRC_RANK: '0',
            CommunicationMatrixInfo.DST_RANK: '1',
            CommunicationMatrixInfo.TRANSIT_TIME_MS: 8,
            CommunicationMatrixInfo.TRANSIT_SIZE_MB: 1,
            CommunicationMatrixInfo.LARGE_PACKET_RATIO: 0.8,
            CommunicationMatrixInfo.BANDWIDTH_GB_S: 10,
            CommunicationMatrixInfo.BANDWIDTH_UTILIZATION: 0.7
        }
        sum_link_dict = {
            CommunicationMatrixInfo.BANDWIDTH_GB_S: 150,
            'count': 10
        }
        with mock.patch(NAMESPACE + '.CommunicationMatrixCalculator.matrix_slow_link_rule', return_value=''):
            ret = CommunicationMatrixCalculator([], []).slowest_rule(sum_link_dict, slowest_dict, StrConstant.HCCS)
            sug = [MatrixProf.PROF_SLOWEST_LINK.format('0', '1', 1, 8, 10, 0.7, 0.8), '']
            self.assertEqual(ret, sug)

    def test_slow_link_rule1(self):
        ret = CommunicationMatrixCalculator([], []).matrix_slow_link_rule(0.7, 0.9, StrConstant.HCCS)
        suggestion_header = StrConstant.SUGGESTION + ": "
        self.assertEqual(ret, suggestion_header + SlowLinkProf.PROF_HCCS_ISSUE.format(0.7))

    def test_slow_link_rule2(self):
        suggestion_header = StrConstant.SUGGESTION + ": "
        ret = CommunicationMatrixCalculator([], []).matrix_slow_link_rule(0.7, 0.7, StrConstant.HCCS)
        self.assertEqual(ret, suggestion_header + SlowLinkProf.PROF_SMALL_PACKET.format(StrConstant.HCCS, 0.7, 0.7))

    def test_slow_link_rule3(self):
        suggestion_header = StrConstant.SUGGESTION + ": "
        ret = CommunicationMatrixCalculator([], []).matrix_slow_link_rule(0.9, 0.9, StrConstant.HCCS)
        self.assertEqual(ret, suggestion_header + MatrixProf.PROF_GOOD_STATE.format(StrConstant.HCCS))
