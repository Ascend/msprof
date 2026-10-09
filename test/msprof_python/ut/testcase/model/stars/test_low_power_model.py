#!/usr/bin/python3
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from unittest import mock

from msmodel.stars.low_power_model import LowPowerModel
from msmodel.stars.low_power_model import LowPowerViewModel

NAMESPACE = 'msmodel.stars.low_power_model'


class TestLowPowerModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.LowPowerModel.insert_data_to_db'):
            check = LowPowerModel('test', 'test', [])
            check.flush([])


class TestLowPowerViewModel(unittest.TestCase):

    def test_get_timeline_data_should_return_data(self):
        with mock.patch('msmodel.interface.base_model.DBManager.judge_table_exist', return_value=True), \
                mock.patch('msmodel.interface.base_model.DBManager.fetch_all_data', return_value=[1]):
            check = LowPowerViewModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])
