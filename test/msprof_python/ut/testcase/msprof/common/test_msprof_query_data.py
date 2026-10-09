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

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_query_data import MsprofQueryData
from sqlite.db_manager import DBManager

NAMESPACE = 'common_func.msprof_query_data'


class TestMsprofQueryData(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_GE_INFO))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_GE_INFO))

    def test_get_job_iteration_info(self):
        create_sql_ge = "create table IF NOT EXISTS TaskInfo (model_id INT)"
        data_ge = ((1,),)
        insert_sql_ge = "insert into {0} values ({value})".format("TaskInfo",
                                                                  value="?," * (len(data_ge[0]) - 1) + "?")

        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT,step_start," \
                     "step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")

        db_manager = DBManager()
        res_ge = db_manager.create_table("ge_info.db", create_sql_ge, insert_sql_ge, data_ge)
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        res_ge[0].close()
        res[0].close()

        # res = db_manager.create_table("step_trace.db")
        key = MsprofQueryData(db_manager.db_path + "/../")
        result = key.get_job_iteration_info()
        self.assertEqual(result, [])

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
                with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                    with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                        key = MsprofQueryData('123')
                        result = key.get_job_iteration_info()
                    self.assertEqual(result, [])

                with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', side_effect=sqlite3.DatabaseError):
                    with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                        key = MsprofQueryData('123')
                        result = key.get_job_iteration_info()
                    self.assertEqual(result, [])

    def test_get_step_iteration_info_should_return_1_info_and_step_order_by_duration_when_query_step_export(self):
        create_sql = "create table IF NOT EXISTS StepTime (index_id INT,model_id INT,step_start," \
                     "step_end,iter_id INT default 1)"
        data = (
            (1, 4294967295, 10000, 10010, 1),
            (2, 4294967295, 20000, 20200, 2),
            (3, 4294967295, 30000, 31000, 3),
            (4, 4294967295, 40000, 40100, 4),
        )
        insert_sql = "insert into {0} values ({value})".format("StepTime",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        conn, curs = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        key = MsprofQueryData(db_manager.db_path + "/../")
        result = key.get_step_iteration_info()
        self.assertEqual([[4294967295, 4, '3,2,4,1']], result)
        db_manager.destroy([conn, curs])

    def test_update_top_iteration_info(self):
        msprof_query_data = MsprofQueryData('123')
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT,step_start," \
                     "step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60), (2, 1, 118562263866, 118571743612, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        res = _db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        result = msprof_query_data._update_top_iteration_info([], {1}, res[0])
        self.assertEqual(result, [])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[0])
        self.assertEqual(result, [])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[1])
        self.assertEqual(result, [[1, 1, "1,2"]])
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {2}, res[1])
        self.assertEqual(result, [[1, 1, "N/A"]])
        _db_manager.clear_table("step_trace_data")
        result = msprof_query_data._update_top_iteration_info([(1, 1), ], {1}, res[1])
        _db_manager.destroy(res)
        self.assertEqual(result, [])

    def test_assembly_job_info_1(self):
        basic_data = [1, 2, 3, 4]
        iteration_data = None
        key = MsprofQueryData('123')
        result = key.assembly_job_info(basic_data, iteration_data)
        self.assertEqual(len(result), 1)

    def test_assembly_job_info_2(self):
        basic_data = [1, 2, 3, 4]
        iteration_data = [(1, 1, "1"), (2, 2, "2,1")]
        key = MsprofQueryData('123')
        result = key.assembly_job_info(basic_data, iteration_data)
        self.assertEqual(len(result), 2)

    def test_query_data(self):
        with mock.patch(NAMESPACE + '.MsprofQueryData.get_job_basic_info', return_value=None):
            key = MsprofQueryData('123')
            result = key.query_data()
        self.assertEqual(result, [])
        with mock.patch(NAMESPACE + '.MsprofQueryData.get_job_basic_info', return_value=[1, 2, 3, 4]), \
             mock.patch(NAMESPACE + '.MsprofQueryData.get_job_iteration_info',
                        return_value=[(1, 1, "1"), (2, 2, "2,1")]):
            key = MsprofQueryData('123')
            result = key.query_data()
        self.assertEqual(len(result), 2)


if __name__ == '__main__':
    unittest.main()
