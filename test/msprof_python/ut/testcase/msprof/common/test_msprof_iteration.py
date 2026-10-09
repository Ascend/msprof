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
from collections import OrderedDict
from collections import namedtuple
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_iteration import MsprofIteration
from constant.constant import INFO_JSON
from profiling_bean.db_dto.step_trace_dto import IterationRange
from sqlite.db_manager import DBManager

NAMESPACE = 'common_func.msprof_iteration'


class TestMsprofIteration(unittest.TestCase):
    def setUp(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def tearDown(self):
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_get_iteration_time(self):
        index_id = 1
        model_id = 1
        with mock.patch(NAMESPACE + '.Utils.is_step_scene', return_value=True):
            key = MsprofIteration('123')
            result = key.get_iteration_time(IterationRange(model_id, index_id, 1))
        self.assertEqual(result, [])

    def test_get_step_iteration_time_1(self):
        index_id = 1
        model_id = 1
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT," \
                     "step_start,step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 1, 118562263836, 118571743672, 1, 60),)
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = MsprofIteration('123')
            InfoConfReader()._info_json = INFO_JSON
            result = key.get_step_iteration_time(IterationRange(model_id, index_id, 1))
        self.assertEqual(len(result), 0)
        with mock.patch(NAMESPACE + '.MsprofIteration._get_iteration_time', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.logging.error'):
            key = MsprofIteration('123')
            result = key.get_step_iteration_time(IterationRange(model_id, index_id, 1))
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db")
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            key = MsprofIteration('123')
            key.get_step_iteration_time(IterationRange(model_id, index_id, 1))
            res[1].execute("drop table step_trace_data")
            res[0].commit()
            db_manager.destroy(res)

    def test_get_step_iteration_time_2(self):
        index_id = 1
        model_id = 2
        create_sql = "create table IF NOT EXISTS step_trace_data (index_id INT,model_id INT," \
                     "step_start,step_end,iter_id INT default 1, ai_core_num INT default 0)"
        data = ((1, 2, 118562263836, 118571743672, 1, 60),
                (2, 2, 118571743772, 118571743872, 2, 60))
        insert_sql = "insert into {0} values ({value})".format("step_trace_data",
                                                               value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            key = MsprofIteration('123')
            result = key._generate_trace_result(key.get_step_iteration_time(IterationRange(model_id, index_id, 1)))
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("step_trace.db")
        res[1].execute("drop table step_trace_data")
        res[0].commit()
        db_manager.destroy(res)

    def test_get_trace_iteration_end(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=1), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, 1)

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=1), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, OrderedDict())

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofIteration._generate_trace_iter_end_result', return_value=1), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value="step_trace.db"):
            key = MsprofIteration('123')
            result = key._MsprofIteration__get_trace_iteration_end()
        self.assertEqual(result, OrderedDict())

    def test_get_iter_id_within_iter_range(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        step_trace = namedtuple('data', ['step_end', 'index_id'])
        iter_range = namedtuple('iter_range', ['iteration_end', 'iteration_start'])
        step_trace_data = step_trace(100, 1)
        iter_range_data = iter_range('100', '99')
        result = MsprofIteration.get_iter_id_within_iter_range([step_trace_data], 90, iter_range_data)
        result_1 = MsprofIteration.get_iter_id_within_iter_range([], 90, iter_range_data)

        self.assertEqual(result, 1)
        self.assertEqual(result_1, '100')


if __name__ == '__main__':
    unittest.main()
