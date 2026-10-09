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
from msmodel.compact_info.node_attr_info_model import NodeAttrInfoModel

NAMESPACE = 'msmodel.compact_info.node_attr_info_model'


class TestNodeAttrInfoModel(unittest.TestCase):
    def test_construct(self):
        check = NodeAttrInfoModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_NODE_ATTR_INFO)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_NODE_ATTR_INFO])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.NodeAttrInfoModel.insert_data_to_db'):
            check = NodeAttrInfoModel('test')
            check.flush(['node', 'node_attr_info', 3602526, 38202864071026,
                         'strides: 0; pads: 0; dilations: 0; groups: 1;data_formats: 0'])



