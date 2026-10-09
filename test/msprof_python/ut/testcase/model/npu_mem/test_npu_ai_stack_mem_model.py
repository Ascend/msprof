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

from msmodel.npu_mem.npu_ai_stack_mem_model import NpuAiStackMemModel
from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel

NAMESPACE = 'msmodel.npu_mem.npu_ai_stack_mem_model'


class TestNpuAiStackMemModel(TestDirCRBaseModel):

    def test_flush(self):
        with mock.patch(NAMESPACE + ".NpuAiStackMemModel.check_db"), \
                mock.patch(NAMESPACE + ".NpuAiStackMemModel.check_table"), \
                mock.patch(NAMESPACE + ".NpuAiStackMemModel.create_table"), \
                mock.patch(NAMESPACE + ".NpuAiStackMemModel.insert_data_to_db"):
            check = NpuAiStackMemModel("test", DBNameConstant.DB_MEMORY_OP, ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.flush(table_name, [])

    def test_get_table_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuAiStackMemModel("test", DBNameConstant.DB_MEMORY_OP, ["NpuOpMem"])
            table_name = "NpuOpMem"
            check.get_table_data(table_name)

    def test__get_table_data_should_return_empty_when_no_db(self):
        model = NpuAiStackMemModel(self.PROF_DIR, DBNameConstant.DB_MEMORY_OP, ["NpuOpMem"])
        table_name = "NpuOpMem"
        ret = model.get_table_data(table_name)
        self.assertEqual(ret, [])

    def test__get_table_data_should_return_no_empty_when_get_npu_op_mem_raw_data(self):
        npu_op_mem_raw_origin_data = [
            [1000, 2000, 196608, 3000, 1, 262144, 623706, 10000, 6, 'NPU:1'],
            [1000, 2000, 4096, 4500, 1, 567718, 666666, 10000, 6, 'NPU:1']
        ]
        model = NpuAiStackMemModel(self.PROF_HOST_DIR, DBNameConstant.DB_MEMORY_OP, ["NpuOpMemRaw"])
        table_name = "NpuOpMemRaw"
        with model:
            model.flush(DBNameConstant.TABLE_NPU_OP_MEM_RAW, npu_op_mem_raw_origin_data)
            npu_op_mem_raw_data = model.get_table_data(table_name)
        self.assertEqual(len(npu_op_mem_raw_data), 2)

    def test__get_table_data_should_return_no_empty_when_get_npu_op_mem_data(self):
        npu_op_mem_origin_data = [
            [29722, 196608, 4672, 9555, 4883, 262144, 6112, 65536, 6112, 'NPU:1', 'Graph_67'],
            [65855, 65536, 4362, 7742, 3380, 131072, 6112, 65536, 6112, 'NPU:1', 'Graph_86']
        ]
        model = NpuAiStackMemModel(self.PROF_HOST_DIR, DBNameConstant.DB_MEMORY_OP, ["NpuOpMem"])
        table_name = "NpuOpMem"
        with model:
            model.flush(DBNameConstant.TABLE_NPU_OP_MEM, npu_op_mem_origin_data)
            npu_op_mem_data = model.get_table_data(table_name)
        self.assertEqual(len(npu_op_mem_data), 2)

    def test__get_table_data_should_return_no_empty_when_get_npu_op_mem_rec_data(self):
        npu_op_mem_rec_origin_data = [
            ['GE', 4672, 6112, 262144, 'NPU:1'],
            ['GE', 9555, 6112, 65536, 'NPU:1']
        ]
        model = NpuAiStackMemModel(self.PROF_HOST_DIR, DBNameConstant.DB_MEMORY_OP, ["NpuOpMemRec"])
        table_name = "NpuOpMemRec"
        with model:
            model.flush(DBNameConstant.TABLE_NPU_OP_MEM_REC, npu_op_mem_rec_origin_data)
            npu_op_mem_rec_data = model.get_table_data(table_name)
        self.assertEqual(len(npu_op_mem_rec_data), 2)

    def test__get_table_data_should_return_no_empty_when_get_npu_module_mem_data(self):
        npu_module_mem_origin_data = [
            [0, 7324, 0, 'NPU:1'],
            [0, 7428, 1000, 'NPU:1']
        ]
        model = NpuAiStackMemModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_NPU_MODULE_MEM, ["NpuModuleMem"])
        table_name = "NpuModuleMem"
        with model:
            model.flush(DBNameConstant.TABLE_NPU_MODULE_MEM, npu_module_mem_origin_data)
            npu_module_mem_data = model.get_table_data(table_name)
        self.assertEqual(len(npu_module_mem_data), 2)
