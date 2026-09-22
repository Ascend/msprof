#!/usr/bin/python3
# -*- coding: utf-8 -*-
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

import unittest
from unittest import mock

from common_func.msvp_constant import MsvpConstant
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.npu_module_mem_dto import NpuModuleMemDto
from viewer.npu_mem.npu_module_mem_viewer import NpuModuleMemViewer
from model.test_dir_cr_base_model import TestDirCRBaseModel

NAMESPACE = 'viewer.npu_mem.npu_module_mem_viewer'


class TestNpuModuleMemViewer(TestDirCRBaseModel):

    def get_module_mem_dto(self, module_mem: list) -> list:
        module_mem_dto = []
        for data in module_mem:
            data_dto = NpuModuleMemDto()
            data_dto.module_id = data[0]
            data_dto.syscnt = data[1]
            data_dto.total_size = data[2]
            data_dto.device_type = data[3]
            module_mem_dto.append(data_dto)
        return module_mem_dto

    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuModuleMemViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_number_when_invalid_module_id_is_included(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_reserve_memory', 'device_type']
        expected_data = [
            ['UNKNOWN', '10.000\t', 4, 'NPU:0']
        ]
        InfoConfReader()._info_json = {
            'CPU': [{'Frequency': "1000"}]
        }
        InfoConfReader()._local_time_offset = 10.0
        invalid_module_mem_data = [
            ['other', 0, 4096, 'NPU:0']
        ]
        invalid_module_mem_dto = self.get_module_mem_dto(invalid_module_mem_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=invalid_module_mem_dto):
            check = NpuModuleMemViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)

    def test_get_summary_data_should_return_module_men_data_when_get_module_mem_data(self):
        InfoConfReader()._host_freq = 1000000000.0
        InfoConfReader()._local_time_offset = 10.0
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "device_type"]}
        params = {
            "project": "test_npu_module_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_reserve_memory', 'device_type']
        expected_data = [
            ['RUNTIME', '10.000\t', 4, 'NPU:0'],
            ['UNKNOWN', '11.000\t', 2, 'NPU:0'],
            ['CCE', '11.000\t', -1, 'NPU:0'],
            ['AML', '12.333\t', -1, 'NPU:0'],
            ['CUSTOM_SCHEDULE', '12.333\t', -1, 'NPU:0'],
            ['PYPTO', '12.333\t', -1, 'NPU:0'],
            ['UNKNOWN', '12.333\t', -1, 'NPU:0']
        ]
        module_mem_data = [
            [7, 0, 4096, 'NPU:0'],
            [15, 1000, 2048, 'NPU:0'],
            [8, 1000, -1, 'NPU:0'],
            [74, 2333, -1, 'NPU:0'],
            [76, 2333, -1, 'NPU:0'],
            [59, 2333, -1, 'NPU:0'],
            [78, 2333, -1, 'NPU:0']
        ]
        module_mem_dto = self.get_module_mem_dto(module_mem_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=module_mem_dto):
            check = NpuModuleMemViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)
