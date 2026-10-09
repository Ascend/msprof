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
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import logging
import os
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_object import CustomizedNamedtupleFactory
from constant.constant import clear_dt_project
from msparser.cluster.cluster_step_trace_parser import ClusterStepTraceParser
from profiling_bean.db_dto.step_trace_dto import TrainingTraceDto

NAMESPACE = 'msparser.cluster.cluster_step_trace_parser'


class TestStepTraceParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_StepTraceParser')

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run_should_return_empty_when_path_invalid(self):
        check = ClusterStepTraceParser('DT_StepTraceParser')
        check.ms_run()

    def test_ms_run_should_return_empty_when_path_valid_db_not_exist(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceParser._check_collection_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            check = ClusterStepTraceParser(self.DIR_PATH)
            check.ms_run()

    def test_ms_run_should_return_empty_when_path_valid_table_not_exist(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceParser._check_collection_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterStepTraceParser.save'):
            check = ClusterStepTraceParser(self.DIR_PATH)
            check.ms_run()

    def test_get_syscnt_time_should_return_correct_value_when_called(self):
        self.assertEqual(ClusterStepTraceParser._get_syscnt_time(0), 0)
        InfoConfReader()._info_json = {'DeviceInfo': [{"hwts_frequency": 100000}]}
        self.assertEqual(ClusterStepTraceParser._get_syscnt_time(300000), 3)

    def test_get_data_list_from_dto_should_return_correct_tuple_when_called(self):
        training_trace_dto_tuple = (
            CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TrainingTraceDto, []))
        values = [
            training_trace_dto_tuple(fp_start=10000000, fp_bp_time=12000000, iteration_time=10, iteration_end=1000,
                                  bp_end=100000)
        ]
        InfoConfReader()._info_json = {'DeviceInfo': [{"hwts_frequency": 100000}]}
        res = ClusterStepTraceParser._get_data_list_from_dto(values)
        self.assertEqual(res, [(None, None, None, 100.0, 1.0, 0.01, 10, 12000000, None, None)])
