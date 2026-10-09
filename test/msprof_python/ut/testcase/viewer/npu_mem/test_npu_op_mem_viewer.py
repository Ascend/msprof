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

from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.npu_op_mem_rec_dto import NpuOpMemRecDto
from profiling_bean.db_dto.npu_module_mem_dto import NpuModuleMemDto
from profiling_bean.db_dto.op_mem_dto import OpMemDto
from viewer.npu_mem.npu_op_mem_viewer import NpuOpMemViewer

NAMESPACE = 'viewer.npu_mem.npu_op_mem_viewer'


class TestNpuOpMemViewer(unittest.TestCase):

    def test_get_table_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["component", "timestamp", "total_reserve_memory", "total_allocate_memory", "device_type"]}
        params = {
            "project": "test_npu_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuOpMemViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)


    def test_get_summary_data_should_return_success_when_get_mem_data(self):
        config = {
            "headers": ["operator", "size", "allocation_time", "release_time", "duration",
                        "allocation_total_allocated", "allocation_total_reserved", "release_total_allocated",
                        "release_total_reserved", "device_type", "name"]
        }
        params = {
            "project": "test_npu_op_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = [
            'operator', 'size', 'allocation_time', 'release_time', 'duration', 'allocation_total_allocated',
            'allocation_total_reserved', 'release_total_allocated', 'release_total_reserved', 'device_type', 'name'
        ]
        expected_data = [['123', 0.0, '10.000\t', 0.001, 0.0, 0.0, 0.0, 0.0, 'NPU:5']]
        InfoConfReader()._host_freq = 1000000000.0
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        mem_dto = OpMemDto()
        mem_dto.operator = '123'
        mem_dto.size = 0
        mem_dto.allocation_time = 0
        mem_dto.release_time = 0
        mem_dto.duration = 1
        mem_dto.allocation_total_allocated = 0
        mem_dto.allocation_total_reserved = 0
        mem_dto.release_total_allocated = 0
        mem_dto.release_total_reserved = 0
        mem_dto.device_type = 'NPU:5'
        mem_dto.name = '123'

        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=[mem_dto]):
            check = NpuOpMemViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)
