#!/usr/bin/python3
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
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from mscalculate.data_analysis.op_counter_op_scene_calculator import OpCounterOpSceneCalculator
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager

file_list = {}
NAMESPACE = 'mscalculate.data_analysis.op_counter_op_scene_calculator'


class TestOpCounterOpSceneCalculator(unittest.TestCase):

    def setUp(self):
        InfoConfReader()._info_json = {}
        InfoConfReader()._start_info = {StrConstant.COLLECT_TIME_BEGIN: "1"}
        InfoConfReader()._end_info = {}
        InfoConfReader()._local_time_offset = 0

    def tearDown(self):
        InfoConfReader()._start_info = {}

    def test_process(self):
        setattr(InfoConfReader(), "_info_json", {'devices': '0'})
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.process()
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True), \
                mock.patch('os.path.exists', return_value=False):
            with mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._is_db_need_to_create', return_value=False), \
                    mock.patch(NAMESPACE + '.logging.warning'):
                check = OpCounterOpSceneCalculator(file_list, CONFIG)
                check.process()
            with mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._is_db_need_to_create', return_value=True), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch(NAMESPACE + '.logging.warning'), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                                return_value=(False, False)):
                    check = OpCounterOpSceneCalculator(file_list, CONFIG)
                    check.process()
                with mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                                return_value=res), \
                        mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._create_db'), \
                        mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator.create_ge_merge'), \
                        mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator.create_task'), \
                        mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator.create_report'):
                    check = OpCounterOpSceneCalculator(file_list, CONFIG)
                    check.process()
        db_manager.destroy(res)

    def test_is_db_need_to_create(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=True):
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
                check = OpCounterOpSceneCalculator(file_list, CONFIG)
                result = check._is_db_need_to_create()
            self.assertTrue(result)
            with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=False):
                with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=1):
                    with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=1):
                        ChipManager().chip_id = ChipModel.CHIP_V1_1_0
                        check = OpCounterOpSceneCalculator(file_list, CONFIG)
                        result = check._is_db_need_to_create()
                    self.assertEqual(result, False)
                with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=1), \
                        mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=2):
                    ChipManager().chip_id = ChipModel.CHIP_V2_1_0
                    check = OpCounterOpSceneCalculator(file_list, CONFIG)
                    result = check._is_db_need_to_create()
                self.assertEqual(result, True)

    def test_get_ge_sql(self):
        InfoConfReader()._info_json = {'devices': '0'}
        check = OpCounterOpSceneCalculator(file_list, CONFIG)
        result = getattr(check, "_get_ge_sql")()
        sql = "select model_id, op_name, op_type, task_type, task_id, stream_id, "\
              "batch_id,context_id from TaskInfo where device_id=0 " \
              "and task_type != 'COMMUNICATION' and task_type != 'HCCL_AI_CPU'"
        self.assertEqual(sql, result)

    def test_create_ge_merge(self):
        create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge (model_name text,model_id INTEGER," \
                     "op_name text,op_type text,task_id INTEGER,stream_id INTEGER,device_id INTEGER)"
        insert_sql = "insert into {} values (?,?,?,?,?,?,?)".format('ge_task_merge')
        data = (('resnet50', 1, 'trans_TransData_0', 'TransData', 3, 4, 5),)
        db_manager = DBManager()
        res = db_manager.create_table("op_counter.db", create_sql, insert_sql, data)
        curs = res[1]
        create_ge_sql = "create table if not exists ge_task_data (device_id INTEGER," \
                        "model_name TEXT,model_id INTEGER,op_name TEXT,stream_id INTEGER," \
                        "task_id INTEGER,block_num INTEGER,op_state TEXT,task_type TEXT," \
                        "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT," \
                        "input_data_types TEXT,input_shapes TEXT,output_count INTEGER," \
                        "output_formats TEXT,output_data_types TEXT,output_shapes TEXT)"
        insert_ge_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)".format('ge_task_data')
        data = ((0, 'test', 1, 'model', 5, 3, 2, 'static', 'AI_CORE', 'trans_data',
                 1, 1, '12', '1750', '1752', 0, 'test', 'test2', 'test3'),)
        db_manager_ge = DBManager()
        ge_res = db_manager_ge.create_table("ge_info.db", create_ge_sql, insert_ge_sql, data)
        sql = "select model_name, model_id, op_name, op_type, task_type, task_id, stream_id, " \
              "device_id from ge_task_data where (iter_id=1 or iter_id=0) "
        ge_data = [('resnet50', 1, 'trans_TransData_0', 'TransData', 'AI_CORE', 3, 5, 0, 1),
                   ('resnet50', 1, 'conv1conv1_relu', 'Conv2D', 'AI_CORE', 4, 5, 0, 1),
                   ('resnet50', 1, 'conv1conv1_relu', 'Conv2D', 'AI_CORE', 2, 5, 0, 2)]
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='db\\ge_info.db'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=ge_res), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._get_ge_sql', return_value=sql), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.create_ge_merge()
        res[1].execute("drop table ge_task_merge")
        ge_res[1].execute('drop table ge_task_data')
        db_manager_ge.destroy(ge_res)
        db_manager.destroy(res)

    def test_create_db(self):
        db_manager = DBManager()
        res = db_manager.create_table("op_counter.db")
        ge_create_sql = "CREATE TABLE IF NOT EXISTS ge_task_merge(model_id INTEGER," \
                        "op_name text,op_type text,task_type text,task_id INTEGER,stream_id INTEGER," \
                        "batch_id INTEGER)"
        rts_task_create_sql = "CREATE TABLE IF NOT EXISTS rts_task(task_id INTEGER,stream_id INTEGER," \
                              "start_time INTEGER, duration INTEGER,task_type text,index_id INTEGER,batch_id INTEGER)"
        op_report_create_sql = " CREATE TABLE IF NOT EXISTS op_report(op_type text,core_type text," \
                               "occurrences text,total_time REAL,min REAL,avg REAL,max REAL," \
                               "ratio text)"

        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=ge_create_sql):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            _create_db = getattr(check, "_create_db")
            _create_db()
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=rts_task_create_sql):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            _create_db = getattr(check, "_create_db")
            _create_db()
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=op_report_create_sql):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            _create_db = getattr(check, "_create_db")
            _create_db()
        res[1].execute("drop table ge_task_merge")
        res[1].execute("drop table rts_task")
        res[1].execute("drop table op_report")
        db_manager.destroy(res)

    def test_create_task(self):
        check = OpCounterOpSceneCalculator(file_list, CONFIG)
        check.create_task()
        with mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True):
            check.create_task()
            with mock.patch(NAMESPACE + '.AscendTaskModel.get_all_data',
                            return_value=[[1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE', 0]]):
                check.create_task()

    def test_get_op_report_sql(self):
        check = OpCounterOpSceneCalculator(file_list, CONFIG)
        result = getattr(check, "_get_op_report_sql")()
        expected_sql = "select op_type, ge_task_merge.task_type, count(op_type), sum(duration) as total_time," \
                       " min(duration) as min, sum(duration)/count(op_type) as avg, max(duration) as max " \
                       "from ge_task_merge, rts_task where ge_task_merge.task_id=rts_task.task_id " \
                       "and ge_task_merge.stream_id=rts_task.stream_id and ge_task_merge.batch_id=rts_task.batch_id " \
                       "and ge_task_merge.context_id=rts_task.subtask_id " \
                       "and rts_task.start_time > 1000 " \
                       "group by op_type,ge_task_merge.task_type"
        self.assertEqual(result, expected_sql)

    def test_create_report(self):
        db_manager = DBManager()
        res = db_manager.create_table('test.db')
        merge_curs = res[1]
        merge_curs.execute("CREATE TABLE IF NOT EXISTS ge_task_merge(model_id INTEGER,"
                           "op_name text,op_type text,task_type text,task_id INTEGER,stream_id INTEGER,"
                           "batch_id INTEGER)")
        merge_curs.execute("CREATE TABLE IF NOT EXISTS rts_task(task_id INTEGER,"
                           "stream_id INTEGER,start_time INTEGER,duration INTEGER,task_type INTEGER,index_id INTEGER,"
                           "batch_id INTEGER)")
        merge_curs.execute("CREATE TABLE IF NOT EXISTS op_report(op_type text,core_type text,"
                           "occurrences text,total_time REAL,min REAL,avg REAL,max REAL,ratio text)")
        res[0].commit()
        sql = "select op_type, ge_task_merge.task_type, count(op_type), sum(duration) as total_time," \
              " min(duration) as min, sum(duration)/count(op_type) as avg, " \
              "max(duration) as max from ge_task_merge, rts_task " \
              "where ge_task_merge.task_id=rts_task.task_id " \
              "and ge_task_merge.stream_id=rts_task.stream_id " \
              "and ge_task_merge.task_type=rts_task.task_type " \
              "and ge_task_merge.batch_id=rts_task.batch_id group by op_type"
        with mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._get_op_report_sql', return_value=sql):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.create_report()
        merge_curs.execute("insert into ge_task_merge values (1,'pool1','Pooling',5,5,0,10)")
        merge_curs.execute("insert into rts_task values (5,0,5,0,5,0,10)")
        res[0].commit()
        with mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._get_op_report_sql', return_value=sql), \
                mock.patch(NAMESPACE + '.OpCounterOpSceneCalculator._cal_total', return_value=10000):
            check = OpCounterOpSceneCalculator(file_list, CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.create_report()
        merge_curs.execute('drop table ge_task_merge')
        merge_curs.execute('drop table rts_task')
        merge_curs.execute('drop table op_report')
        res[0].commit()
        db_manager.destroy(res)

    def test_cal_total(self):
        task_time = [(1, 2, 3, 4), (5, 6, 7, 8)]
        check = OpCounterOpSceneCalculator(file_list, CONFIG)
        _cal_total = getattr(check, "_cal_total")
        result = _cal_total(task_time)
        self.assertEqual(result, 12)




if __name__ == '__main__':
    unittest.main()
