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
Copyright Huawei Technologies Co., Ltd. 2023. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from viewer.api_statistic_viewer import ApiStatisticViewer

NAMESPACE = 'viewer.api_statistic_viewer'


class TestApiStatisticViewer(unittest.TestCase):

    def test_get_api_summary_data_should_return_list_when_data_exists(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 1,
            "iter_id": 1
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_summary_data()
            self.assertEqual([(2, 0, 0.001, 1, 0.001, 0.001, 0.001, 0.0)], ret)

    def test_get_api_summary_data_should_return_empty_list_when_model_init_fails(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 2,
            "iter_id": 2
        }
        check = ApiStatisticViewer(config, params)
        ret = check.get_api_summary_data()
        self.assertEqual(MsvpConstant.EMPTY_LIST, ret)

    def test_get_api_summary_data_should_return_empty_list_when_data_misses(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 3,
            "iter_id": 3
        }
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_api_statistic_data", return_value=[]):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_summary_data()
            self.assertEqual(MsvpConstant.EMPTY_LIST, ret)

    def test_get_api_statistic_data_should_return_tuple_data_exists(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 4,
            "iter_id": 4
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["Level", "API Name", "Time(us)", "Count",
                              "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(2, 0, 0.001, 1, 0.001, 0.001, 0.001, 0.0)],
                              1), ret)

    def test_get_api_statistic_data_should_return_tuple_when_data_contains_two_level(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 5,
            "iter_id": 5
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2)]
        data1 = [(3, 4, 5)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data + data1):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["Level", "API Name", "Time(us)", "Count",
                               "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(2, 0, 0.001, 1, 0.001, 0.001, 0.001, 0.0),
                               (5, 3, 0.004, 1, 0.004, 0.004, 0.004, 0.0)],
                              2), ret)

    def test_get_api_statistic_data_should_return_success_when_data_contains_single_level_and_one_level_name(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 6,
            "iter_id": 6
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 6, 2)]
        data1 = [(0, 8, 2)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data + data1):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["Level", "API Name", "Time(us)", "Count",
                              "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(2, 0, 0.014, 2, 0.007, 0.006, 0.008, 0)], 1), ret)

    def test_get_api_statistic_data_should_return_success_when_data_contains_single_level_and_two_api_name(self):
        config = {
            'headers': [
                'Level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 7,
            "iter_id": 7
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 6, 2)]
        data1 = [(1, 8, 2)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data + data1):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["Level", "API Name", "Time(us)", "Count",
                              "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(2, 0, 0.006, 1, 0.006, 0.006, 0.006, 0.0),
                               (2, 1, 0.008, 1, 0.008, 0.008, 0.008, 0.0)],
                              2), ret)
