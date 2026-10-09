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
function: test imetrics
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from mscalculate.interface.imetrics import IMetrics


class TestIMetrics(unittest.TestCase):
    def test_get_division(self: any) -> None:
        divisor = 15
        dividend = 123
        res = 0.12195121951219512
        self.assertEqual(IMetrics.get_division(divisor, dividend), res)

    def test_get_mul(self: any) -> None:
        value1 = 5
        value2 = 10
        self.assertEqual(IMetrics.get_mul(value1, value2), 50.0)

