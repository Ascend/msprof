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
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from common_func.decode_tool.byte_interpreter import ByteInterpreter


class TestTraceViewer(unittest.TestCase):
    def test_check(self) -> None:
        try:
            byte_interpreter = ByteInterpreter(3, [10, 12])
        except ValueError:
            pass

    def test_decode_byte(self) -> None:
        byte_interpreter = ByteInterpreter(3, [12, 12])
        self.assertEqual(tuple(byte_interpreter.decode_byte(b'\x01\x00\x10')), (1, 256))
