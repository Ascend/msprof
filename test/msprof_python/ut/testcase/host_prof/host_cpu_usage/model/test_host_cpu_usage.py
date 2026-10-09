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

from common_func.info_conf_reader import InfoConfReader
from host_prof.host_cpu_usage.model.host_cpu_usage import HostCpuUsage
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_cpu_usage.model.host_cpu_usage'


class TsetHostCpuUsage(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_insert_cpu_info_data(self):
        cpu_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_cpu_info_data(cpu_info)

    def test_insert_process_usage_data(self):
        process_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_process_usage_data(process_usage_info)

    def test_insert_cpu_usage_data(self):
        cpu_usage_info = {}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HostCpuUsage(self.result_dir)
            check.insert_cpu_usage_data(cpu_usage_info)

    def test_has_cpu_usage_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostCpuUsage(self.result_dir)
            check.has_cpu_usage_data()

    def test_has_cpu_info_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist'):
            check = HostCpuUsage(self.result_dir)
            check.has_cpu_info_data()

    def test_get_cpu_list(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data'):
            check = HostCpuUsage(self.result_dir)
            check.get_cpu_list()

    def test_get_cpu_info_data(self):
        create_sql = "create table if not exists CpuInfo (cpu_num INTEGER,clk_jiffies INTEGER)"
        insert_sql = "insert into {} values (?,?)".format('CpuInfo')
        data = ((20, 100),)
        db_manager = DBManager()
        res = db_manager.create_table("host_cpu_usage.db", create_sql, insert_sql, data)
        check = HostCpuUsage(self.result_dir)
        check.cur = res[1]
        result_1 = check.get_cpu_info_data()
        self.assertEqual(result_1, {"cpu_num": 20})
        res[1].execute("delete from CpuInfo where cpu_num=20")
        result_2 = check.get_cpu_info_data()
        self.assertEqual(result_2, {"cpu_num": 0})
        res[1].execute("drop table CpuInfo")
        db_manager.destroy(res)

    def test_get_cpu_usage_data(self):
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        InfoConfReader()._local_time_offset = 1698809492.41
        InfoConfReader()._host_local_time_offset = 1698809492.41
        create_sql = "create table if not exists CpuUsage (start_time numeric,end_time numeric,cpu_no text,usage REAL)"
        insert_sql = "insert into {} values (?,?,?,?)".format('CpuUsage')
        data = ((10000, 20000, 20, 100),)
        db_manager = DBManager()
        res = db_manager.create_table("host_cpu_usage.db", create_sql, insert_sql, data)
        per_cpu_usage = res[1].execute("SELECT * FROM CpuUsage order by CAST(cpu_no AS DECIMAL)")
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=per_cpu_usage):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_cpu_usage_data()
        self.assertEqual(result_1, [['CPU 20', '1698809502.410', {'Usage(%)': 100.0}]])
        res[1].execute("drop table CpuUsage")
        db_manager.destroy(res)

    def test_get_num_of_used_cpus_normal1(self):
        cpu_used_data = [(0, 1), (1, 28), (2, 33), (3, 0.000001), (4, 0)]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=cpu_used_data):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_num_of_used_cpus()
        self.assertEqual(result_1[0], 5)
        self.assertEqual(result_1[1], 4)

    def test_get_num_of_used_cpus_normal2(self):
        cpu_used_data = [(0, 0), (1, 0), (2, 0), (3, 0), (4, 0)]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=cpu_used_data):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_num_of_used_cpus()
        self.assertEqual(result_1[0], 5)
        self.assertEqual(result_1[1], 0)

    def test_get_num_of_used_cpus_big_data(self):
        cpu_used_data = [(i, i % 2) for i in range(1000)]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=cpu_used_data):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_num_of_used_cpus()
        self.assertEqual(result_1[0], 1000)
        self.assertEqual(result_1[1], 500)

    def test_get_num_of_used_cpus_no_cpu_data(self):
        cpu_used_data = []
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=cpu_used_data):
            check = HostCpuUsage(self.result_dir)
            result_1 = check.get_num_of_used_cpus()
        self.assertEqual(result_1[0], 0)
        self.assertEqual(result_1[1], 0)

if __name__ == '__main__':
    unittest.main()
