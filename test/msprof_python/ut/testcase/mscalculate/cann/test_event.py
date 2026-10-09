#!/usr/bin/env python
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import unittest

from common_func.constant import Constant
from mscalculate.cann.event import Event


class TestEventQueue(unittest.TestCase):
    def test_lt_event(self):
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        e2 = Event(Constant.ACL_LEVEL, 1, 270, 280, "acl_func")
        self.assertTrue(e1 < e2)

    def test_hash_event(self):
        # 重新设置内置ID，避免其余Event创建影响编号
        Event._ID = 0
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertEqual(hash(e1), 0)

    def test_eq_event_when_no_attr_id_should_return_False(self):
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertFalse(e1 is None)

    def test_eq_event_when_event_is_same_should_return_True(self):
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertTrue(e1 == e1)

    def test_str_event(self):
        # 重新设置内置ID，避免其余Event创建影响编号
        Event._ID = 0
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertEqual(str(e1), "launch-0")

    def test_is_invalid_should_return_True_when_is_invalid(self):
        e = Event.invalid_event()
        self.assertTrue(e.is_invalid())

    def test_is_invalid_should_return_False_when_is_valid(self):
        e = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertFalse(e.is_invalid())

    def test_is_additional_should_return_False_when_timestamp_not_equal_bound(self):
        e = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertFalse(e.is_additional())

    def test_add_additional_should_return_None(self):
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        e2 = Event(Constant.ACL_LEVEL, 1, 270, 280, "acl_func")
        e1.add_additional_record(e2)

    def test_to_string_should_return_correct_string(self):
        e1 = Event(Constant.NODE_LEVEL, 1, 258, 300, "launch")
        self.assertEqual(e1.to_string(), "level: 3, thread_id: 1, timestamp: 258, type: launch")
