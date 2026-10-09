#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.constant import CONFIG
from mscalculate.aic.aic_calculator import AicCalculator
from mscalculate.aic.aic_calculator import V5AicCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean

NAMESPACE = 'mscalculate.aic.aic_calculator'


class TestAicCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_parse_by_iter(self):
        with mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_db', return_value=True), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.check_table', return_value=True):
            with mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(127, 1280)), \
                    mock.patch(NAMESPACE + '.AicCalculator._parse'), \
                    mock.patch('framework.offset_calculator.FileCalculator.prepare_process'), \
                    mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
                check = AicCalculator(self.file_list, CONFIG)
                check._parse_by_iter()
            with mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(128, 0)), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = AicCalculator(self.file_list, CONFIG)
                check._parse_by_iter()

    def test_parse_all_file(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.AicCalculator._parse'), \
                mock.patch('builtins.open', mock.mock_open(read_data=aic_reader)), \
                mock.patch('os.path.getsize', return_value=len(aic_reader)):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_all_file()

    def test_calculate(self):
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        with mock.patch(NAMESPACE + '.AicCalculator._parse_by_iter'), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        ProfilingScene().init('test')
        with mock.patch(NAMESPACE + '.AicCalculator._parse_all_file'), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='train'):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_all_file = mock.Mock()
            check.calculate()
            check._parse_all_file.assert_not_called()

    def test_calculate_when_not_all_export_then_parse_by_iter(self):
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}):
            check = AicCalculator(self.file_list, CONFIG)
            check._parse_by_iter = mock.Mock(return_value=None)
            check.calculate()
            check._parse_by_iter.assert_called_once()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_parse(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        with mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_events', return_value=['']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate_pmu_list'):
            check = AicCalculator(self.file_list, CONFIG)
            check._core_num_dict = {'aic': 30, 'aiv': 0}
            check._block_num = {'2-2': [22, 22]}
            check._freq = 1500
            check._parse(aic_reader)

    def test_calculate_pmu_list(self):
        aic_reader = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 78, 9, 1, 0, 1, 2, 3, 4, 6, 5, 2, 4, 5, 6, 7, 89, 9,
                                 7, 4)
        _aic_pmu_log = AicPmuBean.decode(aic_reader)
        with mock.patch(NAMESPACE + '.CalculateAiCoreData.compute_ai_core_data', return_value=('', {})), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.Utils.generator_to_list', return_value=[]):
            check = AicCalculator(self.file_list, CONFIG)
            check.calculate_pmu_list(_aic_pmu_log, [], [], 0)

    def test_save(self):
        with mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.AicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = AicCalculator(self.file_list, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = AicCalculator({}, CONFIG)
            check.ms_run()
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.init_params', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.AicCalculator.save', return_value='test'):
            check = AicCalculator(self.file_list, CONFIG)
            check.ms_run()


class TestV5AicCalculator(unittest.TestCase):

    def test_calculate_when_nomal_then_pass(self):
        with mock.patch('msmodel.v5.v5_stars_model.V5StarsViewModel.get_v5_pmu_details'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch("mscalculate.aic.aic_utils.AicPmuUtils.get_pmu_events"), \
                mock.patch('mscalculate.aic.aic_calculator.AicCalculator._core_num_dict'), \
                mock.patch('common_func.utils.Utils.cal_total_time'), \
                mock.patch('mscalculate.aic.aic_calculator.AicCalculator.calculate_pmu_list'):
            check = V5AicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check.calculate()

    def test_calculate_when_table_exist_then_do_not_execute(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = V5AicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check._parse_without_decode = mock.Mock()
            check.calculate()
            check._parse_without_decode.assert_not_called()

    def test_save_when_nomal_then_pass(self):
        with mock.patch('msmodel.aic.aic_pmu_model.V5AicPmuModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch('msmodel.aic.aic_pmu_model.V5AicPmuModel.flush'), \
                mock.patch('msmodel.aic.aic_pmu_model.V5AicPmuModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = V5AicCalculator({}, CONFIG)
            check._aic_data_list = [123]
            check.save()

    def test_ms_run_when_json_empty_then_return(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_mode': StrConstant.AIC_SAMPLE_BASED_MODE}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'):
            check = V5AicCalculator({}, CONFIG)
            check.ms_run()

    def test_ms_run_when_normal_then_pass(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                mock.patch(NAMESPACE + '.PathManager.get_sample_json_path', return_value='test'), \
                mock.patch(NAMESPACE + '.V5AicCalculator.init_params', return_value='test'), \
                mock.patch(NAMESPACE + '.V5AicCalculator.calculate', return_value='test'), \
                mock.patch(NAMESPACE + '.V5AicCalculator.save', return_value='test'):
            check = V5AicCalculator({}, CONFIG)
            check.ms_run()
