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
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import unittest

from mscalculate.cann.event import Event
from mscalculate.cann.event_queue import EventQueue


class TestEventQueue(unittest.TestCase):
    def test_malloc_new_size_should_malloc_200000_size_when_space_is_not_enough(self):
        q = EventQueue(1)
        q.malloc_new_size(200000)
        self.assertEqual(len(q.queue), 200000)

    def test_add_should_add_error_when_q_is_locked(self):
        q = EventQueue(1, init_len=1)
        q.lock()
        event = Event(1, 1, 1, 1, "")
        q.add(event)
        self.assertEqual(q.size, 0)

    def test_add_should_add_1_when_q_is_empty(self):
        q = EventQueue(1, init_len=1)
        event = Event(1, 1, 1, 1, "")
        q.add(event)
        self.assertEqual(q.size, 1)

    def test_add_should_add_2_and_re_malloc_when_q_is_not_enough(self):
        q = EventQueue(1, init_len=1)
        event1 = Event(1, 1, 1, 1, "")
        event2 = Event(1, 1, 1, 1, "")
        q.add(event1)
        q.add(event2)
        self.assertEqual(q.size, 2)

    def test_pop_should_return_invalid_when_q_is_not_locked(self):
        q = EventQueue(1, init_len=1)
        event = q.pop()
        self.assertTrue(event.is_invalid())

    def test_pop_should_return_invalid_when_q_is_empty(self):
        q = EventQueue(1, init_len=1)
        q.lock()
        event = q.pop()
        self.assertTrue(event.is_invalid())

    def test_pop_should_return_valid_when_q_is_not_empty(self):
        q = EventQueue(1, init_len=1)
        event1 = Event(1, 1, 1, 1, "")
        q.add(event1)
        q.lock()
        event = q.pop()
        self.assertEqual(event.thread_id, 1)

    def test_top_should_return_invalid_when_q_is_not_locked(self):
        q = EventQueue(1, init_len=1)
        event = q.top()
        self.assertTrue(event.is_invalid())

    def test_top_should_return_invalid_when_q_is_empty(self):
        q = EventQueue(1, init_len=1)
        q.lock()
        event = q.top()
        self.assertTrue(event.is_invalid())

    def test_top_should_return_valid_when_q_is_not_empty(self):
        q = EventQueue(1, init_len=1)
        event1 = Event(1, 1, 1, 1, "")
        q.add(event1)
        q.lock()
        event = q.top()
        self.assertEqual(event.thread_id, 1)

    def test_empty_should_return_ture_when_q_empty(self):
        q = EventQueue(1, init_len=1)
        self.assertTrue(q.empty())

    def test_empty_should_return_false_when_q_not_empty(self):
        q = EventQueue(1, init_len=1)
        event1 = Event(1, 1, 1, 1, "")
        q.add(event1)
        self.assertFalse(q.empty())
