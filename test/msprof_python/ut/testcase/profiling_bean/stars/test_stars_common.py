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

from msmodel.sqe_type_map import SqeType
from msparser.compact_info.stream_expand_spec_reader import StreamExpandSpecReader
from profiling_bean.stars.stars_common import StarsCommon


class TestStarsCommon(unittest.TestCase):

    def test_should_stream_change_low_12_bits_when_stream_bit13_is_one(self):
        stars_common = StarsCommon(8, 8195, 0)
        self.assertEqual(8, stars_common.stream_id)

    def test_should_return_stream_when_stream_bit13_is_not_one(self):
        stars_common = StarsCommon(8, 3, 0)
        self.assertEqual(3, stars_common.stream_id)

    def test_should_return_stream_low_12_bits_when_stream_bit12_is_one(self):
        stars_common = StarsCommon(8, 12291, 0)
        self.assertEqual(3, stars_common.stream_id)

    def test_should_return_task_when_stream_bit13_is_one(self):
        stars_common = StarsCommon(8, 8195, 0)
        self.assertEqual(3, stars_common.task_id)

    def test_should_task_change_low_12_bits_when_stream_bit12_and_bit13_is_not_one(self):
        stars_common = StarsCommon(8, 3, 0)
        self.assertEqual(8, stars_common.task_id)

    def test_should_return_task_change_high_3_bits_when_stream_bit12_is_one(self):
        stars_common = StarsCommon(8, 20483, 0)
        self.assertEqual(16392, stars_common.task_id)

    def test_expanded_placeholder_sqe_returns_stream_low_15_bits(self):
        real_reader = StreamExpandSpecReader()
        real_reader._stream_expand_spec = 1
        stars_common = StarsCommon(90000, 12345, SqeType.StarsSqeType.PLACE_HOLDER_SQE)
        self.assertEqual(12345, stars_common.stream_id)
        self.assertEqual(90000, stars_common.task_id)
        real_reader._stream_expand_spec = 0

    def test_expanded_bit15_set_returns_task_low_15_bits(self):
        real_reader = StreamExpandSpecReader()
        real_reader._stream_expand_spec = 1
        stars_common = StarsCommon(32769, 45678, SqeType.StarsSqeType.AI_CORE)
        self.assertEqual(1, stars_common.stream_id)
        self.assertEqual(45678, stars_common.task_id)
        real_reader._stream_expand_spec = 0

    def test_expanded_no_bit15_returns_stream_low_15_bits(self):
        real_reader = StreamExpandSpecReader()
        real_reader._stream_expand_spec = 1
        stars_common = StarsCommon(20000, 55555, SqeType.StarsSqeType.AI_CORE)
        self.assertEqual(20000, stars_common.stream_id)
        self.assertEqual(22787, stars_common.task_id)
        real_reader._stream_expand_spec = 0