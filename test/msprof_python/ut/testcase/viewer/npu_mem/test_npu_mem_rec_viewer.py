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

from common_func.msvp_constant import MsvpConstant
from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.npu_op_mem_rec_dto import NpuOpMemRecDto
from viewer.npu_mem.npu_mem_rec_viewer import NpuMemRecViewer
from model.test_dir_cr_base_model import TestDirCRBaseModel

NAMESPACE = 'viewer.npu_mem.npu_mem_rec_viewer'


class TestNpuMemRecViewer(TestDirCRBaseModel):

    def get_op_mem_rec_dto(self, op_mem_rec: list) -> list:
        op_mem_rec_dto = []
        for data in op_mem_rec:
            data_dto = NpuOpMemRecDto()
            data_dto.component = data[0]
            data_dto.timestamp = data[1]
            data_dto.total_reserve_memory = data[2]
            data_dto.total_allocate_memory = data[3]
            data_dto.device_type = data[4]
            op_mem_rec_dto.append(data_dto)
        return op_mem_rec_dto

    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["component", "timestamp", "total_allocate_memory", "total_reserve_memory", "device_type"]}
        params = {
            "project": "test_npu_mem_rec_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = NpuMemRecViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_op_men_data_when_get_op_mem_rec_data(self):
        config = {"headers": ["component", "timestamp", "total_allocate_memory", "total_reserve_memory", "device_type"]}
        params = {
            "project": "test_npu_op_mem_view",
            "model_id": 1,
            "iter_id": 1
        }
        expected_headers = ['component', 'timestamp', 'total_allocate_memory', 'total_reserve_memory', 'device_type']
        expected_data = [
            ['GE', '10.000\t', 0.0, 0.0, 'NPU:5'],
            ['GE', '11.000\t', 1.0, 1.0, 'NPU:5']
        ]
        InfoConfReader()._host_freq = 1000000000.0
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        op_mem_rec_data = [
            ['GE', 0, 0, 0, 'NPU:5'],
            ['GE', 1000, 1024, 1024, 'NPU:5']
        ]
        op_mem_rec_dto = self.get_op_mem_rec_dto(op_mem_rec_data)
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.get_table_data', return_value=op_mem_rec_dto):
            check = NpuMemRecViewer(config, params)
            headers, data, _ = check.get_summary_data()
            self.assertEqual(headers, expected_headers)
            self.assertEqual(data, expected_data)
