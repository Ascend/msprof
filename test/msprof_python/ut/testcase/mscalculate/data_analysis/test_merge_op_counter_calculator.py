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

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from constant.ut_db_name_constant import DB_OP_COUNTER
from constant.ut_db_name_constant import TABLE_OP_COUNTER_GE_MERGE
from constant.ut_db_name_constant import TABLE_OP_COUNTER_OP_REPORT
from constant.ut_db_name_constant import TABLE_OP_COUNTER_RTS_TASK
from mscalculate.data_analysis.merge_op_counter_calculator import MergeOpCounterCalculator
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.merge_op_counter_calculator'


class TestMergeOpCounterCalculator(unittest.TestCase):
    def setUp(self):
        InfoConfReader()._info_json = {}
        InfoConfReader()._start_info = {StrConstant.COLLECT_TIME_BEGIN: "1"}
        InfoConfReader()._end_info = {}
        InfoConfReader()._local_time_offset = 0

    def tearDown(self):
        InfoConfReader()._start_info = {}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MergeOpCounterCalculator.process'):
            key = MergeOpCounterCalculator(file_list, CONFIG)
            key.ms_run()

    def test_process(self):
        db_manager = DBManager()
        res = db_manager.create_table('op_counter.db')
        with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._init_params'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._is_db_need_to_create',
                            return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = MergeOpCounterCalculator(file_list, CONFIG)
                result = check.process()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._is_db_need_to_create',
                            return_value=True), \
                    mock.patch('common_func.profiling_scene.Utils.get_scene',
                               return_value="step_info"):
                with mock.patch(NAMESPACE + '.MergeOpCounterCalculator.create_db',
                                return_value=res), \
                        mock.patch(NAMESPACE + '.MergeOpCounterCalculator._create_ge_merge'), \
                        mock.patch(NAMESPACE + '.MergeOpCounterCalculator._create_task'), \
                        mock.patch(NAMESPACE + '.MergeOpCounterCalculator._create_report'):
                    check = MergeOpCounterCalculator(file_list, CONFIG)
                    check.process()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._init_params', side_effect=OSError):
            check = MergeOpCounterCalculator(file_list, CONFIG)
            check.process()

    def test_is_db_need_to_create(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db',
                           return_value=True):
            check = MergeOpCounterCalculator(file_list, CONFIG)
            result = check._is_db_need_to_create()
        self.assertEqual(result, True)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db',
                           return_value=False), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path',
                           return_value='test\\db'):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check = MergeOpCounterCalculator(file_list, CONFIG)
            result = check._is_db_need_to_create()
        self.assertEqual(result, False)

    def test_init_params(self):
        with mock.patch(NAMESPACE + '.PathManager.get_sql_dir',
                        return_value='test\\test'):
            with pytest.raises(ProfException) as error:
                check = MergeOpCounterCalculator(file_list, CONFIG)
                getattr(check, "_init_params")()
                self.assertEqual(error.value.code, 10)
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch('os.remove'):
                check = MergeOpCounterCalculator(file_list, CONFIG)
                getattr(check, "_init_params")()

    def test_create_db(self):
        db_manager = DBManager()
        res = db_manager.create_table('op_counter.db')
        ge_create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge(model_name text,model_id INTEGER," \
                        "op_name text,op_type text,task_id INTEGER,stream_id INTEGER,device_id INTEGER)"
        rts_task_create_sql = "CREATE TABLE IF NOT EXISTS rts_task(model_id INTEGER,task_id INTEGER," \
                              "stream_id INTEGER,task_type INTEGER,duration INTEGER,device_id INTEGER)"
        op_report_create_sql = " CREATE TABLE IF NOT EXISTS op_report(model_name text,op_type text," \
                               "core_type text,occurrences text,total_time REAL,min REAL,avg REAL," \
                               "max REAL,ratio text,device_id INTEGER)"
        with mock.patch('os.path.join', return_value='test\\test'):
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=(None, None)), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    pytest.raises(ProfException) as error:
                check = MergeOpCounterCalculator(file_list, CONFIG)
                check.create_db('test')
                self.assertEqual(error.value.code, 10)
            with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                            return_value=res):
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=ge_create_sql):
                    check = MergeOpCounterCalculator(file_list, CONFIG)
                    check.create_db('test')
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=rts_task_create_sql):
                    check = MergeOpCounterCalculator(file_list, CONFIG)
                    check.create_db('test')
                with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                                return_value=op_report_create_sql):
                    check = MergeOpCounterCalculator(file_list, CONFIG)
                    check.create_db('test')
        res[1].execute("drop table ge_task_merge")
        res[1].execute("drop table rts_task")
        res[1].execute("drop table op_report")
        db_manager.destroy(res)

    def test_get_ge_data(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_index_id_list_with_index_and_model',
                        return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[1]):
            check = MergeOpCounterCalculator(file_list, CONFIG)
            result = check._get_ge_data('')
        self.assertEqual(result, [])

    def test_get_op_report_sql(self):
        with mock.patch('common_func.profiling_scene.Utils.get_scene',
                        return_value="step_info"):
            check = MergeOpCounterCalculator(file_list, CONFIG)
            ProfilingScene().init('')
            _get_op_report_sql = getattr(check, "_get_op_report_sql")
            result = _get_op_report_sql()
        self.assertEqual("select op_type, ge_task_merge.task_type, count(op_type), sum(duration) as total_time, "
                         "min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max, "
                         "ge_task_merge.model_id from ge_task_merge, rts_task "
                         "where ge_task_merge.task_id=rts_task.task_id and ge_task_merge.stream_id=rts_task.stream_id "
                         "and ge_task_merge.batch_id=rts_task.batch_id "
                         "and ge_task_merge.context_id=rts_task.subtask_id "
                         "and rts_task.start_time > 1000 "
                         "group by op_type,ge_task_merge.task_type",
                         result)

    def test_create_ge_merge(self):
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                           return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.MergeOpCounterCalculator._get_ge_data', return_value=[[1]]), \
                mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = MergeOpCounterCalculator(file_list, CONFIG)
            getattr(check, "_create_ge_merge")()

    def test_create_task(self):
        check = MergeOpCounterCalculator(file_list, CONFIG)
        check._create_task()
        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            check._create_task()
            with mock.patch(NAMESPACE + '.AscendTaskModel.get_all_data',
                            return_value=[[1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE', 0]]):
                check._create_task()

    def test_create_report(self):
        sql = "select op_type, task_type, count(op_type), sum(duration) as total_time," \
              " min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max, " \
              "ge_task_merge.device_id, model_name from ge_task_merge, rts_task " \
              "where ge_task_merge.task_id=rts_task.task_id " \
              "and ge_task_merge.stream_id=rts_task.stream_id " \
              "and ge_task_merge.model_id=rts_task.model_id " \
              "and ge_task_merge.device_id=0 " \
              "and rts_task.device_id=0 " \
              "group by model_name,op_type"
        ge_create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge(model_name text,model_id INTEGER," \
                        "op_name text,op_type text,task_id INTEGER,stream_id INTEGER,device_id INTEGER)"

        data = ((1, 2, 1, 2, 2, 3, 1),)
        task_sql = "CREATE TABLE IF NOT EXISTS rts_task(model_id INTEGER,task_id INTEGER," \
                   "stream_id INTEGER,task_type INTEGER,duration INTEGER,device_id INTEGER)"
        task_data = ((2, 2, 3, 2, 2, 0),)
        with DBOpen(DB_OP_COUNTER) as db_open:
            db_open.create_table(ge_create_sql)
            db_open.insert_data(TABLE_OP_COUNTER_GE_MERGE, data)

            db_open.create_table(task_sql)
            db_open.insert_data(TABLE_OP_COUNTER_RTS_TASK, task_data)
            with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._get_op_report_sql', return_value=sql):
                check = MergeOpCounterCalculator(file_list, CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                result = check._create_report()
                self.assertEqual(result, None)
            db_open.db_curs.execute("insert into ge_task_merge values(1, 2, 1, 2, 2, 3, 0)")
            db_open.db_curs.execute("CREATE TABLE IF NOT EXISTS op_report(model_name text,op_type text,"
                                    "core_type text,occurrences text,total_time REAL,min REAL,avg REAL,"
                                    "max REAL,ratio text)")
            with mock.patch(NAMESPACE + '.MergeOpCounterCalculator._get_op_report_sql', return_value=sql), \
                    mock.patch(NAMESPACE + '.get_ge_model_name_dict', return_value={0: "model_1"}):
                check = MergeOpCounterCalculator(file_list, CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                _create_report = getattr(check, "_create_report");
                _create_report()
                db_open.db_conn.commit()
                model_name = \
                    db_open.db_curs.execute("select model_name from {}".format(TABLE_OP_COUNTER_OP_REPORT)).fetchone()[
                        0]
                self.assertEqual("model_1", model_name)
