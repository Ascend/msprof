# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import sqlite3
import unittest
from unittest import mock

from msmodel.ccu_model import insert_ccu_data_to_db


class TestCcuModel(unittest.TestCase):
    def test_insert_should_report_commit_result_without_changing_base_model(self):
        conn = mock.Mock()
        conn.execute.return_value.fetchone.return_value = (0,)
        with mock.patch("msmodel.ccu_model.DBManager.executemany_sql", return_value=True):
            self.assertTrue(insert_ccu_data_to_db(conn, "CCUTable", [[1, 2]]))
        with mock.patch("msmodel.ccu_model.DBManager.executemany_sql", return_value=False):
            self.assertFalse(insert_ccu_data_to_db(conn, "CCUTable", [[1, 2]]))

    def test_insert_should_accept_empty_data_and_reject_missing_connection(self):
        self.assertFalse(insert_ccu_data_to_db(None, "CCUTable", []))
        self.assertFalse(insert_ccu_data_to_db(None, "CCUTable", [[1]]))
        conn = sqlite3.connect(":memory:")
        try:
            conn.execute("CREATE TABLE CCUTable (value NUMERIC)")
            self.assertTrue(insert_ccu_data_to_db(conn, "CCUTable", []))
            self.assertTrue(insert_ccu_data_to_db(conn, "CCUTable", [[7]]))
            self.assertFalse(insert_ccu_data_to_db(conn, "CCUTable", []))
            self.assertEqual([(7,)], conn.execute("SELECT * FROM CCUTable").fetchall())
        finally:
            conn.close()

    def test_insert_should_reject_retry_without_duplicating_rows(self):
        with sqlite3.connect(":memory:") as conn:
            conn.execute("CREATE TABLE CCUTable (value NUMERIC)")
            self.assertTrue(insert_ccu_data_to_db(conn, "CCUTable", [[7]]))
            self.assertFalse(insert_ccu_data_to_db(conn, "CCUTable", [[7]]))
            self.assertEqual([(7,)], conn.execute("SELECT * FROM CCUTable").fetchall())
