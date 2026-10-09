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
function: test l2_cache_parser_model
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.l2_cache.l2_cache_parser_model import L2CacheParserModel

NAMESPACE = 'msmodel.l2_cache.l2_cache_parser_model'


class TestL2CacheParserModel(unittest.TestCase):
    def test_construct(self):
        check = L2CacheParserModel('test', [DBNameConstant.TABLE_L2CACHE_PARSE])
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_L2CACHE)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_L2CACHE_PARSE])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.L2CacheParserModel.insert_data_to_db'):
            check = L2CacheParserModel('test', [DBNameConstant.TABLE_L2CACHE_PARSE])
            check.flush(1)
