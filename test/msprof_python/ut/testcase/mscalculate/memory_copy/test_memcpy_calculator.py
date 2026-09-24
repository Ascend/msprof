# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.constant import clear_dt_project
from mscalculate.memory_copy.memcpy_calculator import MemcpyCalculator
from msconfig.config_manager import ConfigManager
from profiling_bean.db_dto.step_trace_dto import IterationRange
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = "common_func.msprof_iteration"


def prepare_step_trace_db(db_path):
    conn, curs = DBManager.create_connect_db(db_path)
    columns = ["index_id", "model_id", "step_start", "step_end", "iter_id", "ai_core_num"]
    sql = "create table if not exists {0} ({1})".format(DBNameConstant.TABLE_STEP_TRACE_DATA,
                                                        ",".join(columns))
    conn.execute(sql)

    prepare_step_trace_data(conn, curs)
    prepare_ts_memcpy_data(conn, curs)

    DBManager.destroy_db_connect(conn, curs)


def prepare_step_trace_data(conn, curs):
    columns = ["index_id", "model_id", "step_start", "step_end", "iter_id", "ai_core_num"]
    sql = "create table if not exists {0} ({1})".format(DBNameConstant.TABLE_STEP_TRACE_DATA,
                                                        ",".join(columns))
    conn.execute(sql)

    step_trace_data = [(1, 1, 1000, 2000, 1, 30),
                       (1, 2, 3000, 4000, 2, 40)]

    if conn and step_trace_data:
        sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_STEP_TRACE_DATA, "?," * (len(step_trace_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, sql, step_trace_data)


def prepare_ts_memcpy_data(conn, curs):
    sql = DBManager.sql_create_general_table(DBNameConstant.TABLE_TS_MEMCPY + "Map", DBNameConstant.TABLE_TS_MEMCPY,
                                             ConfigManager.TABLES)
    curs.execute(sql)
    ts_memcpy_data = [(500, 1, 1, 0), (600, 1, 1, 1), (700, 1, 1, 2), (800, 1, 1, 3), (800, 1, 1, 1),
                      (2500, 1, 2, 0), (2600, 1, 2, 1), (2700, 1, 2, 2), (2800, 2, 1, 0)]

    if conn and ts_memcpy_data:
        sql = 'insert into {0} values ({1})'.format(
            DBNameConstant.TABLE_TS_MEMCPY, "?," * (len(ts_memcpy_data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, sql, ts_memcpy_data)


class TestMemcpyModel(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_MEMCPY_TEST')

    def setUp(self):
        os.makedirs(os.path.join(self.DIR_PATH, "sqlite"))
        db_path = os.path.join(self.DIR_PATH, "sqlite", DBNameConstant.DB_STEP_TRACE)
        prepare_step_trace_db(db_path)

    def tearDown(self):
        clear_dt_project(self.DIR_PATH)

    def test_calculate_1(self):
        # stream_id, task_id, receive_time, start_time, end_time, duration, name, type
        expect_res = [(1, 2, 2.5, 2.6, 2.7, 0.10000000000000009, 'MemcopyAsync', 'other')]
        memcpy_calculator = MemcpyCalculator({}, {"result_dir": self.DIR_PATH, "iter_id": IterationRange(2, 1, 1),
                                                  "model_id": 2})
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)

        with mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.STEP_INFO):
            ProfilingScene().init("")
            InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1000)]).apply()
            memcpy_calculator.calculator_connect_db()
            memcpy_calculator.calculate()
            self.assertEqual(expect_res, memcpy_calculator._memcpy_data)

    def test_calculate_2(self):
        # stream_id, task_id, receive_time, start_time, end_time, duration, name, type
        expect_res = [(1, 1, 0.5, 0.6, 0.7, 0.09999999999999998, 'MemcopyAsync', 'other'),
                      (1, 2, 2.5, 2.6, 2.7, 0.10000000000000009, 'MemcopyAsync', 'other')]
        memcpy_calculator = MemcpyCalculator({}, {"result_dir": self.DIR_PATH, "iter_id": IterationRange(2, 1, 1),
                                                  "model_id": 2})
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

        with mock.patch('common_func.utils.Utils.get_scene', return_value=Constant.SINGLE_OP):
            ProfilingScene().init("")
            InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1000)]).apply()
            memcpy_calculator.calculator_connect_db()
            memcpy_calculator.calculate()
            self.assertEqual(expect_res, memcpy_calculator._memcpy_data)
