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

from msmodel.biu_perf.biu_perf_model import BiuPerfModel
from sqlite.db_manager import DBManager

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.biu_perf.biu_perf_model'


class TestBiuPerfModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            BiuPerfModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.BiuPerfModel.insert_data_to_db'):
            self.assertEqual(BiuPerfModel('test', ['test']).flush('BiuCycles', []), None)

    def test_get_all_data(self):
        db_manager = DBManager()
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.conn, check.cur = db_manager.create_table('test.db')
            check.get_all_data('test')
        db_manager.destroy((check.conn, check.cur))

    def test_get_biu_flow_process(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_flow_process()

    def test_get_biu_flow_thread(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_flow_thread()

    def test_get_biu_cycles_process(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_cycles_process()

    def test_get_biu_cycles_thread(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_cycles_thread()

    def test_get_biu_flow_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_flow_data()

    def test_get_biu_cycles_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_valyue=[]):
            check = BiuPerfModel('test', ['test'])
            check.get_biu_cycles_data()


if __name__ == '__main__':
    unittest.main()
