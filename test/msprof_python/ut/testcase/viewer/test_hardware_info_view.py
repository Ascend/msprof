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
import json
import unittest
from unittest import mock

from common_func.ms_constant.str_constant import StrConstant
from sqlite.db_manager import DBManager
from viewer.hardware_info_view import get_llc_train_summary

NAMESPACE = 'viewer.hardware_info_view'
configs = {StrConstant.LLC_PROF: StrConstant.LLC_CAPACITY_ITEM}


class TestHardwareInfoView(unittest.TestCase):
    def test_get_llc_train_summary_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + StrConstant.LLC_METRICS_TABLE + \
                     " (device_id, l3tid, timestamp, hitrate, throughput)"
        data = ((0, 0, 81951266894476, 0, 0),
                (0, 0, 81951355247476, 0.661267227823513, 9564.52737881))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(StrConstant.LLC_METRICS_TABLE, data)

        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_llc_train_summary('', configs, 0)
            test_sql = db_manager.connect_db("llc.db")
        (test_sql[1]).execute("drop Table {}".format(StrConstant.LLC_METRICS_TABLE))
        db_manager.destroy(test_sql)
        self.assertEqual(len(json.loads(res)), 3)

    def test_get_llc_train_summary_3(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE IF NOT EXISTS " + StrConstant.LLC_METRICS_TABLE + \
                     " (device_id, l3tid, timestamp, hitrate, throughput)"
        data = ((0, None, 81951266894476, 0, 0.1),)
        insert_sql = db_manager.insert_sql(StrConstant.LLC_METRICS_TABLE, data)
        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_llc_train_summary('', configs, 0)
        db_manager.destroy(test_sql)
        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql)
        (test_sql[1]).execute("drop Table if exists {}".format(StrConstant.LLC_METRICS_TABLE))
        db_manager.destroy(test_sql)
        self.assertEqual(len(json.loads(res)), 2)

    def test_get_llc_train_summary_should_close_db_when_llc_db_is_not_existed(self):
        error_info = "The LLC Metric Data doesn't exist."
        db_manager = DBManager()
        test_sql = db_manager.create_table("llc.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
            mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_llc_train_summary('', configs, 0)
        db_manager.destroy(test_sql)
        self.assertIn(error_info, res)

    def test_get_llc_train_summary_should_close_db_when_llc_data_is_not_existed(self):
        error_info = "The LLC Data doesn't exist."
        db_manager = DBManager()
        create_sql = "CREATE TABLE IF NOT EXISTS " + StrConstant.LLC_METRICS_TABLE + \
                     " (device_id, l3tId, timestamp, hitrate, throughput)"
        data = ((0, 0, 81951266894476, 0, 0.1),)
        insert_sql = db_manager.insert_sql(StrConstant.LLC_METRICS_TABLE, data)
        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            res = get_llc_train_summary('', configs, 0)
        db_manager.destroy(test_sql)
        self.assertIn(error_info, res)


if __name__ == '__main__':
    unittest.main()
