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
from unittest import mock

import pytest

from common_func.utils import Utils


class TestUtils(unittest.TestCase):

    def test_is_valid_num(self):
        res = Utils.is_valid_num(None)
        self.assertEqual(res, 0)

    def test_percentile(self):
        data = [1, 2, 3, 4, 5, 6, 7, 8]
        ret = Utils.percentile(data, 0.33)
        self.assertEqual(3, ret)

    def test_is_step_scene_should_return_false_when_not_table_in_db(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=False):
            self.assertFalse(Utils.is_step_scene('./'))

    def test_is_step_scene_should_return_true_when_step_trace_data_has_value_other_than_4294967295(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_one_data', return_value=(1, )), \
                mock.patch('msmodel.interface.base_model.BaseModel.finalize'):
            self.assertTrue(Utils.is_step_scene('./'))

    def test_generator_to_list_should_return_empty_list_when_input_None_throws_exception(self):
        self.assertEqual([], Utils.generator_to_list(None))
