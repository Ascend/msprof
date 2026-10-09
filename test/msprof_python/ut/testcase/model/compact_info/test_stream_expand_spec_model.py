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

from common_func.db_name_constant import DBNameConstant
from msmodel.compact_info.stream_expand_spec_model import StreamExpandSpecModel


class TestStreamExpandSpecModel(unittest.TestCase):

    def test_construct(self):
        check = StreamExpandSpecModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_STREAM_EXPAND_SPEC)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_STREAM_EXPAND_SPEC])

    def test_flush(self):
        with mock.patch("msmodel.compact_info.stream_expand_spec_model" +
                        '.StreamExpandSpecModel.insert_data_to_db'):
            check = StreamExpandSpecModel('test')
            check.flush([[1]])