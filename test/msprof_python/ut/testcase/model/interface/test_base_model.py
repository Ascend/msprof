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

from msmodel.interface.base_model import BaseModel

NAMESPACE = 'msmodel.interface.base_model'


class TestBaseModel(unittest.TestCase):

    def test_init(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None),\
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(None, None)):
            check = BaseModel('test', 'test', 'test')
            ret = check.init()
        self.assertFalse(ret)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None),\
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)),\
                mock.patch(NAMESPACE + '.BaseModel.create_table'):
            check = BaseModel('test', 'test', 'test')
            ret = check.init()
        self.assertTrue(ret)

    def test_check_db(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)):
            check = BaseModel('test', 'test', 'test')
            ret = check.check_db()
        self.assertTrue(ret)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            check = BaseModel('test', 'test', 'test')
            ret = check.check_db()
        self.assertFalse(ret)

    def test_finalize(self):
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = BaseModel('test', 'test', 'test')
            check.finalize()

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = BaseModel('test', 'test', 'test')
            check.table_list = ['test']
            check.create_table()
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False),\
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=None),\
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = BaseModel('test', 'test', 'test')
            check.table_list = ['test']
            check.create_table()

    def test_insert_data_to_db(self):
        with mock.patch(NAMESPACE + '.DBManager.executemany_sql', return_value=False):
            check = BaseModel('test', 'test', 'test')
            check.conn = True
            check.insert_data_to_db('test', 'test')

    def test_get_all_data0(self):
        table_name = 'test'
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1,)):
            check = BaseModel('test', 'test', 'test')
            ret = check.get_all_data(table_name)
        self.assertEqual(ret, (1,))

    def test_get_all_data1(self):
        table_name = 'test'
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = BaseModel('test', 'test', 'test')
            ret = check.get_all_data(table_name)
        self.assertEqual(ret, [])

    def test_contextor(self):
        check = BaseModel('test', 'test', 'test')
        with check:
            pass

    def test_drop_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            check = BaseModel('test', 'test', 'test')
            ret = check.drop_table('test')
