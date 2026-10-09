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
import struct
import unittest

from msparser.step_trace.ts_binary_data_reader.block_num_reader import BlockNumReader


class TestBlockNumReader(unittest.TestCase):
    def test_read_binary_data_should_get_block_num_when_binary_data_parsed(self):
        # stream_id, task_id, timestamp, block_num
        expect_res = [(123456789, 1, 1, 8)]

        file_data = struct.pack("=BBH4BQHHL2Q", 1, 15, 32, 0, 0, 0, 0, 123456789, 1, 1, 8, 0, 0)

        block_num_reader = BlockNumReader()
        block_num_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, block_num_reader.data)
