#!/usr/bin/env python
# coding=utf-8
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022-2024. All rights reserved.
"""
import unittest
from unittest import TestCase
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.stars.ffts_pmu_calculator import FftsPmuCalculator
from msmodel.aic.aic_pmu_model import FftsV1PmuModel
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_block_pmu import FftsBlockPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from profiling_bean.stars.pmu_bean_v6 import PmuBeanV6

NAMESPACE = 'mscalculate.stars.ffts_pmu_calculator'


class TestFftsPmuCalculator(TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def setUp(self):
        ProfilingScene().init('')

    def test_ms_run_sample_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "sample-based",
                                      "aiv_profiling_mode": "sample-based"}):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_task_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "task-based"}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.calculate'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.parse'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.init_params'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.save'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_calculate_by_iter_no_table(self):
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=False):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.calculate()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_calculate_by_iter_table_exist_and_save(self):
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=[0, 100]), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('os.path.getsize', return_value=1280), \
                mock.patch(NAMESPACE + '.FileCalculator.prepare_process',
                           return_value=b'\xe8\x01\xd3k\x0b\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
                                        b'\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1ek1\x00\x00\x00\x00'
                                        b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\x97+\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00\x00\x00\x16\x8a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00`\x00\x00\x00\x00\x00\x00\x00\x95Z.\x00\x00\x00\x00\x00'
                                        b'\x976\x00\x00\x00\x00\x00\x00\xb4\n\x00\x00\x00\x00\x00\x00\x05\x19+'
                                        b'\xafH\x03\x00\x00\xe9"+\xafH\x03\x00\x00'), \
                mock.patch(NAMESPACE + ".FftsPmuModel.create_table"), \
                mock.patch(NAMESPACE + '.FftsPmuModel.flush'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_num = {'2-2': [22, 22]}
            check._freq = 1500
            check.parse()
            check.calculate()
            check.save()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_save_should_be_success_when__is_mix_needed_is_true(self):
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=[0, 100]), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + ".FileOpen.file_reader.read"), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=1280), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process',
                           return_value=b'\xe8\x01\xd3k\x0b\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
                                        b'\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1ek1\x00\x00\x00\x00'
                                        b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\x97+\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00\x00\x00\x16\x8a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00`\x00\x00\x00\x00\x00\x00\x00\x95Z.\x00\x00\x00\x00\x00'
                                        b'\x976\x00\x00\x00\x00\x00\x00\xb4\n\x00\x00\x00\x00\x00\x00\x05\x19+'
                                        b'\xafH\x03\x00\x00\xe9"+\xafH\x03\x00\x00'), \
                mock.patch(NAMESPACE + ".FftsPmuModel.create_table"), \
                mock.patch(NAMESPACE + '.FftsPmuModel.flush'):
            ChipManager().chip_id = ChipModel.CHIP_V4_1_0
            InfoConfReader()._info_json = {
                "drvVersion": InfoConfReader().ALL_EXPORT_VERSION,
                "DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}],
            }
            mixCalcute = FftsPmuCalculator(self.file_list, CONFIG)
            mixCalcute._core_num_dict = {'aic': 30, 'aiv': 0}
            mixCalcute._block_num = {'2-2': [22, 22]}
            mixCalcute._freq = 1500
            mixCalcute.parse()
            mixCalcute.calculate()
            mixCalcute._is_mix_needed = True
            mixCalcute.save()
            InfoConfReader()._info_json = {}

    def test__get_pmu_decode_data_when_pmu_ov_is_1_then_return_empty(self):
        data_byte = b'\xa8\x03\xd3k\x03\x00 \x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00' \
                                     b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00g%\x03:\x01\x00\x00\x00' \
                                     b'\x10L\\\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf8G\x00' \
                                     b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x006b\xdc\x13\x00\x00\x00\x00' \
                                     b'\x0b\xf6:\x00\x00\x00\x00\x00A\xdc\x919\x00\x00\x00\x00\x02\x00\x00\x00\x00' \
                                     b'\x00\x00\x00\xa9\xa5\x07\x00\x00\x00\x00\x00:\x04\xd1\xc2\x0b\x00\x00\x007' \
                                     b'\xf8!\xcf\x0b\x00\x00\x00'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("os.path.exists", return_value=True):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._get_pmu_decode_data(data_byte)
            self.assertEqual(check._data_list, {})

    def test_calculate_all_file(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process',
                           return_value=b'\xe8\x01\xd3k\x0b\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00'
                                        b'\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1ek1\x00\x00\x00\x00'
                                        b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\x97+\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00\x00\x00\x16\x8a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                        b'\x00\x00\x00\x00`\x00\x00\x00\x00\x00\x00\x00\x95Z.\x00\x00\x00\x00\x00'
                                        b'\x976\x00\x00\x00\x00\x00\x00\xb4\n\x00\x00\x00\x00\x00\x00\x05\x19+\xafH'
                                        b'\x03\x00\x00\xe9"+\xafH\x03\x00\x00'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_save_no_data(self):
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.save()

    def test_calculate_block_pmu_list(self):
        data_1 = [
            617, 27603, 3, 2, 0, 6, 0, 55, 128, 0, 0, 1, 1, 642379,
            0, 28, 0, 28384, 0, 920, 209, 2584, 121, 2201803260082, 2201803277549
        ]
        data_2 = [
            617, 27603, 3, 2, 0, 6, 0, 55, 128, 0, 0, 1, 1, 642379,
            0, 28, 0, 28384, 0, 920, 209, 2584, 121, 2201803260082, 2201803277549
        ]
        pmu_data_list = [FftsBlockPmuBean(data_1), FftsBlockPmuBean(data_2)]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_pmu_value'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.calculate_block_pmu_list(pmu_data_list[1])
            check.calculate_block_pmu_list(pmu_data_list[0])

    def test_is_block_should_return_true_when_taskBlock_in_sample_json_is_on(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._sample_json = {'taskBlock': 'on'}
            self.assertTrue(check.is_block())

    def test_is_block_should_return_false_when_taskBlock_in_sample_json_is_empty(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._sample_json = {'taskBlock': ''}
            self.assertFalse(check.is_block())

    def test_is_block_should_return_false_when_taskBlock_is_not_in_sample_json(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._sample_json = {}
            self.assertFalse(check.is_block())

    def test_get_group_number_when_is_block_and_mix_block_num_not_equals_0(self):
        key = '3-0-0'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=True):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [20]},
                'mix_block_num': {key: [40]},
                'block_num_group': {key: [[20, 40]]}
            }
            self.assertEqual(check._get_group_number(59, [60]), 0)

    def test_get_group_number_when_is_not_block_and_mix_block_num_equals_0(self):
        key = '3-0-0'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=False):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [20, 20]},
                'mix_block_num': {key: [40, 0]},
                'block_num_group': {key: [[20, 40], [20, 0]]}
            }
            self.assertEqual(check._get_group_number(39, [40, 0]), 0)

    def test_get_group_number_when_is_block_and_mix_block_num_equals_0(self):
        key = '3-0-0'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=True):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [20, 20]},
                'mix_block_num': {key: [40, 0]},
                'block_num_group': {key: [[20, 40], [20, 0]]}
            }
            self.assertEqual(check._get_group_number(79, [60, 20]), 1)

    def test_get_group_number_when_is_not_block(self):
        key = '3-0-0'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=False):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [20]},
                'mix_block_num': {key: [40]},
                'block_num_group': {key: [[20, 40]]}
            }
            self.assertEqual(check._get_group_number(420, [40]), 10)

    def test_add_block_pmu_list(self):
        key = '12-133-0'
        mix_type = 'MIX_AIC'
        block_dict = {
            key: [
                [mix_type, (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13)), 1, 21, 7, 0],
                [mix_type, (349425, (1161, 0, 1121, 0, 7915, 7409, 407, 13)), 1, 22, 7, 0],
                [mix_type, (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13)), 1, 23, 7, 0],
                [mix_type, (349456, (1161, 0, 1088, 0, 7809, 7380, 391, 13)), 0, 24, 7, 0]
            ]
        }
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events',
                           return_value=['vec_ratio', 'mac_ratio', 'scalar_ratio',
                                         'mte1_ratio', 'mte2_ratio', 'mte3_ratio',
                                         'icache_req_ratio', 'icache_miss_rate']), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.group_block_with_iter'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_pmu_value'):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check.block_dict = block_dict
            check._block_num = {
                'block_num': {key: [1]},
                'mix_block_num': {key: [2]},
                'block_num_group': {key: [[1, 2]]}
            }
            check.add_block_pmu_list()

    def test_group_block_with_iter_when_is_not_block(self):
        key = '12-133-0'
        mix_type = 'MIX_AIC'
        value = [
            (mix_type, (3054, (59, 0, 749, 0, 2, 1667, 100, 48)), 1),
            (mix_type, (3472, (59, 0, 773, 0, 2, 1863, 93, 47)), 1),
            (mix_type, (3708, (59, 0, 464, 0, 2, 1862, 97, 51)), 1),
            (mix_type, (3798, (59, 0, 867, 0, 2, 1731, 99, 50)), 1),
            (mix_type, (4305, (59, 0, 1014, 0, 2, 1387, 96, 50)), 1),
            (mix_type, (4673, (59, 0, 585, 0, 2, 997, 96, 46)), 1),
            (mix_type, (4884, (59, 0, 2740, 0, 2, 1745, 110, 50)), 1),
            (mix_type, (5488, (59, 0, 2242, 0, 2, 1059, 94, 47)), 1)
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=False):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [2]},
                'mix_block_num': {key: [4]},
                'block_num_group': {key: [[2, 4]]}
            }
            group_size_list = check.get_group_size_list(key)
            result_value = {
                0: (
                    (mix_type, 3054, (59, 0, 749, 0, 2, 1667, 100, 48), 1),
                    (mix_type, 3472, (59, 0, 773, 0, 2, 1863, 93, 47), 1),
                    (mix_type, 3708, (59, 0, 464, 0, 2, 1862, 97, 51), 1),
                    (mix_type, 3798, (59, 0, 867, 0, 2, 1731, 99, 50), 1)
                ),
                1: (
                    (mix_type, 4305, (59, 0, 1014, 0, 2, 1387, 96, 50), 1),
                    (mix_type, 4673, (59, 0, 585, 0, 2, 997, 96, 46), 1),
                    (mix_type, 4884, (59, 0, 2740, 0, 2, 1745, 110, 50), 1),
                    (mix_type, 5488, (59, 0, 2242, 0, 2, 1059, 94, 47), 1)
                )
            }
            self.assertEqual(check.group_block_with_iter(value, group_size_list), result_value)

    def test_group_block_with_iter_when_is_block(self):
        key = '3-0-0'
        mix_type = 'MIX_AIC'
        value = [
            (mix_type, (324486, (28, 0, 21339, 0, 817, 129, 2684, 118)), 0),
            (mix_type, (324504, (28, 0, 21265, 0, 741, 126, 2679, 124)), 0),
            (mix_type, (324872, (28, 0, 22183, 0, 866, 139, 2672, 117)), 1),
            (mix_type, (324850, (28, 0, 21295, 0, 968, 150, 2668, 117)), 1),
            (mix_type, (325659, (28, 0, 22337, 0, 1004, 134, 2782, 161)), 1),
            (mix_type, (325898, (28, 0, 23192, 0, 824, 138, 2740, 119)), 1)
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=True):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._block_num = {
                'block_num': {key: [2]},
                'mix_block_num': {key: [4]},
                'block_num_group': {key: [[2, 4]]}
            }
            group_size_list = check.get_group_size_list(key)
            result_value = {
                0: (
                    (mix_type, 324486, (28, 0, 21339, 0, 817, 129, 2684, 118), 0),
                    (mix_type, 324504, (28, 0, 21265, 0, 741, 126, 2679, 124), 0),
                    (mix_type, 324872, (28, 0, 22183, 0, 866, 139, 2672, 117), 1),
                    (mix_type, 324850, (28, 0, 21295, 0, 968, 150, 2668, 117), 1),
                    (mix_type, 325659, (28, 0, 22337, 0, 1004, 134, 2782, 161), 1),
                    (mix_type, 325898, (28, 0, 23192, 0, 824, 138, 2740, 119), 1)
                )
            }
            self.assertEqual(check.group_block_with_iter(value, group_size_list), result_value)

    def test_get_current_freq(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._freq = 1850000000
            check.freq_data = []
            self.assertEqual(check._get_current_freq(1500), 1850000000)
            check.freq_data = [[1000, 0], [2000, 0], [4000, 0]]

            self.assertEqual(check._get_current_freq(1500), 1850000000)
            check.freq_data = [[1000, 1600], [2000, 1500], [4000, 800]]

            self.assertEqual(check._get_current_freq(1500), 1600000000)
            self.assertEqual(check._get_current_freq(2200), 1500000000)
            self.assertEqual(check._get_current_freq(4600), 800000000)

    def test_calculate_total_time_when_get_total_cycle_then_return_total_time(self):
        data = [1, 2, 3, 4, 5, 6, 7, 89, 9, 1, 4, 5, 6, 789, 9, 5, 5, 5, 6, 7, 9, 54, 1, 55, 5]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._is_not_mix_main_core', side_effect=[True, False]), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_current_freq', return_value=40), \
                mock.patch(NAMESPACE + '.Utils.cal_total_time', return_value=100):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._core_num_dict = {"aic": 20}
            check._freq = 1800
            check.freq_data = [[1000, 1800], [2000, 800], [4000, 1800]]
            block_num_dict = {False: 20, True: 40}
            self.assertEqual(check.calculate_total_time(FftsPmuBean(data), 200, block_num_dict), 100)

    def test_set_ffts_table_name_list_should_set_table_aic_and_aiv_name_when_mix_needed_is_True(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "ub_read_bw(GB/s)", "ub_write_bw(GB/s)", "l1_read_bw(GB/s)",
            "l1_write_bw(GB/s)", "l2_read_bw(GB/s)", "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
            "main_mem_write_bw(GB/s)"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = True
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, table_name_list[2:])

    def test_set_ffts_table_name_list_should_set_table_aic_name_when_mix_needed_is_False(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "ub_read_bw(GB/s)", "ub_write_bw(GB/s)", "l1_read_bw(GB/s)",
            "l1_write_bw(GB/s)", "l2_read_bw(GB/s)", "l2_write_bw(GB/s)", "main_mem_read_bw(GB/s)",
            "main_mem_write_bw(GB/s)"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = False
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, [])

    def test_set_ffts_table_name_list_should_set_new_aic_name_when_metric_is_pipelineExecuteUtilization(self):
        table_name_list = [
            "total_time(ms)", "total_cycles", "vec_exe_time", "vec_exe_ratio",
            "mac_time", "mac_ratio_extra", "scalar_time", "scalar_ratio",
            "mte1_time", "mte1_ratio_extra", "mte2_time", "mte2_ratio",
            "mte3_time", "mte3_ratio", "fixpipe_time", "fixpipe_ratio"
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_metrics": AiCoreMetricsManager.PMU_PIPE_EXECUT}), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           side_effect=[table_name_list, table_name_list]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._is_mix_needed = False
            check._set_ffts_table_name_list()
            self.assertEqual(check.aic_table_name_list, table_name_list[2:])
            self.assertEqual(check.aiv_table_name_list, [])

    def test_calculate_total_cycle_and_cycle_list_when_not_is_block(self):
        mix_type = 'MIX_AIC'
        value_new_dict = (
            (mix_type, 330021, (28, 0, 20301, 0, 1190, 113, 2546, 149), 1),
            (mix_type, 330228, (28, 0, 21270, 0, 1310, 132, 2497, 104), 1),
            (mix_type, 329840, (28, 0, 23177, 0, 1237, 150, 2618, 105), 1),
            (mix_type, 330393, (28, 0, 20396, 0, 1322, 136, 2546, 153), 1),
            (mix_type, 330656, (28, 0, 22451, 0, 1353, 119, 2497, 104), 1),
            (mix_type, 330646, (28, 0, 22104, 0, 1146, 136, 2510, 105), 1),
            (mix_type, 331017, (28, 0, 21286, 0, 1456, 124, 2495, 105), 1),
            (mix_type, 331151, (28, 0, 20880, 0, 956, 137, 2496, 103), 1),
            (mix_type, 331200, (28, 0, 22202, 0, 1128, 134, 2504, 105), 1),
            (mix_type, 331234, (28, 0, 22545, 0, 1322, 127, 2508, 103), 1)
        )
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=False):
            mix_type_value = mix_type
            check = FftsPmuCalculator(self.file_list, CONFIG)
            total_cycle, cycle_list = check._calculate_total_cycle_and_cycle_list(mix_type_value, value_new_dict)
            self.assertEqual(3306386, total_cycle)
            self.assertEqual([280, 0, 216612, 0, 12420, 1308, 25217, 1136], cycle_list)

    def test_calculate_total_cycle_and_cycle_list_when_is_block_and_mix_type_is_aic(self):
        mix_type = 'MIX_AIC'
        value_new_dict = (
            (mix_type, 333500, (28, 0, 21310, 0, 1279, 118, 2505, 105), 0),
            (mix_type, 334082, (607, 0, 42125, 0, 1245, 4445, 4635, 172), 0),
            (mix_type, 333836, (615, 0, 42215, 0, 1536, 4129, 4645, 174), 0),
            (mix_type, 334162, (28, 0, 22234, 0, 894, 133, 2438, 106), 0),
            (mix_type, 334771, (607, 0, 37655, 0, 1192, 4208, 4710, 183), 0),
            (mix_type, 334472, (607, 0, 41371, 0, 1768, 4158, 4653, 175), 0),
            (mix_type, 334531, (28, 0, 23870, 0, 916, 110, 2436, 104), 0),
            (mix_type, 334822, (607, 0, 42507, 0, 1356, 4381, 4649, 179), 0),
            (mix_type, 334674, (28, 0, 23805, 0, 1250, 2043, 2507, 106), 1),
            (mix_type, 334884, (607, 0, 38959, 0, 1671, 4679, 4645, 178), 1),
            (mix_type, 335333, (607, 0, 38650, 0, 1407, 4035, 4665, 196), 1),
            (mix_type, 335092, (607, 0, 41318, 0, 1773, 4261, 4641, 171), 1),
            (mix_type, 335197, (607, 0, 41360, 0, 1292, 4321, 4646, 175), 1),
            (mix_type, 335418, (607, 0, 38754, 0, 1624, 4379, 4651, 179), 1),
            (mix_type, 335319, (28, 0, 23401, 0, 985, 130, 2439, 106), 1),
            (mix_type, 335632, (28, 0, 22379, 0, 1128, 142, 2510, 105), 1),
            (mix_type, 335985, (607, 0, 39348, 0, 1987, 4441, 4640, 173), 1),
            (mix_type, 335659, (28, 0, 22348, 0, 1066, 148, 2443, 105), 1),
            (mix_type, 335843, (607, 0, 43672, 0, 1742, 6423, 4646, 176), 1),
            (mix_type, 335768, (28, 0, 22357, 0, 1146, 146, 2439, 106), 1),
            (mix_type, 336066, (607, 0, 40218, 0, 1811, 7432, 4645, 177), 1),
            (mix_type, 335818, (28, 0, 23172, 0, 875, 112, 2507, 102), 1),
            (mix_type, 336239, (607, 0, 44852, 0, 1277, 4187, 4640, 177), 1),
            (mix_type, 336609, (607, 0, 46346, 0, 1506, 6712, 4644, 176), 1)
        )
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=True):
            mix_type_value = mix_type
            check = FftsPmuCalculator(self.file_list, CONFIG)
            total_cycle, cycle_list = check._calculate_total_cycle_and_cycle_list(mix_type_value, value_new_dict)
            self.assertEqual(5369536, total_cycle)
            self.assertEqual([6238, 0, 550939, 0, 22540, 53591, 61308, 2408], cycle_list)

    def test_get_total_cycle_and_cycle_list_when_is_block_and_mix_type_is_aiv(self):
        mix_type = 'MIX_AIV'
        value_new_dict = (
            (mix_type, 335985, (607, 0, 39348, 0, 1987, 4441, 4640, 173), 1),
            (mix_type, 335659, (28, 0, 22348, 0, 1066, 148, 2443, 105), 1),
            (mix_type, 335843, (607, 0, 43672, 0, 1742, 6423, 4646, 176), 1),
            (mix_type, 335768, (28, 0, 22357, 0, 1146, 146, 2439, 106), 1),
            (mix_type, 336066, (607, 0, 40218, 0, 1811, 7432, 4645, 177), 1),
            (mix_type, 335818, (28, 0, 23172, 0, 875, 112, 2507, 102), 1),
            (mix_type, 336239, (607, 0, 44852, 0, 1277, 4187, 4640, 177), 1),
            (mix_type, 336609, (607, 0, 46346, 0, 1506, 6712, 4644, 176), 1),
            (mix_type, 333500, (28, 0, 21310, 0, 1279, 118, 2505, 105), 0),
            (mix_type, 334082, (607, 0, 42125, 0, 1245, 4445, 4635, 172), 0),
            (mix_type, 333836, (615, 0, 42215, 0, 1536, 4129, 4645, 174), 0),
            (mix_type, 334162, (28, 0, 22234, 0, 894, 133, 2438, 106), 0),
            (mix_type, 334771, (607, 0, 37655, 0, 1192, 4208, 4710, 183), 0),
            (mix_type, 334472, (607, 0, 41371, 0, 1768, 4158, 4653, 175), 0),
            (mix_type, 334531, (28, 0, 23870, 0, 916, 110, 2436, 104), 0),
            (mix_type, 334822, (607, 0, 42507, 0, 1356, 4381, 4649, 179), 0),
            (mix_type, 334674, (28, 0, 23805, 0, 1250, 2043, 2507, 106), 0),
            (mix_type, 334884, (607, 0, 38959, 0, 1671, 4679, 4645, 178), 0),
            (mix_type, 335333, (607, 0, 38650, 0, 1407, 4035, 4665, 196), 0),
            (mix_type, 335092, (607, 0, 41318, 0, 1773, 4261, 4641, 171), 0),
            (mix_type, 335197, (607, 0, 41360, 0, 1292, 4321, 4646, 175), 0),
            (mix_type, 335418, (607, 0, 38754, 0, 1624, 4379, 4651, 179), 0),
            (mix_type, 335319, (28, 0, 23401, 0, 985, 130, 2439, 106), 0),
            (mix_type, 335632, (28, 0, 22379, 0, 1128, 142, 2510, 105), 0)
        )
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config'), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator.is_block', return_value=True):
            mix_type_value = mix_type
            check = FftsPmuCalculator(self.file_list, CONFIG)
            total_cycle, cycle_list = check._calculate_total_cycle_and_cycle_list(mix_type_value, value_new_dict)
            self.assertEqual(5355725, total_cycle)
            self.assertEqual([6246, 0, 541913, 0, 21316, 45672, 61375, 2414], cycle_list)

    def test_calculate_mix_pmu_list_should_return_3_data_when_3_data_parsed(self):
        context_task = [
            FftsPmuBean([0, 0, 1, 1, 0, 0, 4294967295, 8192, 0, 0, 1111, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10000, 10100]),
            FftsPmuBean([0, 0, 1, 2, 0, 0, 4294967295, 8192, 0, 0, 1112, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10200, 10300]),
            FftsPmuBean([0, 0, 1, 3, 0, 0, 4294967295, 8192, 0, 0, 1113, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10400, 10500]),
        ]
        pmu_value = {
            "l0a_read_bw(GB/s)": [0],
            "l0a_write_bw(GB/s)": [0],
            "l0b_read_bw(GB/s)": [0],
            "l0b_write_bw(GB/s)": [0],
            "l0c_read_bw(GB/s)": [0],
            "l0c_read_bw_cube(GB/s)": [0],
            "l0c_write_bw(GB/s)": [0],
            "l0c_write_bw_cube(GB/s)": [0],
        }
        pmu_data_list = [None] * len(context_task)
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 1000}]}
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_current_block', return_value=10), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_pmu_value', return_value=pmu_value):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._data_list['context_task'] = context_task
            check._freq = 1000
            check._core_num_dict = {
                "aic": 1,
                "aiv": 1,
            }
            check.aic_table_name_list = list(pmu_value.keys())
            check.aiv_table_name_list = list(pmu_value.keys())
            check.calculate_mix_pmu_list(pmu_data_list)
            self.assertEqual(3, len(pmu_data_list))

    def test_calculate_pmu_list_should_return_3_data_when_3_data_parsed(self):
        context_task = [
            FftsPmuBean([0, 0, 1, 1, 0, 0, 4294967295, 8192, 0, 0, 1111, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10000, 10100]),
            FftsPmuBean([0, 0, 1, 2, 0, 0, 4294967295, 8192, 0, 0, 1112, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10200, 10300]),
            FftsPmuBean([0, 0, 1, 3, 0, 0, 4294967295, 8192, 0, 0, 1113, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10400, 10500]),
        ]
        pmu_value = {
            "l0a_read_bw(GB/s)": [0],
            "l0a_write_bw(GB/s)": [0],
            "l0b_read_bw(GB/s)": [0],
            "l0b_write_bw(GB/s)": [0],
            "l0c_read_bw(GB/s)": [0],
            "l0c_read_bw_cube(GB/s)": [0],
            "l0c_write_bw(GB/s)": [0],
            "l0c_write_bw_cube(GB/s)": [0],
        }
        pmu_data_list = [None] * len(context_task)
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_current_block', return_value=10), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=[0, pmu_value]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._data_list['context_task'] = context_task
            check._freq = 1000
            check._core_num_dict = {
                "aic": 1,
                "aiv": 1,
            }
            check.aic_table_name_list = list(pmu_value.keys())
            check.calculate_pmu_list(pmu_data_list)
            self.assertEqual(3, len(pmu_data_list))

    def test_calculate_pmu_list_non_mix_should_use_ffts_v1_model_and_keep_end_time(self):
        # 7/8/11(V1.1.x) ffts非mix：__update_model_instance应切换到FftsV1PmuModel(AIC表尾带end_time列)，
        # 且行尾保留每算子wall-clock结束时间(此前calculate()中会被[:-1]剥掉，导致unified读取落0)
        context_task = [
            FftsPmuBean([0, 0, 1, 1, 0, 0, 4294967295, 8192, 0, 0, 1111, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 10000, 10100]),
        ]
        pmu_value = {
            "l0a_read_bw(GB/s)": [0],
            "l0a_write_bw(GB/s)": [0],
            "l0b_read_bw(GB/s)": [0],
            "l0b_write_bw(GB/s)": [0],
            "l0c_read_bw(GB/s)": [0],
            "l0c_read_bw_cube(GB/s)": [0],
            "l0c_write_bw(GB/s)": [0],
            "l0c_write_bw_cube(GB/s)": [0],
        }
        pmu_data_list = [None] * len(context_task)
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.FftsPmuCalculator._get_current_block', return_value=10), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=[0, pmu_value]):
            check = FftsPmuCalculator(self.file_list, CONFIG)
            check._data_list['context_task'] = context_task
            check._freq = 1000
            check._core_num_dict = {
                "aic": 1,
                "aiv": 1,
            }
            check.aic_table_name_list = list(pmu_value.keys())
            check.calculate_pmu_list(pmu_data_list)
            self.assertIsInstance(check._model, FftsV1PmuModel)
            # 行尾字段为timestamp(即wall-clock结束时间)，保留以按位落入表尾end_time列
            self.assertEqual(pmu_data_list[0]._fields[-1], 'timestamp')
            self.assertEqual(pmu_data_list[0].timestamp,
                             InfoConfReader().time_from_syscnt(context_task[0].time_list[1]))

if __name__ == '__main__':
    unittest.main()
