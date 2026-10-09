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
from unittest import mock

from constant.constant import CONFIG
from mscalculate.factory.calculator_factory import CalculatorFactory

NAMESPACE = 'mscalculate.factory.calculator_factory'
CONFIG_NAMESPACE = 'common_func.config_mgr'


class TestCalculatorFactory(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.ConfigDataParsers.get_parsers'), \
                mock.patch(CONFIG_NAMESPACE + '.ConfigMgr.read_sample_config',
                           return_value={"ai_core_profiling_mode": ""}):
            check = CalculatorFactory({}, CONFIG, '310')
            check.run()
