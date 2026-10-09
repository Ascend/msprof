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

from msparser.step_trace.helper.model_with_q_parser import ModelWithQParser


class TestModelWithQParser(unittest.TestCase):
    def test_read_binary_data(self):
        # index id, model id, timestamp, tag_id, event_id

        file_data = struct.pack("=HHIQQIHHQ24B", 1, 1, 1, 101612167908, 1, 1, 3, 1, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

        data_parser = ModelWithQParser()
        data_parser.read_binary_data(file_data)
        self.assertEqual(data_parser._data[0][2], 101612167908)
