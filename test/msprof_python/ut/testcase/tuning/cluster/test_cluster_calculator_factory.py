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

from tuning.cluster.cluster_calculator_factory import SlowLinkCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowRankCalculatorFactory

NAMESPACE = 'tuning.cluster.cluster_calculator_factory'


class TestSlowRankCalculatorFactory(unittest.TestCase):
    def test_generate_calculator(self):
        op_info = {'hccl_name': {}}
        SlowRankCalculatorFactory(op_info)


class TestSlowLinkCalculatorFactory(unittest.TestCase):
    def test_generate_calculator(self):
        op_info = {'hccl_name': {1: {}}}
        SlowLinkCalculatorFactory(op_info)