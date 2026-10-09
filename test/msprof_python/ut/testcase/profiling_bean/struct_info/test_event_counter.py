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

from profiling_bean.struct_info.event_counter import CounterInfo, AiCoreTaskInfo

NAMESPACE = 'profiling_bean.struct_info.event_counter'


class TestCounterInfo(unittest.TestCase):
    def test_init(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0)
        self.assertEqual(test._overflow, 0)

    def test_overflow(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow
        self.assertEqual(test, 0)

    def test_overflow_cycle(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow_cycle
        self.assertEqual(test, 0)

    def test_task_cyc(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).task_cyc
        self.assertEqual(test, 0)

    def test_time_stamp(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).time_stamp
        self.assertEqual(test, 0)

    def test_block(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).block
        self.assertEqual(test, 0)

    def test_overflow_cycle_2(self):
        test = CounterInfo(0, 0, 0, 0, 0, 0).overflow_cycle
        self.assertEqual(test, 0)


class TestAiCoreTaskInfo(unittest.TestCase):
    def test_init(self):
        test = AiCoreTaskInfo([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
        self.assertEqual(test.task_type, 0)


if __name__ == '__main__':
    unittest.main()
