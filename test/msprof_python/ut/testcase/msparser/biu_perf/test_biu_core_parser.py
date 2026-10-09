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
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.biu_perf.biu_core_parser import BiuCoreParser
from msparser.biu_perf.biu_core_parser import BiuCubeParser
from msparser.biu_perf.biu_perf_parser import BiuPerfParser
from profiling_bean.biu_perf.core_info_bean import CoreInfo
from profiling_bean.biu_perf.flow_bean import FlowBean
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.biu_perf.biu_perf_parser'


class TestBiuPerfParser(unittest.TestCase):
    file_list = {DataTag.BIU_PERF: ["biu.group_0_aic.0.slice_0", "biu.group_0_aiv0.0.slice_0"]}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.BiuPerfParser.parse'), \
                mock.patch(NAMESPACE + '.BiuPerfParser.save'):
            BiuPerfParser(self.file_list, CONFIG).ms_run()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.BiuCubeParser.get_monitor_data', return_value=([1], [2])), \
                mock.patch(NAMESPACE + '.BiuVectorParser.get_monitor_data', return_value=[2]):
            BiuPerfParser(self.file_list, CONFIG).parse()

    def test_add_cycles_data_0(self: any) -> None:
        test_object = mock.Mock()
        test_object.core_info = mock.Mock()
        test_object.core_info.core_type = CoreInfo.AI_CUBE
        BiuCoreParser.add_cycles_data(test_object, mock.Mock())

    def test_add_cycles_data_1(self: any) -> None:
        test_object = mock.Mock()
        test_object.core_info = mock.Mock()
        test_object.core_info.core_type = CoreInfo.AI_VECTOR0
        BiuCoreParser.add_cycles_data(test_object, mock.Mock())

    def test_calculate_delta_cycles(self: any) -> None:
        test_object = mock.Mock()
        test_object.FLOW_MAX_CYCLES = BiuCoreParser.FLOW_MAX_CYCLES
        res = BiuCoreParser.calculate_delta_cycles(test_object, 0, 4095)
        self.assertEqual(res, 1)

    def test_constant(self: any) -> None:
        res = BiuCoreParser.CUBE_CYCLES_INDEX_RANGE
        self.assertEqual(res, [[40, 48], [56, 64], [72, 80], [88, 96]])

        res = BiuCoreParser.CUBE_FLOW_INDEX_RANGE
        self.assertEqual(res, [[32, 40], [48, 56], [64, 72], [80, 88]])

    def test_split_monitor_data(self: any) -> None:
        test_object = mock.Mock()
        test_object.CUBE_SIZE = BiuCoreParser.CUBE_SIZE
        test_object.CUBE_CYCLES_INDEX_RANGE = BiuCoreParser.CUBE_CYCLES_INDEX_RANGE
        test_object.CUBE_FLOW_INDEX_RANGE = BiuCoreParser.CUBE_FLOW_INDEX_RANGE

        BiuCubeParser.split_monitor_data(test_object, b'\x00' * BiuCoreParser.CUBE_SIZE)

    def test_bean(self: any) -> None:
        flow_bean = FlowBean([0] * 10)
        res = [flow_bean.stat_rcmd_num, \
                flow_bean.stat_wcmd_num, \
                flow_bean.stat_rlat_raw, \
                flow_bean.stat_wlat_raw, \
                flow_bean.stat_flux_rd, \
                flow_bean.stat_flux_wr, \
                flow_bean.stat_flux_rd_l2, \
                flow_bean.stat_flux_wr_l2, \
                flow_bean.timestamp, \
                flow_bean.l2_cache_hit]


if __name__ == '__main__':
    unittest.main()
