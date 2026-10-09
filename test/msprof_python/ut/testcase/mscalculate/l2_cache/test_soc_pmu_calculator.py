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
function: test soc_pmu_calculator
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from mscalculate.l2_cache.soc_pmu_calculator import SocPmuCalculator
from msconfig.config_manager import ConfigManager
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.l2_cache.soc_pmu_calculator'
MODEL_NAMESPACE = 'msmodel.l2_cache.soc_pmu_model'


class TestSocPmuCalculator(unittest.TestCase):
    sample_config = {
        'npuEvents': 'HA:0x00,0x88,0x89,0x8A,0x74,0x75,0x97;SMMU:0x2,0x8a,0x8b,0x8c,0x8d',
        'result_dir': '/tmp/result',
        'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
        'host_id': '127.0.0.1'
    }
    soc_pmu_ps_data = [[0, 1, 1, '4', '5', '4', '1', '0', '0', '0', '0'],
                       [0, 1, 2, '4', '5', '4', '1', '0', '0', '0', '0'],
                       [0, 1, 3, '4', '5', '4', '1', '0', '0', '0', '0'],
                       [0, 1, 4, '1', '4', '1', '3', '0', '0', '0', '0']]
    file_list = {DataTag.SOC_PMU: ['socpmu.data.0.slice_0']}

    def test_is_valid_index_where_index_is_valid_and_invalid(self: any):
        valid_dict = {"request_events": [{'coefficient': 1, 'index': 0}, {'coefficient': 1, 'index': 1}],
                      "hit_events": [{'coefficient': 1, 'index': 4}],
                      "victim_events": [{'coefficient': 1, 'index': 3}]}
        not_valid_dict = {"request_events": [{'coefficient': -1, 'index': -1}],
                          "hit_events": [{'coefficient': 1, 'index': 4}],
                          "victim_events": [{'coefficient': 1, 'index': 3}]}
        self.assertEqual(SocPmuCalculator._is_valid_index(valid_dict), True)
        self.assertEqual(SocPmuCalculator._is_valid_index(not_valid_dict), False)

    def test_get_event_index_when_event_type_success_and_fail(self: any):
        event_type_success = '0x8d'
        event_type_fail = '0x80'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = SocPmuCalculator({}, self.sample_config)
            self.assertEqual(check._get_event_index(event_type_success), {'coefficient': 1, 'index': 4})
            self.assertEqual(check._get_event_index(event_type_fail), {'coefficient': -1, 'index': -1})

    def test_pre_check(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            InfoConfReader()._info_json = {"platform_version": '5'}
            check = SocPmuCalculator({}, self.sample_config)
            check._events = ["0x2", "0x8a", "0x8b", "0x8b", "0x8c", "0x8d"]
            self.assertEqual(check._pre_check(), True)
            InfoConfReader()._info_json = {"platform_version": '15'}
            check = SocPmuCalculator({}, self.sample_config)
            check._events = ["0x2", "0x8a", "0x8b"]
            self.assertEqual(check._pre_check(), True)
            InfoConfReader()._info_json = {"platform_version": '1'}
            check = SocPmuCalculator({}, self.sample_config)
            self.assertEqual(check._pre_check(), False)
            InfoConfReader()._info_json = {"platform_version": '1111'}
            check = SocPmuCalculator({}, self.sample_config)
            check._events = ["0x2", "0x8a", "0x8b", "0x8b", "0x8c", "0x8d"]
            self.assertEqual(check._pre_check(), False)
            InfoConfReader()._info_json = {"platform_version": '5'}
            check._events = []
            self.assertEqual(check._pre_check(), False)
            check._events = ["111"]
            self.assertEqual(check._pre_check(), False)
            check._events = ['1', 'x', '2', '3', '4', '5', '6', '7', '8', '9']
            self.assertEqual(check._pre_check(), False)

    def test_set_soc_pmu_events_indexes(self: any):
        expected_event_indexes = {
            'hit_events': [{'coefficient': 1, 'index': 3}, {'coefficient': 1, 'index': 4}],
            'miss_events': [{'coefficient': 1, 'index': 0}],
            'request_events': [{'coefficient': 1, 'index': 1}]
        }
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = SocPmuCalculator({}, self.sample_config)
            check._platform_type = "15"
            check._cfg_parser = ConfigManager.get(ConfigManager.SOC_PMU)
            check._set_soc_pmu_events_indexes()
            self.assertEqual(check._event_indexes, expected_event_indexes)

    def test_cal_metrics(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = SocPmuCalculator({}, self.sample_config)
            check._soc_pmu_ps_data = self.soc_pmu_ps_data
            check._events = ["0x2", "0x8a", "0x8b", "0x8b", "0x8c", "0x8d"]
            except_data_result = [[0, 1, 1, 0.2, 0.8],
                                  [0, 1, 2, 0.2, 0.8],
                                  [0, 1, 3, 0.2, 0.8],
                                  [0, 1, 4, 0.75, 0.25]]
            check._event_indexes = {
                'hit_events': [{'coefficient': 1, 'index': 3}, {'coefficient': 1, 'index': 4}],
                'miss_events': [{'coefficient': 1, 'index': 0}],
                'request_events': [{'coefficient': 1, 'index': 1}]
            }
            check._cal_metrics()
            self.assertEqual(check._soc_pmu_cal_data, except_data_result)

    def test_save(self: any):
        with mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.flush'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.finalize'):
            check = SocPmuCalculator({}, self.sample_config)
            check.save()
            check._soc_pmu_cal_data = [[0, 1, 1, 0.2, 0.8],
                                       [0, 1, 2, 0.2, 0.8],
                                       [0, 1, 3, 0.2, 0.8],
                                       [0, 1, 4, 0.75, 0.25]]
            check.save()

    def test_calculate_with_no_data(self):
        with mock.patch(NAMESPACE + '.SocPmuCalculator._set_soc_pmu_events_indexes'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                           return_value=self.sample_config), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.init'):
            check = SocPmuCalculator({}, self.sample_config)
            check.calculate()

    def test_calculate_with_invalid_index(self):
        with mock.patch(NAMESPACE + '.SocPmuCalculator._set_soc_pmu_events_indexes'), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.init'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('configparser.ConfigParser.items'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.SocPmuCalculator._is_valid_index', return_value=False):
            check = SocPmuCalculator({}, self.sample_config)
            check.calculate()

    def test_calculate_with_data(self):
        with mock.patch(NAMESPACE + '.SocPmuCalculator._set_soc_pmu_events_indexes'), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuCalculatorModel.split_events_data', return_value=[1]), \
                mock.patch(NAMESPACE + '.SocPmuCalculator._cal_metrics'):
            check = SocPmuCalculator({}, self.sample_config)
            check.calculate()

    def test_ms_run_where_data_is_empty(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = SocPmuCalculator({}, self.sample_config)
            check.ms_run()

    def test_ms_run_with_pre_check_false(self: any):
        with mock.patch(NAMESPACE + '.SocPmuCalculator._pre_check', return_value=False), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = SocPmuCalculator({1: '1'}, self.sample_config)
            check.ms_run()

    def test_ms_run_with_pre_check_true_and_calculate_success(self: any):
        with mock.patch(NAMESPACE + '.SocPmuCalculator._pre_check', return_value=True), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.SocPmuCalculator.calculate', return_value=True):
            check = SocPmuCalculator(self.file_list, self.sample_config)
            check._soc_pmu_ps_data = self.soc_pmu_ps_data
            check._events = ["0x2", "0x8a", "0x8b", "0x8b", "0x8c", "0x8d"]
            check.ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = SocPmuCalculator(self.file_list, self.sample_config)
            check.calculate = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()


if __name__ == '__main__':
    unittest.main()
