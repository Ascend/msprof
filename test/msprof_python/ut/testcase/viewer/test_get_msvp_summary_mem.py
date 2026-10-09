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
import sqlite3
import unittest
from unittest import mock

from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBOpen
from viewer.get_msvp_summary_mem import get_sys_mem_data, get_process_mem_data

NAMESPACE = 'viewer.get_msvp_summary_mem'
configs = {'headers': ["Memory Total", "Memory Free", "Buffers", "Cached", "Share Memory", "Commit Limit",
                       "Committed AS", "Huge Pages Total(pages)", "Huge Pages Free(pages)"],
           "db": "memory_0.db"}


class TestMsvpSummaryMem(unittest.TestCase):
    def test_get_sys_mem_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.OperationalError):
            res = get_sys_mem_data('', 'sysmem', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_sys_mem_data('', 'sysmem', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_sys_mem_data('', 'sysmem', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_sys_mem_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS sysmem" \
                     " (timestamp, memtotal, memfree, buffers, cached, shmem, commitlimit, committed_as, " \
                     "hugepages_total, hugepages_free, unit)"
        data = ((1466711807878, 7952780, 5279712, 0, 274100, 274100, 2983108, 740292, 970, 970, "kB"),)
        with DBOpen("memory_0.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("sysmem", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_sys_mem_data('', 'sysmem', configs)
            self.assertEqual(res[2], 1)

    def test_get_process_mem_data_error(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_process_mem_data('', 'pidmem', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.OperationalError):
            res = get_process_mem_data('', 'pidmem', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_process_mem_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS pidmem" \
                     " (timestamp, name, size, resident, shared, pid)"
        data = ((1466721174492, "init", 989, 246, 220, 1),)
        with DBOpen("memory_0.db") as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("pidmem", data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_open.db_conn, db_open.db_curs)), \
                 mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
                res = get_process_mem_data('', 'pidmem', configs)
            self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
