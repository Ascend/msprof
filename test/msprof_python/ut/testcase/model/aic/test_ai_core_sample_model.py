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
import unittest
from collections import OrderedDict
from unittest import mock

import pytest

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from msmodel.aic.ai_core_sample_model import AiCoreSampleModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from constant.info_json_construct import InfoJson

NAMESPACE = 'msmodel.aic.ai_core_sample_model'


class TestAiCoreSampleModel(unittest.TestCase):

    def test_init(self):
        db_manager = DBManager()
        res = db_manager.create_table('aicore.db')
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}),\
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(None, None)):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            ret = check.init()
        self.assertFalse(ret)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=None), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.BaseModel.create_table'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            ret = check.init()
        self.assertTrue(ret)
        db_manager.destroy(res)

    def test_create_aicore_originaldatatable(self):
        table_name = 'AICoreOriginalData'
        sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData(mode integer, replayid integer," \
              "timestamp integer, coreid integer, task_cyc integer, event1 integer, event2 integer," \
              " event3 integer, event4 integer)"

        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}),\
                DBOpen("aicore.db") as db_open:
            db_open.create_table(sql)
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                            return_value=None), \
                    mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                    mock.patch(NAMESPACE + '.logging.error'), \
                    mock.patch(NAMESPACE + '.error'), \
                    pytest.raises(ProfException) as err:
                check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
                check.conn = db_open.db_conn
                check.create_aicore_originaldatatable(table_name)
                self.assertEqual(err.value.code, 10)
            with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                            return_value=sql):
                check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
                check.conn, check.cur = db_open.db_conn, db_open.db_curs
                check.create_aicore_originaldatatable(table_name)

    def test_insert_metric_summary_table(self):
        sql = 'SELECT cast(SUM((task_cyc*1000/(100))) as decimal(8,2)),' \
              'cast(1.0*SUM(r49)/SUM(task_cyc) as decimal(8,2)),' \
              'cast(1.0*SUM(r4a)/SUM(task_cyc) as decimal(8,2)),' \
              'r4b, r4c, r4d, r4e FROM EventCount where coreid = ?'
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount (coreid integer, task_cyc integer," \
                     "r49 integer, r4a integer, r4b integer, r4c integer, r4d integer, r4e integer)"
        data = ((1, 1, 1, 1, 1, 1, 1, 1),)
        insert_sql = "insert into {} values (?,?,?,?,?,?,?,?)".format('EventCount')

        create_metric_sql = "CREATE TABLE IF NOT EXISTS MetricSummary (metric text, " \
                            "value numeric, coreid INT)"
        insert_metric_sql = "insert into {} values (?,?,?)".format('MetricSummary')
        metric_data = ((1, 1, 1),)
        db_manager = DBManager()
        db_manager.create_table("aicore.db", create_sql, insert_sql, data)
        res = db_manager.create_table("aicore.db", create_metric_sql, insert_metric_sql, metric_data)
        freq = 1150000000
        metric_key = 'test'
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            result = check.insert_metric_summary_table(freq, metric_key)
            self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}),\
                mock.patch(NAMESPACE + '.AiCoreSampleModel._get_metrics', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.error'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            check.insert_metric_summary_table(freq, 'ArithmeticUtilization')
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.AiCoreSampleModel.sql_insert_metric_summary_table',
                           return_value=sql):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            check.conn = res[0]
            check.cur = res[1]
            check.insert_metric_summary_table(freq, 'ArithmeticUtilization')
        res[0].commit()
        db_manager.destroy(res)

    def test_insert_metric_value(self):
        metrics_config = OrderedDict([('total_cycles', 'task_cyc'),
                                      ('scalar_ld_ratio', '1.0*SUM(r3a)/SUM(task_cyc)')])
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount (coreid integer, task_id integer )"
        insert_sql = "insert into {} values (?,?)".format('EventCount')
        data = ((1, 1),)
        db_manager = DBManager()
        res = db_manager.create_table("aicore.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=False):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            result = check.insert_metric_value()
        self.assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}),\
                mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=metrics_config):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            check.conn = res[0]
            check.cur = res[1]
            check.insert_metric_value()
        res[1].execute("drop table EventCount")
        res[1].execute("drop table MetricSummary")
        db_manager.destroy(res)

    def test_sql_insert_metric_summary_table(self):
        metrics = ['total_time(ms)', "mac_fp16_ratio", "mac_int8_ratio"]
        freq = 100
        metric_key = 'ai_core_metrics'
        sql = 'SELECT cast(SUM((task_cyc*1000000/(100))) as decimal(8,2)),' \
              'cast(1.0*SUM(r49)/SUM(task_cyc) as decimal(8,2)),' \
              'cast(1.0*SUM(r4a)/SUM(task_cyc) as decimal(8,2)) ' \
              'FROM EventCount where coreid = ?'
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                mock.patch(NAMESPACE + '.CalculateAiCoreData.add_fops_header'):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            result = check.sql_insert_metric_summary_table(metrics, freq)
        self.assertEqual(result, sql)

    def test_get_ai_core_event_chunk(self):
        event = [1, 1, 1, 1, 1, 1, 1, 1]
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            result = check.get_ai_core_event_chunk(event)
        self.assertEqual(result, [[1, 1, 1, 1, 1, 1, 1, 1]])

    def test_flush(self):
        sql = "CREATE TABLE IF NOT EXISTS AICoreOriginalData(mode integer, replayid integer," \
              "timestamp integer, coreid integer, task_cyc integer, event1 integer, event2 integer," \
              " event3 integer, event4 integer)"
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                DBOpen('aicore.db') as db_open:
            db_open.create_table(sql)
            InfoJson(devices='0').apply()
            check = AiCoreSampleModel('test', 'aicore.db', ['AICoreOriginalData'], 'ai_core_metrics')
            check.conn, check.cur = db_open.db_conn, db_open.db_curs
            check.flush([[1, 2, 3, 2, 3, 5, 6]])

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = AiCoreSampleModel('test', 'nic.db', ['NicOriginalData'], 'ai_core_metrics')
            check.clear()

    def test_adapt_register_change(self):
        sql = "SELECT cast(SUM((task_cyc*1000000/(1224000000.0))) as decimal(8,2))," \
              "cast(1.0*SUM(r3d)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
              "cast(1.0*SUM(r3e)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
              "cast(1.0*SUM(r43)*128.0*2.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
              "cast(1.0*SUM(r44)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
              "cast(1.0*SUM(r37)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
              "cast(1.0*SUM(r38)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))" \
              " FROM EventCount where coreid = ?"
        sql1 = "SELECT cast(SUM((task_cyc*1000000/(1224000000.0))) as decimal(8,2))," \
               "cast(1.0*SUM(r1a5)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
               "cast(1.0*SUM(r1a6)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
               "cast(1.0*(SUM(r17f)+SUM(r180))*128.0*2.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) " \
               "as decimal(8,2))," \
               "cast(1.0*SUM(r191)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
               "cast(1.0*SUM(r37)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2))," \
               "cast(1.0*SUM(r38)*128.0*1.0/((task_cyc*1000/(1224000000.0*1000.0)))/(8589934592.0) as decimal(8,2)) " \
               "FROM EventCount where coreid = ?"
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value={}):
            check = AiCoreSampleModel('test', 'aicore.db', ['EventCount'], 'ai_core_metrics')
            check.sample_config['ai_core_metrics'] = "MemoryUB"
            check.sample_config['ai_core_profiling_events'] = '0x10,0x13,0x37,0x38,0x3d,0x3e,0x43,0x44'
            result = check._adapt_register_change(sql)
            self.assertEqual(result, sql)
            check.sample_config['ai_core_profiling_events'] = '0x37,0x38,0x1a5,0x1a6,0x17f,0x180,0x191'
            result = check._adapt_register_change(sql)
            self.assertEqual(result, sql1)

if __name__ == '__main__':
    unittest.main()
