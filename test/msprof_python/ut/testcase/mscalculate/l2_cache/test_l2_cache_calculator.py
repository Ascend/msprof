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
function: test l2_cache_calculator
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from mscalculate.l2_cache.l2_cache_calculator import L2CacheCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.l2_cache.l2_cache_calculator'
MODEL_NAMESPACE = 'msmodel.l2_cache.l2_cache_calculator_model'


class TestL2CacheCalculator(unittest.TestCase):
    sample_config = {'l2CacheTaskProfiling': 'on',
                     'l2CacheTaskProfilingEvents': '0x78,0x79,0x77,0x71,0x6A,0x6C,0x74,0x62',
                     'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'host_id': '127.0.0.1'}
    l2_cache_ps_data = [[0, 3, 2, '640', '3136', '0', '0', '47', '3776', '0', '3159'],
                        [0, 3, 3, '5524', '3136', '0', '0', '5313', '8660', '0', '3151'],
                        [0, 3, 4, '3432', '784', '0', '0', '3425', '4216', '0', '791'],
                        [0, 3, 5, '896', '784', '0', '0', '870', '1680', '0', '794'],
                        [0, 3, 6, '1601', '784', '0', '0', '1436', '2385', '0', '805']]
    file_list = {DataTag.L2CACHE: ['l2cache.data.0.slice_0']}

    def test_is_valid_index(self: any):
        valid_dict = {"request_events": [{'coefficient': 1, 'index': 0}, {'coefficient': 1, 'index': 1}],
                      "hit_events": [{'coefficient': 1, 'index': 4}],
                      "victim_events": [{'coefficient': 1, 'index': 3}]}
        not_valid_dict = {"request_events": [{'coefficient': -1, 'index': -1}],
                          "hit_events": [{'coefficient': 1, 'index': 4}],
                          "victim_events": [{'coefficient': 1, 'index': 3}]}
        self.assertEqual(L2CacheCalculator._is_valid_index(valid_dict), True)
        self.assertEqual(L2CacheCalculator._is_valid_index(not_valid_dict), False)

    def test_get_l2_cache_ps_data(self: any):
        with mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.get_all_data', return_value=[]), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            self.assertEqual(check._get_l2_cache_ps_data(), [])

    def test_get_event_index(self: any):
        event_type_ok = '0x78'
        event_type_fail = '0x88'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            self.assertEqual(check._get_event_index(event_type_ok), {'coefficient': 1, 'index': 0})
            self.assertEqual(check._get_event_index(event_type_fail), {'coefficient': -1, 'index': -1})

    def test_get_event_index_when_event_type_is_negative_then_get_index_success(self: any):
        event_type_ok = '0x88'
        event_type_ok_neg = '-0x97'
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x00','0x88','0x89,''0x8A','0x74','0x75','0x97']
            self.assertEqual(check._get_event_index(event_type_ok), {'coefficient': 1, 'index': 1})
            self.assertEqual(check._get_event_index(event_type_ok_neg), {'coefficient': -1, 'index': 5})

    def test_pre_check(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            InfoConfReader()._info_json = {"platform_version": '2'}
            check = L2CacheCalculator({}, self.sample_config)
            InfoConfReader()._info_json = {"platform_version": '3'}
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            self.assertEqual(check._pre_check(), True)
            InfoConfReader()._info_json = {"platform_version": '1'}
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            self.assertEqual(check._pre_check(), True)
            InfoConfReader()._info_json = {"platform_version": '1111'}
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            self.assertEqual(check._pre_check(), False)
            InfoConfReader()._info_json = {"platform_version": '1'}
            check._l2_cache_events = []
            self.assertEqual(check._pre_check(), False)
            check._l2_cache_events = ["111"]
            self.assertEqual(check._pre_check(), False)
            check._l2_cache_events = ['1', 'x', '2', '3', '4', '5', '6', '7', '8', '9']
            self.assertEqual(check._pre_check(), False)

    def test_set_l2_cache_events_indexes(self: any):
        expected_event_indexes_1951 = {
            "request_events": [{'coefficient': 1, 'index': 0}, {'coefficient': 1, 'index': 1}],
            "hit_events": [{'coefficient': 1, 'index': 4}],
            "victim_events": [{'coefficient': 1, 'index': 3}]
        }
        expected_event_indexes_1980 = {
            "request_events": [{'coefficient': 1, 'index': 1}],
            "hit_events": [{'coefficient': 1, 'index': 0}],
            "victim_events": [{'coefficient': 1, 'index': 2}]
        }
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._platform_type = None
            check._set_l2_cache_events_indexes()
        # 1951
        with mock.patch(NAMESPACE + '.configparser.ConfigParser.get', side_effect=['0x78,0x79', '0x6a', '0x71']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            check._platform_type = '3'
            res_1951 = check._set_l2_cache_events_indexes()
            self.assertEqual(check._event_indexes, expected_event_indexes_1951)
        with mock.patch(NAMESPACE + '.configparser.ConfigParser.get', side_effect=['0x78,0x79', '0x6a', '0x71']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.configparser.ConfigParser.items', return_value=''):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x78', '0x7m', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            check._platform_type = '3'
            res_1951 = check._set_l2_cache_events_indexes()
        # 1980
        with mock.patch(NAMESPACE + '.configparser.ConfigParser.get', side_effect=['0x59', '0x5b', '0x5c']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x5b', '0x59', '0x5c', '0x7d', '0x7e', '0x71', '0x79', '0x7c']
            check._platform_type = '1'
            res_1951 = check._set_l2_cache_events_indexes()
            self.assertEqual(check._event_indexes, expected_event_indexes_1980)
        with mock.patch(NAMESPACE + '.configparser.ConfigParser.get', side_effect=['0x78,0x79', '0x6a', '0x71']), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.configparser.ConfigParser.items', return_value=''):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_events = ['0x5b', '0x5m', '0x5c', '0x7d', '0x7e', '0x71', '0x79', '0x7c']
            check._platform_type = '1'
            res_1951 = check._set_l2_cache_events_indexes()

    def test_cal_metrics(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check._l2_cache_ps_data = self.l2_cache_ps_data
            check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62']
            except_data_result = [[0, 3, 2, 0.012447, 0.0],
                                  [0, 3, 3, 0.61351, 0.0],
                                  [0, 3, 4, 0.812381, 0.0],
                                  [0, 3, 5, 0.517857, 0.0],
                                  [0, 3, 6, 0.602096, 0.0]]
            check._event_indexes = {
                "request_events": [{'coefficient': 1, 'index': 0}, {'coefficient': 1, 'index': 1}],
                "hit_events": [{'coefficient': 1, 'index': 4}],
                "victim_events": [{'coefficient': 1, 'index': 3}]
            }
            check._cal_metrics()
            self.assertEqual(check._l2_cache_cal_data, except_data_result)

    def test_save(self: any):
        with mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.flush'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.finalize'):
            check = L2CacheCalculator({}, self.sample_config)
            check.save()
            check._l2_cache_cal_data = [[0, 0, 3, 2, 0.012447, 0.0],
                                        [0, 0, 3, 3, 0.61351, 0.0],
                                        [0, 0, 3, 4, 0.812381, 0.0],
                                        [0, 0, 3, 5, 0.517857, 0.0],
                                        [0, 0, 3, 6, 0.602096, 0.0]]
            check.save()

    def test_calculate(self: any):
        with mock.patch(NAMESPACE + '.L2CacheCalculator._set_l2_cache_events_indexes'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.L2CacheCalculator._get_l2_cache_ps_data', return_value=[]):
            check = L2CacheCalculator({}, self.sample_config)
            check.calculate()
        with mock.patch(NAMESPACE + '.L2CacheCalculator._set_l2_cache_events_indexes'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.init'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('configparser.ConfigParser.items'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.L2CacheCalculator._is_valid_index', return_value=False), \
                mock.patch(NAMESPACE + '.L2CacheCalculator._get_l2_cache_ps_data', return_value=[]):
            check = L2CacheCalculator({}, self.sample_config)
            check.calculate()
        with mock.patch(NAMESPACE + '.L2CacheCalculator._set_l2_cache_events_indexes'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.init'), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.L2CacheCalculator._get_l2_cache_ps_data', return_value=[1]), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheCalculatorModel.split_events_data', return_value=[1]), \
                mock.patch(NAMESPACE + '.L2CacheCalculator._cal_metrics'):
            check = L2CacheCalculator({}, self.sample_config)
            check.calculate()

    def test_ms_run(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({}, self.sample_config)
            check.ms_run()
        with mock.patch(NAMESPACE + '.L2CacheCalculator._pre_check', return_value=False), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config):
            check = L2CacheCalculator({1: '1'}, self.sample_config)
            check.ms_run()
        with mock.patch(NAMESPACE + '.L2CacheCalculator._pre_check', return_value=True), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch(NAMESPACE + '.L2CacheCalculator.calculate', return_value=True):
            check = L2CacheCalculator(self.file_list, self.sample_config)
            check._l2_cache_ps_data = [[0, 0, 1, 1, '26', '4', '0', '0', '14', '30', '4', '9']]
            check._l2_cache_events = ['0x5b', '0x59', '0x5c', '0x7d', '0x7e', '0x71', '0x79', '0x7c']
            check.ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self: any):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=self.sample_config), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = L2CacheCalculator(self.file_list, self.sample_config)
            check.calculate = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()


if __name__ == '__main__':
    unittest.main()
