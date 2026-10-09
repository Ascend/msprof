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

from constant.constant import CONFIG
from mscalculate.npu_mem.npu_op_mem_calculator import NpuOpMemCalculator
from profiling_bean.db_dto.npu_op_mem_dto import NpuOpMemDto

NAMESPACE = 'mscalculate.npu_mem.npu_op_mem_calculator'


class TestNpuOpMemCalculator(unittest.TestCase):

    def test_save(self):
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.finalize'):
            check = NpuOpMemCalculator({}, CONFIG)
            setattr(check, "_memory_record", ['GE', '1', 0, 1, 'NPU:0'])
            result = check.save()
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.finalize'):
            check = NpuOpMemCalculator({}, CONFIG)
            setattr(check, "_operator_memory", ['a', 1, '0', '1', '1', 0, 1, 0, 1, "NPU:0", "a"])
            result = check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuOpMemCalculator._judge_should_calculate'), \
                mock.patch(NAMESPACE + '.NpuOpMemCalculator.calculate'), \
                mock.patch(NAMESPACE + '.NpuOpMemCalculator.save'):
            check = NpuOpMemCalculator({}, CONFIG)
            check.ms_run()

    def test_calc_operator_memory(self):
        check = NpuOpMemCalculator({}, CONFIG)

        npu_op_mem_dto_1 = NpuOpMemDto()
        npu_op_mem_dto_1.operator = '1'
        npu_op_mem_dto_1.addr = 100
        npu_op_mem_dto_1.size = 1
        npu_op_mem_dto_1.timestamp = 111
        npu_op_mem_dto_1.thread_id = 312
        npu_op_mem_dto_1.total_allocate_memory = 0
        npu_op_mem_dto_1.total_reserve_memory = 1
        npu_op_mem_dto_1.level = 10000
        npu_op_mem_dto_1.type = 6
        npu_op_mem_dto_1.device_type = "NPU:0"

        npu_op_mem_dto_2 = NpuOpMemDto()
        npu_op_mem_dto_2.operator = '1'
        npu_op_mem_dto_2.addr = 100
        npu_op_mem_dto_2.size = -1
        npu_op_mem_dto_2.timestamp = 112
        npu_op_mem_dto_2.thread_id = 312
        npu_op_mem_dto_2.total_allocate_memory = 0
        npu_op_mem_dto_2.total_reserve_memory = 1
        npu_op_mem_dto_2.level = 10000
        npu_op_mem_dto_2.type = 6
        npu_op_mem_dto_2.device_type = "NPU:0"

        setattr(check, "_op_data", [npu_op_mem_dto_1, npu_op_mem_dto_2])
        setattr(check, "_operator_memory", [])
        check._calc_operator_memory()
        self.assertEqual(check._operator_memory, [['1', 1, 111, 112, 1, 0, 1, 0, 1, "NPU:0", '']])

    def test_calc_memory_record(self):
        check = NpuOpMemCalculator({}, CONFIG)

        npu_op_mem_dto_1 = NpuOpMemDto()
        npu_op_mem_dto_1.operator = '1'
        npu_op_mem_dto_1.addr = 100
        npu_op_mem_dto_1.size = 1
        npu_op_mem_dto_1.timestamp = 111
        npu_op_mem_dto_1.thread_id = 312
        npu_op_mem_dto_1.total_allocate_memory = 0
        npu_op_mem_dto_1.total_reserve_memory = 1
        npu_op_mem_dto_1.level = 10000
        npu_op_mem_dto_1.type = 6
        npu_op_mem_dto_1.device_type = "NPU:0"

        npu_op_mem_dto_2 = NpuOpMemDto()
        npu_op_mem_dto_2.operator = '1'
        npu_op_mem_dto_2.addr = 100
        npu_op_mem_dto_2.size = -1
        npu_op_mem_dto_2.timestamp = 112
        npu_op_mem_dto_2.thread_id = 312
        npu_op_mem_dto_2.total_allocate_memory = 0
        npu_op_mem_dto_2.total_reserve_memory = 1
        npu_op_mem_dto_2.level = 10000
        npu_op_mem_dto_2.type = 6
        npu_op_mem_dto_2.device_type = "NPU:0"

        setattr(check, "_op_data", [npu_op_mem_dto_1, npu_op_mem_dto_2])
        setattr(check, "_memory_record", [[]])
        check._calc_memory_record()
        self.assertEqual(check._memory_record, [['GE', 111, 1, 0, "NPU:0"], ['GE', 112, 1, 0, "NPU:0"]])

    def test__judge_should_calculate_should_return_true_when_no_calculated_table_exist_in_task_memory_db(self):
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=['test', 'test']):
            check = NpuOpMemCalculator({}, CONFIG)
            self.assertTrue(check._judge_should_calculate())

    def test__judge_should_calculate_should_return_false_when_calculated_table_exist_in_task_memory_db(self):
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=['test', 'test']):
            check = NpuOpMemCalculator({}, CONFIG)
            self.assertFalse(check._judge_should_calculate())

    def test__judge_should_calculate_should_return_false_when_task_memory_db_not_exist(self):
        with mock.patch("os.path.exists", return_value=False):
            check = NpuOpMemCalculator({}, CONFIG)
            self.assertFalse(check._judge_should_calculate())
