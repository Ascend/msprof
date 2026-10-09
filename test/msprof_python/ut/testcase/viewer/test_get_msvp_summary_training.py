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

from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from viewer.get_msvp_summary_training import get_hbm_summary
from viewer.get_msvp_summary_training import get_hbm_summary_data

NAMESPACE = 'viewer.get_msvp_summary_training'


class TestMsvpSummaryMem(unittest.TestCase):
    def test_get_hbm_summary_1(self):
        with DBOpen(DBNameConstant.DB_HBM) as db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
                res = get_hbm_summary('', 0)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_hbm_summary('', 0)
            self.assertEqual(len(json.loads(res)), 2)

            new_curs = db_open.db_conn.cursor()
            new_curs.close()
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, new_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_hbm_summary('', 0)
            self.assertEqual(len(json.loads(res)), 2)

    def test_get_hbm_summary_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS HBMbwData" \
                     " (device_id, timestamp, bandwidth, hbmid, event_type)"
        data = ((0, 176167461566990, 0.157647268065233, 0, "read"),)
        with DBOpen(DBNameConstant.DB_HBM) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_HBM_BW, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_hbm_summary('', 0)
            self.assertEqual(len(json.loads(res)), 3)

    def test_get_hbm_summary_data_1(self):
        with DBOpen(DBNameConstant.DB_HBM) as db_open:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
                res = get_hbm_summary_data('', 0)
            self.assertEqual(len(json.loads(res)), 2)

            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
                res = get_hbm_summary_data('', 0)
            self.assertEqual(len(json.loads(res)), 2)

            new_curs = db_open.db_conn.cursor()
            new_curs.close()
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, new_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_hbm_summary_data('', 0)
            self.assertEqual([], res)

    def test_get_hbm_summary_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS HBMbwData " \
                     "(device_id, timestamp, bandwidth, hbmid, event_type)"
        data = ((0, 176167461566990, 0.157647268065233, 0, "read"),)
        insert_sql = "insert into {0} values ({value})".format(
            "HBMbwData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("hbm.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_hbm_summary_data('', 0)
        self.assertEqual(len(res), 2)
        test_sql = db_manager.create_table("hbm.db")
        (test_sql[1]).execute("drop Table HBMbwData")
        db_manager.destroy(test_sql)


if __name__ == '__main__':
    unittest.main()
