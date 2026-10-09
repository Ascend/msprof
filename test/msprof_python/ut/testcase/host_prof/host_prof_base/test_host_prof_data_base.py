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

from common_func.constant import Constant
from host_prof.host_prof_base.host_prof_data_base import HostProfDataBase
from sqlite.db_manager import DBManager

NAMESPACE = 'host_prof.host_prof_base.host_prof_data_base'


class TsetHostProfDataBase(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_init(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                           return_value=(None, None)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.init()
        self.assertEqual(result, False)
        db_manager = DBManager()
        res = db_manager.create_table('host_cpu_usage.db')
        res[1].execute('create table if not exists CpuInfo(id integer, name integer)')
        sql = "create table if not exists CpuUsage(id integer, name integer)"
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                           return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                           return_value=sql):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo', 'CpuUsage'])
            result = check.init()
        self.assertEqual(result, True)
        res[1].execute("drop table CpuInfo")
        res[1].execute("drop table CpuUsage")
        db_manager.destroy(res)

    def test_check_db(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                        return_value=(None, None)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.check_db()
        self.assertEqual(result, False)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db',
                        return_value=(1, 1)):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result = check.check_db()
        self.assertEqual(result, True)

    def test_insert_single_data(self):
        data = [191424.758430371, 191424.778907458, '0.000000']
        with mock.patch(NAMESPACE + '.HostProfDataBase.flush_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.exception'):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            check.cache_data = list(range(10001))
            check.insert_single_data(data)
        self.assertEqual(check.cache_data, [])

    def test_recommend_value_for_data_list_normal1(self):
        data = [(0,), (1,), (3,), (4,), (5,), (6,)]
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 6 / Constant.RATIO_FOR_BEST_PERFORMANCE)

    def test_recommend_value_for_data_list_normal2(self):
        data = [(0,), (1,), (2,), (2,), (2,), (2,)]
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 2 / Constant.RATIO_FOR_BEST_PERFORMANCE)

    def test_recommend_value_for_data_list_big_data1(self):
        data = [(i, ) for i in range(100)]
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 98 / Constant.RATIO_FOR_BEST_PERFORMANCE)

    def test_recommend_value_for_data_list_big_data2(self):
        data = [(i, ) for i in range(1000)]
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 980 / Constant.RATIO_FOR_BEST_PERFORMANCE)

    def test_recommend_value_for_data_list_all_zero(self):
        data = [(0, ) for i in range(100)]
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 0)

    def test_recommend_value_for_data_list_no_data(self):
        data = []
        check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
        ret = check.recommend_value_for_data_list(data)
        self.assertEqual(ret, 0)

    def test_recommend_value_normal(self):
        value_data = [(0,), (0.1,), (1,), (1000,)]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=value_data), \
             mock.patch(NAMESPACE + '.HostProfDataBase.recommend_value_for_data_list', return_value=0):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result_1 = check.get_recommend_value('value', 'table')
        self.assertEqual(result_1[0], 1000)

    def test_recommend_value_no_data(self):
        value_data = []
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=value_data), \
             mock.patch(NAMESPACE + '.HostProfDataBase.recommend_value_for_data_list', return_value=0):
            check = HostProfDataBase(self.result_dir, 'host_cpu_usage.db', ['CpuInfo'])
            result_1 = check.get_recommend_value('value', 'table')
        self.assertEqual(result_1[0], Constant.NA)

if __name__ == '__main__':
    unittest.main()
