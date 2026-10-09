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

from msparser.step_trace.ts_binary_data_reader.ts_memcpy_reader import TsMemcpyReader


class TestTsMemcpyReader(unittest.TestCase):
    def test_read_binary_data(self):
        # timestamp, stream_id, task_id, tag_state
        expect_res = [(101612167908, 1, 1, 0)]

        file_data = struct.pack("=BBH4BQHHBBHQQ", 0, 11, 32, 0, 0, 0, 0, 101612167908, 1, 1, 0, 0, 0, 0, 0)

        ts_memcpy_reader = TsMemcpyReader()
        ts_memcpy_reader.read_binary_data(file_data)
        self.assertEqual(expect_res, ts_memcpy_reader.data)
