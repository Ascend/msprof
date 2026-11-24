# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
from unittest import TestCase
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.stars.chip6_pmu_calculator import Chip6PmuCalculator
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.stars.chip6_pmu_calculator'


class TestChip6PmuStages(TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def setUp(self):
        self.old_chip = ChipManager().chip_id
        self.old_info = InfoConfReader()._info_json
        ProfilingScene().init('')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)

    def tearDown(self):
        ChipManager().chip_id = self.old_chip
        InfoConfReader()._info_json = self.old_info
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_decode_pmu_bean_should_call_v6_2_0_decode_when_chip_is_v6_2_0(self):
        mock_bin_data = b'\xe8\x01\xd3k' + b'\x00' * 124
        mock_result_bean = mock.Mock()
        InfoConfReader()._info_json = {"platform_version": "16"}
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        v6_2_0_bean_path = 'profiling_bean.stars.pmu_bean_v6_2_0.PmuBeanV6_2_0'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.HwtsIterModel'), \
                mock.patch(NAMESPACE + '.V6PmuModel'), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData'), \
                mock.patch(v6_2_0_bean_path + '.decode', return_value=mock_result_bean) as mock_decode:
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            result = check._decode_pmu_bean(mock_bin_data)
            self.assertEqual(result, mock_result_bean)
            mock_decode.assert_called_once_with(mock_bin_data)

    def test_decode_pmu_bean_should_call_v6_normal_decode_when_chip_is_not_v6_2_0(self):
        mock_file_list = {DataTag.FFTS_PMU: ['pmu_data_0']}
        mock_bin_data = b'\xe8\x01\xd3k' + b'\x01' * 124
        mock_result_bean = mock.Mock()
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "15"}
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        v6_bean_path = 'profiling_bean.stars.pmu_bean_v6.PmuBeanV6'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.HwtsIterModel'), \
                mock.patch(NAMESPACE + '.V6PmuModel'), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData'), \
                mock.patch(v6_bean_path + '.decode', return_value=mock_result_bean) as mock_decode:
            check = Chip6PmuCalculator(mock_file_list, CONFIG)
            result = check._decode_pmu_bean(mock_bin_data)
            self.assertEqual(result, mock_result_bean)
            mock_decode.assert_called_once_with(mock_bin_data)

    def test_get_fake_end_time_should_return_converted_time_when_fake_end_time_exists(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        mock_block_data = mock.Mock()
        mock_block_data.fake_end_time = 2201803202168
        reader_instance = InfoConfReader()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch.object(reader_instance, 'time_from_syscnt', return_value=38140480645103.0):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            result = check.get_fake_end_time(mock_block_data)
            self.assertEqual(38140480645103.0, result)

    def test_get_fake_end_time_should_return_zero_when_fake_end_time_is_none(self):
        mock_block_data = mock.Mock()
        mock_block_data.fake_end_time = None
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            result = check.get_fake_end_time(mock_block_data)
            self.assertEqual(0, result)

    def test_get_fake_end_time_should_return_zero_when_fake_end_time_attribute_not_exists(self):
        class MockBlockData:
            pass
        mock_block_data = MockBlockData()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            result = check.get_fake_end_time(mock_block_data)
            self.assertEqual(0, result)

    def test_zero_fake_end_time_remains_missing_instead_of_becoming_clock_offset(self):
        from types import SimpleNamespace
        with mock.patch.object(InfoConfReader(), 'time_from_syscnt') as convert:
            self.assertEqual(0, Chip6PmuCalculator.get_fake_end_time(SimpleNamespace(fake_end_time=0)))
            convert.assert_not_called()
