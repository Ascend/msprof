#!/usr/bin/python3
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

from mscalculate.step_trace.create_step_table import CreateStepTime
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto

NAMESPACE = 'mscalculate.step_trace.create_step_table'


class TestCreateStepTime(unittest.TestCase):
    def test_extract_data_should_return_2_data_when_2_step_is_running(self):
        step_data = [
            StepTraceOriginDto(1, 4294967295, 0, 60000, 0, 1000),
            StepTraceOriginDto(1, 4294967295, 0, 60001, 4, 1300),
            StepTraceOriginDto(2, 4294967295, 0, 60000, 6, 1500),
            StepTraceOriginDto(2, 4294967295, 0, 60001, 9, 1900),
        ]
        CreateStepTime.sample_config = {"result_dir": "./"}
        with mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_with_tag", return_value=step_data):
            CreateStepTime.extract_data()
            CreateStepTime.connect_db()
            res = [
                [1, 4294967295, 1000, 1300, 1],
                [2, 4294967295, 1500, 1900, 2],
            ]
            self.assertEqual(res, CreateStepTime.data)
        CreateStepTime.sample_config = {}
        CreateStepTime.data = []

    def test_extract_data_should_return_1_data_when_there_is_only_start_tag(self):
        step_data = [
            StepTraceOriginDto(1, 4294967295, 0, 60000, 0, 1000),
            StepTraceOriginDto(1, 4294967295, 0, 60001, 4, 1300),
            StepTraceOriginDto(2, 4294967295, 0, 60000, 6, 1500),
        ]
        CreateStepTime.sample_config = {"result_dir": "./"}
        with mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_with_tag", return_value=step_data):
            CreateStepTime.extract_data()
            res = [
                [1, 4294967295, 1000, 1300, 1],
            ]
            self.assertEqual(res, CreateStepTime.data)
        CreateStepTime.sample_config = {}
        CreateStepTime.data = []
