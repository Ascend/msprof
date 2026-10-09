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
from unittest import mock

from msparser.step_trace.ts_binary_data_reader.task_flip_reader import TaskFlipReader
from common_func.info_conf_reader import InfoConfReader


class TestTaskFlipReader(unittest.TestCase):

    def test_read_binary_data_should_return_tasks_len_4_when_decode(self) -> None:
        data = [1, 14, 1, 0, 111111111000, 2, 1, 0, 1, *(0,) * 16]
        struct_data = struct.pack("=BBHLQHHHH16B", *data)
        InfoConfReader()._info_json = {
            "DeviceInfo": [{"hwts_frequency": 100}]
        }

        task_flip_reader = TaskFlipReader()
        task_flip_reader.read_binary_data(struct_data)
        self.assertEqual(task_flip_reader.table_name, 'DeviceTaskFlip')
        self.assertEqual(len(task_flip_reader.data), 1)
        self.assertEqual(len(task_flip_reader.data[0]), 4)
