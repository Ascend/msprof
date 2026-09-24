#!/usr/bin/env python
# coding=utf-8
"""
function: test soc_pmu_parser
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.platform.chip_manager import ChipManager
from msparser.l2_cache.soc_pmu_parser import SocPmuParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.soc_pmu import SocPmuBean
from profiling_bean.struct_info.soc_pmu import SocPmuChip6Bean

NAMESPACE = 'msparser.l2_cache.soc_pmu_parser'
MODEL_NAMESPACE = 'msmodel.l2_cache.soc_pmu_model'
L2_MODEL_NAMESPACE = 'msmodel.l2_cache.l2_cache_parser_model'
BEAN_NAMESPACE = 'profiling_bean.struct_info.soc_pmu'


class TestSocPmuParser(unittest.TestCase):
    sample_config = {
        'npuEvents': 'HA:0x00,0x88,0x89,0x8A,0x74,0x75,0x97;SMMU:0x2,0x8a,0x8b,0x8c,0x8d',
        'result_dir': '/tmp/result',
        'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
        'host_id': '127.0.0.1'
    }
    file_list = {DataTag.SOC_PMU: ['socpmu.data.0.slice_0']}

    def test_check_file_complete(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch('os.path.getsize', return_value=160):
            check = SocPmuParser(self.file_list, self.sample_config)
            self.assertEqual(check._check_file_complete("test"), 160)
        with mock.patch('os.path.getsize', return_value=150):
            check = SocPmuParser(self.file_list, self.sample_config)
            self.assertEqual(check._check_file_complete("test"), 150)

    def test_parse(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        data_bean = SocPmuBean()
        data = (*([0] * 8), 0, 1, 1, 2, 4, 5, 4, 1, 0, 0, 0, 0)
        data_bean.construct_bean(data)
        with mock.patch(NAMESPACE + '.SocPmuParser._check_file_complete', return_value=160), \
                mock.patch('builtins.open', mock.mock_open(read_data="")), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=100), \
                mock.patch('common_func.file_manager.check_dir_writable'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.access', return_value=True), \
                mock.patch(BEAN_NAMESPACE + '.SocPmuBean.decode', return_value=data_bean):
            check = SocPmuParser(self.file_list, self.sample_config)
            check.parse()
        with mock.patch(NAMESPACE + '.SocPmuParser._check_file_complete', return_value=0):
            check = SocPmuParser(self.file_list, self.sample_config)
            check.parse()

    def test_parse_should_success_when_chip_is_v6(self):
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        data_bean = SocPmuChip6Bean()
        data = (*([0] * 6), 1, 1, 0, 5, 1, 2, 3, 4, 5, 6, 7, 8)
        data_bean.construct_bean(data)
        with mock.patch(NAMESPACE + '.SocPmuParser._check_file_complete', return_value=160), \
                mock.patch('builtins.open', mock.mock_open(read_data="")), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=100), \
                mock.patch('common_func.file_manager.check_dir_writable'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.access', return_value=True), \
                mock.patch(BEAN_NAMESPACE + '.SocPmuChip6Bean.decode', return_value=data_bean):
            check = SocPmuParser(self.file_list, self.sample_config)
            check.parse()

    def test_save(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch(L2_MODEL_NAMESPACE + '.L2CacheParserModel.flush'), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuModel.flush'):
            check = SocPmuParser(self.file_list, self.sample_config)
            check._soc_pmu_data = [0]
            check._ha_data = [1]
            check.save()

    def test_ms_run(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        check = SocPmuParser(self.file_list, self.sample_config)
        with mock.patch(NAMESPACE + '.SocPmuParser.parse'), \
                mock.patch(NAMESPACE + '.SocPmuParser.save'):
            check = SocPmuParser(self.file_list, self.sample_config)
            check.ms_run()
