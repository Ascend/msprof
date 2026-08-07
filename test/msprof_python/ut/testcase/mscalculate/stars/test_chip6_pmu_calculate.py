#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
import unittest
from unittest import TestCase
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.stars.chip6_pmu_calculator import Chip6PmuCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.pmu_bean_v6 import PmuBeanV6

NAMESPACE = 'mscalculate.stars.chip6_pmu_calculator'


class TestChip6PmuCalculator(TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profile.data.0.slice_0']}

    def setUp(self):
        ProfilingScene().init('')
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)

    def tearDown(self):
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_ms_run_should_be_success_when_mode_is_sample_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "sample-based",
                                      "aiv_profiling_mode": "sample-based"}):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_should_be_success_when_mode_is_task_based(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={"ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "task-based"}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.calculate'), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.parse'), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.init_params'), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.save'):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.ms_run()

    def test_calculate_should_be_success_when_by_iter_no_table(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.STEP_INFO), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.remove"), \
                mock.patch("msmodel.interface.base_model.BaseModel.init"), \
                mock.patch("msmodel.interface.base_model.BaseModel.finalize"), \
                mock.patch("msmodel.interface.base_model.BaseModel.create_table"), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.HwtsIterModel.check_table', return_value=False):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_by_iter_should_be_success_when_table_exist_and_save(self):
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
                mock.patch(NAMESPACE + ".V6PmuModel.create_table"), \
                mock.patch(NAMESPACE + '.V6PmuModel.flush'):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'DeviceInfo': [{'aic_frequency': 1500, 'hwts_frequency': 1000}],
                                           "devices": "0"}
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_num = {'2-2': [22, 22]}
            check._freq = 1500
            check.parse()
            check.calculate()
            check.save()

    def test_save_should_be_success_when_is_mix_needed_is_true(self):
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
                mock.patch(NAMESPACE + ".V6PmuModel.create_table"), \
                mock.patch(NAMESPACE + '.V6PmuModel.flush'):
            InfoConfReader()._info_json = {
                "drvVersion": InfoConfReader().ALL_EXPORT_VERSION,
                "devices": "0",
                "DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}],
            }
            mixCalcute = Chip6PmuCalculator(self.file_list, CONFIG)
            mixCalcute._core_num_dict = {'aic': 30, 'aiv': 0}
            mixCalcute._block_num = {'2-2': [22, 22]}
            mixCalcute._freq = 1500
            mixCalcute.parse()
            mixCalcute.calculate()
            mixCalcute._is_mix_needed = True
            mixCalcute.save()
            InfoConfReader()._info_json = {}

    def test_get_pmu_decode_data_when_pmu_ov_is_1_then_return_empty(self):
        data_byte = b'\xa8\x03\xd3k\x03\x00 \x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00g%\x03:\x01\x00\x00\x00' \
                    b'\x10L\\\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf8G\x00' \
                    b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x006b\xdc\x13\x00\x00\x00\x00' \
                    b'\x0b\xf6:\x00\x00\x00\x00\x00A\xdc\x919\x00\x00\x00\x00\x02\x00\x00\x00\x00' \
                    b'\x00\x00\x00\xa9\xa5\x07\x00\x00\x00\x00\x00:\x04\xd1\xc2\x0b\x00\x00\x007' \
                    b'\xf8!\xcf\x0b\x00\x00\x00'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("os.path.exists", return_value=True):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check._get_pmu_decode_data(data_byte)
            self.assertEqual(check._data_list_dict, {})

    def test_calculate_should_be_success_when_all_file(self):
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
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.get_scene', return_value=Constant.SINGLE_OP), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_save_do_not_execute_when_no_data(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.save()

    def test_calculate_should_get_four_block_and_one_context_when_is_blocked(self):
        context_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 0, 6, 1, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
        ]
        block_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 0, 6, 1, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
            PmuBeanV6([
                105, 27603, 3, 0, 0, 6, 1, 43, 128, 0, 0, 1, 22, 324504, 0, 28, 0, 21265, 0, 741, 126, 2679, 124, 0,
                2201803181719, 2201803202175]),
            PmuBeanV6([
                169, 27603, 3, 0, 0, 6, 1, 45, 1, 0, 0, 0, 23, 324872, 0, 28, 0, 22183, 0, 866, 139, 2672, 117, 0,
                2201803181719, 2201803202197]),
            PmuBeanV6([
                233, 27603, 3, 0, 0, 6, 1, 47, 1, 0, 0, 1, 23, 324850, 0, 28, 0, 21295, 0, 968, 150, 2668, 117, 0,
                2201803181719, 2201803202198]),
        ]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch('profiling_bean.stars.pmu_bean_v6.PmuBeanV6.is_mix_data', return_value=True):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['block_task'] = block_task
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check.get_block_pmu_data()
            pmu_data_list = check.calculate_mix_pmu_list()
            check.save()
            self.assertEqual(1, len(pmu_data_list))
            self.assertEqual(4, len(check.block_pmu_data))

    def test_calculate_should_get_four_block_and_one_context_when_is_not_block(self):
        context_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 0, 6, 1, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
        ]
        block_task = []
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch('profiling_bean.stars.pmu_bean_v6.PmuBeanV6.is_mix_data', return_value=True):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['block_task'] = block_task
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check.get_block_pmu_data()
            pmu_data_list = check.calculate_mix_pmu_list()
            self.assertEqual(1, len(pmu_data_list))
            self.assertEqual(0, len(check.block_pmu_data))

    def test_calculate_v6_aicore_time_when_is_not_block_and_is_mix(self):
        context_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 1111, 6, 15, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
            PmuBeanV6([
                41, 27603, 3, 0, 1111, 6, 14, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
        ]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            pmu_data_list = check.calculate_mix_pmu_list()
            self.assertEqual(1, len(pmu_data_list))
            self.assertAlmostEqual(20.45, pmu_data_list[0].aic_total_time)
            self.assertEqual(1111, pmu_data_list[0].aic_total_cycle)

    def test_calculate_v6_aicore_time_when_is_block_and_is_mix(self):
        context_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 1111, 6, 15, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
            PmuBeanV6([
                41, 27603, 3, 0, 1111, 6, 14, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
        ]
        block_task = [
            PmuBeanV6([
                41, 27603, 3, 0, 0, 6, 1, 37, 128, 0, 0, 0, 21, 324486, 0, 28, 0, 21339, 0, 817, 129, 2684, 118, 0,
                2201803181719, 2201803202168]),
            PmuBeanV6([
                105, 27603, 3, 0, 0, 6, 1, 43, 128, 0, 0, 1, 22, 324504, 0, 28, 0, 21265, 0, 741, 126, 2679, 124, 0,
                2201803181719, 2201803202175]),
            PmuBeanV6([
                169, 27603, 3, 0, 0, 6, 1, 45, 1, 0, 0, 0, 23, 324872, 0, 28, 0, 22183, 0, 866, 139, 2672, 117, 0,
                2201803181719, 2201803202197]),
            PmuBeanV6([
                233, 27603, 3, 0, 0, 6, 1, 47, 1, 0, 0, 1, 23, 324850, 0, 28, 0, 21295, 0, 968, 150, 2668, 117, 0,
                2201803181719, 2201803202198]),
        ]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 1500, 'hwts_frequency': 1000}]}
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None):
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._data_list_dict['block_task'] = block_task
            check._freq = 1000
            pmu_data_list = check.calculate_mix_pmu_list()
            check.get_block_pmu_data()
            self.assertEqual(1, len(pmu_data_list))
            self.assertAlmostEqual(20.45, pmu_data_list[0].aic_total_time)
            self.assertEqual(1111, pmu_data_list[0].aic_total_cycle)
            self.assertEqual(4, len(check.block_pmu_data))

    def test_calculate_mix_aic_slave_use_total_cycle(self):
        """MIX_AIC: 主核AIC用start-end，从核AIV用total_cycle计算，且两者不一样"""
        context_task = [
            # main: AIC(core_type=0), mst=1, mix=1, total_cycle=1000
            PmuBeanV6([0, 0, 3, 0, 1000, 6, 3, 0, 128, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1000000, 1020000]),
            # slave: AIV(core_type=1), mst=0, mix=1, total_cycle=5000
            PmuBeanV6([0, 0, 3, 0, 5000, 6, 2, 0, 129, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1000000, 3000000]),
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=25.0) as mock_dur, \
                mock.patch(NAMESPACE + '.Utils.cal_total_time', return_value=99.99) as mock_cal:
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check._block_num = {'block_num': {}, 'mix_block_num': {'3-3': [4]}, 'block_num_group': {}}
            check._core_num_dict = {'aic': 30, 'aiv': 3}
            pmu_data_list = check.calculate_mix_pmu_list()

            self.assertEqual(1, len(pmu_data_list))
            # 主核AIC: 使用duration_from_syscnt → 25.0
            self.assertAlmostEqual(25.0, pmu_data_list[0].aic_total_time)
            self.assertEqual(1000, pmu_data_list[0].aic_total_cycle)
            # 从核AIV: 使用total_cycle计算 → mocked为99.99, 与duration_from_syscnt结果25.0不同
            self.assertAlmostEqual(99.99, pmu_data_list[0].aiv_total_time)
            self.assertEqual(5000, pmu_data_list[0].aiv_total_cycle)
            # 从核调用cal_total_time(cycle, freq, block_num, core_num)
            mock_cal.assert_called_once_with(5000, 1000, 4, 3)
            # 主核调用duration_from_syscnt(delta)
            mock_dur.assert_called_once_with(20000)

    def test_calculate_mix_aiv_slave_aic_zero(self):
        """MIX_AIV: 主核AIV用start-end，从核AIC数据归零"""
        context_task = [
            # main: AIV(core_type=1), mst=1, mix=1, total_cycle=1000
            PmuBeanV6([0, 0, 4, 0, 1000, 6, 3, 0, 129, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2000000, 2020000]),
            # slave: AIC(core_type=0), mst=0, mix=1, total_cycle=5000
            PmuBeanV6([0, 0, 4, 0, 5000, 6, 2, 0, 128, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2000000, 5000000]),
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=30.0) as mock_dur, \
                mock.patch(NAMESPACE + '.Utils.cal_total_time') as mock_cal:
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check._block_num = {'block_num': {}, 'mix_block_num': {'4-4': [4]}, 'block_num_group': {}}
            check._core_num_dict = {'aic': 30, 'aiv': 3}
            pmu_data_list = check.calculate_mix_pmu_list()

            self.assertEqual(1, len(pmu_data_list))
            # 主核AIV: 使用duration_from_syscnt → 30.0
            self.assertAlmostEqual(30.0, pmu_data_list[0].aiv_total_time)
            self.assertEqual(1000, pmu_data_list[0].aiv_total_cycle)
            # 从核AIC: AIV主核场景下归零
            self.assertEqual(0, pmu_data_list[0].aic_total_time)
            self.assertEqual(5000, pmu_data_list[0].aic_total_cycle)
            # 从核不走total_cycle计算
            mock_cal.assert_not_called()
            mock_dur.assert_called_once_with(20000)

    def test_calculate_no_mix_aic_only(self):
        """非mix: 仅AIC数据，total_time来自start-end，AIV归零"""
        context_task = [
            # AIC(core_type=0), mst=1, mix=0
            PmuBeanV6([0, 0, 5, 0, 800, 6, 1, 0, 128, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1000000, 1015000]),
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=15.0) as mock_dur:
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check._block_num = {'block_num': {'5-5': [1]}, 'mix_block_num': {}, 'block_num_group': {}}
            check._core_num_dict = {'aic': 30, 'aiv': 3}
            pmu_data_list = check.calculate_mix_pmu_list()

            self.assertEqual(1, len(pmu_data_list))
            # AIC: 使用duration_from_syscnt → 15.0
            self.assertAlmostEqual(15.0, pmu_data_list[0].aic_total_time)
            self.assertEqual(800, pmu_data_list[0].aic_total_cycle)
            # 非mix无AIV数据
            self.assertEqual(0, pmu_data_list[0].aiv_total_time)
            self.assertEqual(0, pmu_data_list[0].aiv_total_cycle)
            mock_dur.assert_called_once_with(15000)

    def test_calculate_no_mix_aiv_only(self):
        """非mix: 仅AIV数据，total_time来自start-end，AIC归零"""
        context_task = [
            # AIV(core_type=1), mst=1, mix=0
            PmuBeanV6([0, 0, 6, 0, 600, 6, 1, 0, 129, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2000000, 2030000]),
        ]
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Chip6PmuCalculator.get_config_params', return_value=None), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=30.0) as mock_dur:
            check = Chip6PmuCalculator(self.file_list, CONFIG)
            check.init_params()
            check._data_list_dict['context_task'] = context_task
            check._freq = 1000
            check._block_num = {'block_num': {'6-6': [1]}, 'mix_block_num': {}, 'block_num_group': {}}
            check._core_num_dict = {'aic': 30, 'aiv': 3}
            pmu_data_list = check.calculate_mix_pmu_list()

            self.assertEqual(1, len(pmu_data_list))
            # AIV: 使用duration_from_syscnt → 30.0
            self.assertAlmostEqual(30.0, pmu_data_list[0].aiv_total_time)
            self.assertEqual(600, pmu_data_list[0].aiv_total_cycle)
            # 非mix无AIC数据
            self.assertEqual(0, pmu_data_list[0].aic_total_time)
            self.assertEqual(0, pmu_data_list[0].aic_total_cycle)
            mock_dur.assert_called_once_with(30000)
            self.assertEqual(600, pmu_data_list[0].aiv_total_cycle)


if __name__ == '__main__':
    unittest.main()
