#!/usr/bin/env python
# coding=utf-8
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
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

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.cann.cann_calculator import CANNCalculator

NAMESPACE = 'mscalculate.cann.cann_calculator'


class TestClusterLinkCalculate(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.CannCalculatorScene.is_cpp_enable', return_value=False), \
                mock.patch(NAMESPACE + '.RTAddInfoCenter'), \
                mock.patch(NAMESPACE + '.CANNCalculator.calculate') as calculate, \
                mock.patch(NAMESPACE + '.CANNCalculator.save') as save, \
                mock.patch(NAMESPACE + '.dump_cann_trace') as dump_cann_trace:
            CANNCalculator({}, CONFIG).ms_run()
        calculate.assert_called_once_with()
        save.assert_called_once_with()
        dump_cann_trace.assert_not_called()

    def test_ms_run_should_use_native_parser_when_enabled(self):
        with mock.patch(NAMESPACE + '.CannCalculatorScene.is_cpp_enable', return_value=True), \
                mock.patch(NAMESPACE + '.CANNCalculator.calculate') as calculate, \
                mock.patch(NAMESPACE + '.CANNCalculator.save') as save, \
                mock.patch(NAMESPACE + '.dump_cann_trace') as dump_cann_trace:
            CANNCalculator({}, CONFIG).ms_run()
        dump_cann_trace.assert_called_once_with(CONFIG['result_dir'])
        calculate.assert_not_called()
        save.assert_not_called()

    def test_calculate(self):
        InfoConfReader()._info_json = {'pid': '0'}
        with mock.patch('mscalculate.cann.cann_event_generator.CANNEventGenerator.run'), \
                mock.patch(NAMESPACE + '.CANNCalculator.save'):
            check = CANNCalculator({}, CONFIG)
            check.thread_set = {1, }
            check.calculate()

    def test_save(self):
        check = CANNCalculator({}, CONFIG)
        check.save()


if __name__ == '__main__':
    unittest.main()
