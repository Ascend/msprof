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
from unittest import mock

from msmodel.stars.acc_pmu_model import AccPmuModel

NAMESPACE = 'msmodel.stars.acc_pmu_model'


class TestAccPmuModel(unittest.TestCase):

    def test_insert_pmu_origin_data(self):
        with mock.patch(NAMESPACE + '.AccPmuModel.insert_data_to_db'):
            check = AccPmuModel('test', 'test', [])
            check.insert_pmu_origin_data([])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AccPmuModel.insert_pmu_origin_data'):
            check = AccPmuModel('test', 'test', [])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = AccPmuModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [1])

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = AccPmuModel('test', 'test', [])
            res = check.get_timeline_data()
        self.assertEqual(res, [])

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = AccPmuModel('test', 'test', [])
            res = check.get_summary_data()
        self.assertEqual(res, [1])

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = AccPmuModel('test', 'test', [])
            res = check.get_summary_data()
        self.assertEqual(res, [])
