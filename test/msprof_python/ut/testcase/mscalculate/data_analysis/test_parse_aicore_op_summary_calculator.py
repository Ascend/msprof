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

import pytest

from common_func.msprof_exception import ProfException
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from mscalculate.data_analysis.parse_aicore_op_summary_calculator import ParseAiCoreOpSummaryCalculator
from sqlite.db_manager import DBManager

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.parse_aicore_op_summary_calculator'


class TestParseAiCoreOpSummaryCalculator(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.process'):
            key = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            key.ms_run()

    def test_process(self):
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.init_params'), \
                mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_summary_table'):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.process()
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.init_params', side_effect=OSError):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.process()
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.init_params'),\
                mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_summary_table', \
                           side_effect=sqlite3.Error):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.process()

    def test_init_params(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
                pytest.raises(ProfException) as err:
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.init_params()
            self.assertEqual(err.value.code, 10)
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('os.path.exists', side_effect=(True, False)), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
                pytest.raises(ProfException) as err:
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.init_params()
            self.assertEqual(err.value.code, 10)
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_sql_dir', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                mock.patch('os.remove'):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.init_params()

    def test_create_summary_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ge_info.db')
        with mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                result = check.create_summary_table()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_conn', return_value=res), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_ge_summary_table',
                               return_value=False), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_ai_core_metrics_table',
                               return_value=False), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.create_task_time_table',
                               return_value=False):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                check.conn, check.curs = res
                check.create_summary_table()
        db_manager.destroy(res)

    def test_create_conn(self):
        db_manager = DBManager()
        res = db_manager.create_table('ai_core_op_summary.db')
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value='test\\test'), \
                mock.patch('sqlite3.connect', return_value=res[0]), \
                mock.patch('os.chmod'):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.create_conn()
        db_manager.destroy(res)

    def test_get_ge_data(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                        return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            result = check._get_ge_data()
        self.assertEqual(result, [])

    def test_create_ge_summary_table(self):
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.create_ge_summary_table()
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.create_ge_summary_table()
        with mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
                mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator._get_ge_data', return_value=[]), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                mock.patch(NAMESPACE + '.DBManager.insert_data_into_table'):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            check.create_ge_summary_table()

    def test_create_ai_core_metrics_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ai_core_op_summary.db')
        create_sql = "CREATE TABLE IF NOT EXISTS MetricSummary (index_id INT,model_id INT,step_start REAL," \
                     " step_end INT,iter_id INT,ai_core_num INT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('MetricSummary')
        data = ((1, 1, 2, 3, 4, 5),)
        db_manager_rt = DBManager()
        res_runtime = db_manager_rt.create_table("runtime.db", create_sql, insert_sql, data)
        res_runtime[1].execute("create table IF NOT EXISTS AivMetricSummary(index_id INT,model_id INT,"
                               "step_start REAL, step_end INT,iter_id INT,ai_core_num INT)")
        db_manager.attach(res[1], 'runtime.db', 'runtime')
        with mock.patch('os.path.splitext', return_value=('runtime', 'db')):
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=False):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                check.curs, check.conn = res[1], res[0]
                check.create_ai_core_metrics_table()
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_db_path', return_value='test\\test'), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                check.curs, check.conn = res[1], res[0]
                check.create_ai_core_metrics_table()
            with mock.patch(NAMESPACE + '.DBManager.attach_to_db', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                check.curs, check.conn = res[1], res[0]
                check.create_ai_core_metrics_table()
        db_manager.destroy(res)
        db_manager_rt.destroy(res_runtime)

    def test_create_task_time_table(self):
        with mock.patch('common_func.profiling_scene.Utils.get_scene',
                        return_value="train"), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=False):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table()
            with mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_task_time_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value="test"):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table()
            with mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                    mock.patch(NAMESPACE + '.ParseAiCoreOpSummaryCalculator.get_task_time_data', \
                               return_value=[[0, 1]]), \
                    mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value="test"):
                check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
                ProfilingScene().init('')
                ProfilingScene()._scene = "single_op"
                check.create_task_time_table()

    def test_get_task_time_data(self):
        check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
        self.assertEqual(check.get_task_time_data(), [])
        with mock.patch("common_func.db_manager.DBManager.check_connect_db", return_value=(True, True)), \
                mock.patch(NAMESPACE + '.AscendTaskModel.get_all_data',
                        return_value=[[1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE', 0]]):
            self.assertEqual(check.get_task_time_data(), [[1, 1, 1000, 1000, 0, 'AI_CORE', 1, 1, 1, 1]])

    def test_get_db_path(self):
        db_name = 'test'
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=1):
            check = ParseAiCoreOpSummaryCalculator(file_list, CONFIG)
            result = check.get_db_path(db_name)
        self.assertEqual(result, 1)
