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
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from msmodel.hardware.llc_model import LlcModel
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.llc_model'


class TestLlcModel(unittest.TestCase):
    table_list = ["LLCOriginalData", "LLCEvents", "LLCMetrics", "LLCMetricData", "LLCCapacity"]

    def test_flush(self):
        with mock.patch(NAMESPACE + '.LlcModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = LlcModel('test', 'llc.db', self.table_list)
            check.flush([])

    def test_drop_tab(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        check = LlcModel('test', 'llc.db', self.table_list)
        check.conn, check.cur = res[0], res[1]
        check.drop_tab()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.drop_tab()

    def test_create_table(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_original_sql = "CREATE TABLE IF NOT EXISTS LLCOriginalData(device_id INT," \
                              "timestamp REAL,counts INT,event INT,l3tid INT)"
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        with mock.patch(NAMESPACE + '.LlcModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_table_with_key',
                           side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.create_table()
        with mock.patch(NAMESPACE + '.LlcModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_table_with_key',
                           return_value=None), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            try:
                check.create_table()
            except ProfException as err:
                self.assertEqual(err.code, ProfException.PROF_SYSTEM_EXIT)
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        with mock.patch(NAMESPACE + '.LlcModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_table_with_key',
                           return_value=create_original_sql), \
                mock.patch(NAMESPACE + '.LlcModel.create_events_trigger',
                           return_value=1):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.create_table()
        with mock.patch(NAMESPACE + '.LlcModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_table_with_key',
                           return_value=create_original_sql), \
                mock.patch(NAMESPACE + '.LlcModel.create_events_trigger',
                           return_value=0):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.create_table()
        res[1].execute("drop table LLCOriginalData")
        db_manager.destroy(res)

    def test_insert_metrics(self):
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        res[1].execute("CREATE TABLE IF NOT EXISTS LLCMetrics(device_id INT,l3tid INT,"
                       "timestamp REAL,hitrate REAL,throughput REAL)")
        with mock.patch(NAMESPACE + '.LlcModel.calculate_metrics'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.metrics_data = [(0, 0, 586157878285.0, '0', 0)]
            check.insert_metrics_data()
            res[1].execute("drop table LLCMetrics")
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.metrics_data = [(0, 0, 586157878285.0, '0', 0)]
            check.insert_metrics_data()
        db_manager.destroy(res)

    def test_calculate_hit_rate(self):
        with mock.patch(NAMESPACE + '.float_calculate', side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            result = check.calculate_hit_rate((0, 0, 0.0, 0, None, None, None, None, None, None, None))
            self.assertEqual(result, 0)

    def test_calculate_throughput(self):
        with mock.patch(NAMESPACE + '.float_calculate', side_effect=ZeroDivisionError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            result = check.calculate_throughput((0, 3, 231839.0, 91324, 0, 48600, 0, 99591, 24478, 0, 0),
                                                0.0002850625)
        self.assertEqual(result, 0)

    def test_calculate_metrics(self):
        create_event_sql = "CREATE TABLE IF NOT EXISTS LLCEvents(device_id INT,l3tid INT," \
                           "timestamp REAL,event0 INT,event1 INT,event2 INT,event3 INT," \
                           "event4 INT,event5 INT,event6 INT,event7 INT, " \
                           "PRIMARY KEY(device_id,l3tId,timestamp))"
        insert_sql = "insert into LLCEvents values(0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0)"
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.calculate_metrics()
        res[1].execute(create_event_sql)
        res[1].execute(insert_sql)
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.calculate_metrics()
        res[1].execute("insert into LLCEvents values(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), "
                       "(0,1,2,3,4,5,6,7,8,9,1), (0,1,3,4,5,6,7,8,9,1,2), (0,2,2,3,4,5,6,7,8,9,1),"
                       " (0,3,2,3,4,5,6,7,8,9,1)")
        check = LlcModel('test', 'llc.db', self.table_list)
        check.conn, check.cur = res[0], res[1]
        check.calculate_metrics()
        res[1].execute('drop table LLCEvents')
        res[0].commit()
        db_manager.destroy(res)

    def test_create_events_trigger(self):
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            result = check.create_events_trigger('resd')
        self.assertEqual(result, 1)
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        with mock.patch(NAMESPACE + '.logging.error'):
            check = LlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            result = check.create_events_trigger('read')
            self.assertEqual(result, 0)
        res[1].execute("CREATE TABLE IF NOT EXISTS LLCOriginalData(device_id INT,"
                       "timestamp REAL,counts INT,event INT,l3tid INT)")
        res[1].execute("CREATE TABLE IF NOT EXISTS LLCEvents(device_id INT,l3tid INT,"
                       "timestamp REAL,event0 INT,event1 INT,event2 INT,event3 INT,"
                       "event4 INT,event5 INT,event6 INT,event7 INT, "
                       "PRIMARY KEY(device_id,l3tId,timestamp))")
        check_read = LlcModel('test', 'llc.db', self.table_list)
        check_write = LlcModel('test', 'llc.db', self.table_list)
        check_read.conn, check_read.cur = res[0], res[1]
        check_write.conn, check_write.cur = res[0], res[1]
        result_read = check_read.create_events_trigger('read')
        result_write = check_write.create_events_trigger('write')
        self.assertEqual(result_read, 0)
        self.assertEqual(result_write, 0)
        res[0].execute('drop table LLCOriginalData')
        res[0].execute('drop table LLCEvents')
        db_manager.destroy(res)

    def test_init_l3_list_dispatch_should_return_two_llc_when_chip_is_v6_1_1(self):
        origin_chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_1_1
        try:
            check = LlcModel('test', 'llc.db', self.table_list)
            self.assertEqual(check.l3_list, [0, 1])
        finally:
            ChipManager().chip_id = origin_chip_id
