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
from sqlite.db_manager import DBManager
from viewer.cpu_usage_report import get_sys_cpu_usage_data, get_process_cpu_usage

NAMESPACE = 'viewer.cpu_usage_report'
configs = {'handler': '_get_cpu_usage_data',
           'headers': ['Cpu Type', 'User(%)', 'Sys(%)', 'IoWait(%)', 'Irq(%)', 'Soft(%)', 'Idle(%)'],
           'table': 'SysCpuUsage', 'unused_cols': []}


class TestCPUUsage(unittest.TestCase):
    def test_get_sys_cpu_usage_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.OperationalError):
            res = get_sys_cpu_usage_data('', 'SysCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_sys_cpu_usage_data('', 'SysCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_sys_cpu_usage_data('', 'SysCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_sys_cpu_usage_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS SysCpuUsage" \
                     " (timestamp, cpun, user, nice, sys, idle, iowait, irq, soft, steal, guest, gnice, cputype)"
        data = ((1466711456523, "cpu", 1286, 0, 2151, 1168096, 0, 0, 4, 0, 0, 0, None),
                (1466711456523, "cpu0", 158, 0, 1008, 144309, 0, 0, 4, 0, 0, 0, "ctrlcpu"))
        insert_sql = "insert into {0} values ({value})".format(
            "SysCpuUsage", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("cpu_usage.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_sys_cpu_usage_data('', 'SysCpuUsage', configs)
        self.assertEqual(res[2], 1)
        test_sql = db_manager.connect_db('cpu_usage.db')
        (test_sql[1]).execute("drop Table SysCpuUsage")
        db_manager.destroy(test_sql)

    def test_get_process_cpu_usage_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.OperationalError):
            res = get_process_cpu_usage('', 'ProCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_process_cpu_usage('', 'ProCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_process_cpu_usage('', 'ProCpuUsage', configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_process_cpu_usage_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ProCpuUsage" \
                     " (pid, process_name, utime, stime, cutime, cstime, timestamp, sys_usage)"
        data = ((1, "init", 0, 345, 279, 263, 1466721323815, 1171545),)
        insert_sql = "insert into {0} values ({value})".format(
            "ProCpuUsage", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("cpu_usage.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_process_cpu_usage('', 'ProCpuUsage', configs)
        self.assertEqual(res[2], 1)
        test_sql = db_manager.connect_db('cpu_usage.db')
        (test_sql[1]).execute("drop Table ProCpuUsage")
        db_manager.destroy(test_sql)


if __name__ == '__main__':
    unittest.main()
