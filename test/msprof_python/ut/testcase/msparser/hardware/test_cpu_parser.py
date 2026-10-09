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
from unittest.mock import Mock

import pytest

from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.cpu_parser import ParsingCPUData
from msparser.hardware.cpu_parser import create_eventcounttable
from msparser.hardware.cpu_parser import create_hotinstable
from msparser.hardware.cpu_parser import create_originaldatatable
from msparser.hardware.cpu_parser import get_cpu_pmu_events
from msparser.hardware.cpu_parser import insert_eventcounttable
from msparser.hardware.cpu_parser import insert_hotinstable
from msparser.hardware.cpu_parser import sql_insert_eventcounttable
from msparser.hardware.cpu_parser import sql_insert_hotinstable
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen

NAMESPACE = 'msparser.hardware.cpu_parser'


def test_create_originaldatatable():
    sql = "CREATE TABLE IF NOT EXISTS OriginalData(common text,pid int,tid int,core int," \
          "timestamp numeric,pmucount int,pmuevent char(10),ip text,function text,offset text," \
          "module text,callstack text,replayid int)"
    db_manager = DBManager()
    res = db_manager.create_table('test.db')
    with mock.patch('os.path.join', return_value='test\\test'), \
            mock.patch(NAMESPACE + '.logging.error'), \
            mock.patch(NAMESPACE + '.error'):
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=None):
            result = create_originaldatatable(res[1], 'test')
        unittest.TestCase().assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value='select * from task'):
            result = create_originaldatatable(res[1], 'test')
        unittest.TestCase().assertEqual(result, 1)
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=sql):
            result = create_originaldatatable(res[1], 'test')
        unittest.TestCase().assertEqual(result, 0)
    db_manager.destroy(res)


def test_create_eventcounttable():
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    db_manager = DBManager()
    res = db_manager.create_table('runtime.db')
    create_eventcounttable(res[1], pmu_events)
    res[1].execute("drop table EventCount")
    db_manager.destroy(res)


def test_sql_insert_eventcounttable():
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    sql = "INSERT INTO EventCount SELECT function,module,callstack,common,pid,tid,core," \
          "SUM(CASE WHEN pmuevent='r8' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='ra' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='r9' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='rb' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='rc' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='rd' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='r54' THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent='r55' THEN pmucount ELSE 0 END) " \
          "FROM OriginalData GROUP BY function,module,callstack,common,pid,tid,core"
    result = sql_insert_eventcounttable(pmu_events)
    unittest.TestCase().assertEqual(result, sql)


def test_insert_eventcounttable():
    db_manager = DBManager()
    res = db_manager.create_table("test.db")
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    with mock.patch(NAMESPACE + '.sql_insert_eventcounttable', return_value='select * from task'), \
            mock.patch(NAMESPACE + '.logging.info'), \
            mock.patch(NAMESPACE + '.logging.error'):
        insert_eventcounttable(res[0], res[1], pmu_events)
    res[1].execute("CREATE TABLE IF NOT EXISTS EventCount(func text,module text,callstack text,common text,"
                   "pid INT,tid INT,core INT,r8 INT,ra INT,r9 INT,rb INT,rc INT,rd INT,r54 INT,r55 INT)")
    res[1].execute("CREATE TABLE IF NOT EXISTS OriginalData(common text,pid int,tid int,core int,"
                   "timestamp numeric,pmucount int,pmuevent char(10),ip text,function text,offset text,"
                   "module text,callstack text,replayid int)")
    with mock.patch(NAMESPACE + '.logging.info'):
        insert_eventcounttable(res[0], res[1], pmu_events)
    res[1].execute("drop table EventCount")
    res[1].execute("drop table OriginalData")
    db_manager.destroy(res)


def test_create_hotinstable():
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    db_manager = DBManager()
    res = db_manager.create_table("test.db")
    create_hotinstable(res[1], pmu_events)
    res[1].execute("drop table HotIns")
    db_manager.destroy(res)


def test_sql_insert_hotinstable():
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    sql = "INSERT INTO HotIns SELECT ip,SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "SUM(CASE WHEN pmuevent=? THEN pmucount ELSE 0 END)," \
          "pid,tid,core,function,module FROM OriginalData GROUP BY pid,tid,core,ip,function,module"
    result = sql_insert_hotinstable(pmu_events)
    unittest.TestCase().assertEqual(result, sql)


def test_insert_hotinstable():
    pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    db_manager = DBManager()
    res = db_manager.create_table("test.db")
    res[1].execute("CREATE TABLE IF NOT EXISTS HotIns  (r8 INT,ip TEXT,ra INT,r9 INT,rb INT,rc INT,"
                   "rd INT,r54 INT,r55 INT,pid INT,tid INT,core INT,function TEXT,module TEXT)")
    res[1].execute("CREATE TABLE IF NOT EXISTS OriginalData(common text,pid int,tid int,core int,"
                   "timestamp numeric,pmucount int,pmuevent char(10),ip text,function text,offset text,"
                   "module text,callstack text,replayid int)")
    with mock.patch(NAMESPACE + '.logging.info'), \
            mock.patch(NAMESPACE + '.logging.error'):
        insert_hotinstable(res[0], res[1], pmu_events)
        with mock.patch(NAMESPACE + '.sql_insert_hotinstable', return_value='select * from task'):
            insert_hotinstable(res[0], res[1], pmu_events)
    res[1].execute("drop table HotIns")
    res[1].execute("drop table OriginalData")
    db_manager.destroy(res)


def test_get_cpu_pmu_events():
    input_pmu_events = ['0x8'], ['0xa'], ['0x9'], ['0xb'], ['0xc'], ['0xd'], ['0x54'], ['0x55']
    output_pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
    with mock.patch(NAMESPACE + '.get_cpu_event_chunk', return_value=input_pmu_events):
        result = get_cpu_pmu_events({}, 'test')
    unittest.TestCase().assertEqual(result, output_pmu_events)


class TestParsingCPUData(unittest.TestCase):

    def test_init_cpu_db(self):
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.isdir', return_value=True), \
                mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=0)), \
                mock.patch('os.access', return_value=True):
            check = ParsingCPUData(CONFIG)
            check.init_cpu_db()

    def test_parsing_data_file(self):
        with DBOpen('test.db') as db_open:
            sql = "CREATE TABLE IF NOT EXISTS OriginalData(common text,pid int,tid int,core int," \
                  "timestamp numeric,pmucount int,pmuevent char(10),ip text,function text,offset text," \
                  "module text,callstack text,replayid int)"
            with mock.patch(NAMESPACE + ".logging.error"), \
                    mock.patch(NAMESPACE + ".logging.info"), \
                    mock.patch('sys.exit'), \
                    mock.patch(NAMESPACE + '.error'), \
                    mock.patch('os.path.getsize', return_value=100), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch('os.path.exists', return_value=False):
                with mock.patch(NAMESPACE + '.create_originaldatatable', return_value=1), \
                        mock.patch(NAMESPACE + '.ParsingCPUData._multiprocess', side_effect=OSError):
                    check = ParsingCPUData(CONFIG)
                    check.conn, check.curs = db_open.db_conn, db_open.db_curs
                    result = check.parsing_data_file(1, 'test')
                self.assertEqual(result, 1)
                with mock.patch(NAMESPACE + '.create_originaldatatable', return_value=1):
                    check = ParsingCPUData(CONFIG)
                    check.conn, check.curs = db_open.db_conn, db_open.db_curs
                    result = check.parsing_data_file(1, 'test')
                self.assertEqual(result, 1)
            with mock.patch(NAMESPACE + ".logging.error"), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.getsize', return_value=100), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                               return_value=sql), \
                    mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                    mock.patch(NAMESPACE + '.ParsingCPUData._multiprocess'):
                check = ParsingCPUData(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                result = check.parsing_data_file(1, 'test')
                self.assertEqual(result, 0)
                check = ParsingCPUData(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                result = check.parsing_data_file(1, 'test')
                self.assertEqual(result, 0)

    def test_create_other_table(self):
        pmu_events = ['r8', 'ra', 'r9', 'rb', 'rc', 'rd', 'r54', 'r55']
        with DBOpen('test.db') as db_open:
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                            return_value=False), \
                    mock.patch(NAMESPACE + '.get_cpu_pmu_events', return_value=pmu_events), \
                    mock.patch(NAMESPACE + '.create_eventcounttable', side_effect=sqlite3.Error), \
                    mock.patch(NAMESPACE + '.logging.error'):
                check = ParsingCPUData(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                check.create_other_table()

        with DBOpen('test.db') as db_open:
            with mock.patch(NAMESPACE + '.get_cpu_pmu_events', return_value=pmu_events), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                               return_value=False), \
                    mock.patch(NAMESPACE + '.create_eventcounttable'), \
                    mock.patch(NAMESPACE + '.insert_eventcounttable'), \
                    mock.patch(NAMESPACE + '.create_hotinstable'), \
                    mock.patch(NAMESPACE + '.insert_hotinstable'):
                check = ParsingCPUData(CONFIG)
                check.conn, check.curs = db_open.db_conn, db_open.db_curs
                check.create_other_table()

    def test_start_create_cpu_db(self):
        with mock.patch(NAMESPACE + '.ParsingCPUData.init_and_parsing',
                        side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.exception'), \
                mock.patch(NAMESPACE + '.error'):
            check = ParsingCPUData(CONFIG)
            check.start_create_cpu_db()

    def test_init_and_parsing(self):
        listdir = ['test1.slice_1', 'test2.slice_2']
        with mock.patch(NAMESPACE + ".logging.error"), \
                mock.patch(NAMESPACE + ".logging.info"), \
                mock.patch('sys.exit'), \
                mock.patch(NAMESPACE + '.error'):
            with mock.patch('os.path.exists', return_value=False):
                check = ParsingCPUData(CONFIG)
                result = check.init_and_parsing()
            self.assertEqual(result, None)
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch('os.listdir', return_value=listdir), \
                    mock.patch(NAMESPACE + '.get_file_name_pattern_match',
                               return_value=True), \
                    mock.patch(NAMESPACE + '.is_valid_original_data',
                               return_value=True):
                with mock.patch(NAMESPACE + '.ParsingCPUData.get_cpu_id',
                                return_value=''):
                    check = ParsingCPUData(CONFIG)
                    result = check.init_and_parsing()
                self.assertEqual(result, None)
                with mock.patch(NAMESPACE + '.ParsingCPUData.get_cpu_id',
                                return_value=1), \
                        mock.patch(NAMESPACE + '.ParsingCPUData.init_cpu_db'), \
                        mock.patch('os.path.getsize', return_value=1):
                    with mock.patch(NAMESPACE + '.ParsingCPUData.parsing_data_file',
                                    return_value=1):
                        check = ParsingCPUData(CONFIG)
                        result = check.init_and_parsing()
                    self.assertEqual(result, None)
                    with mock.patch(NAMESPACE + '.ParsingCPUData.parsing_data_file',
                                    return_value=0), \
                            mock.patch(NAMESPACE + '.ParsingCPUData.create_other_table'):
                        check = ParsingCPUData(CONFIG)
                        result = check.init_and_parsing()
                    self.assertEqual(result, None)

    def test_ms_run(self):
        db_manager = DBManager()
        res = db_manager.create_table("test.db")
        with mock.patch(NAMESPACE + '.ParsingCPUData.start_create_cpu_db',
                        side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingCPUData(CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.ParsingCPUData.start_create_cpu_db'):
            check = ParsingCPUData(CONFIG)
            check.conn, check.curs = res[0], res[1]
            check.ms_run()

    def test_get_cpu_id(self):
        sample_config = CONFIG
        cpu_type = 'ai'
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.error'):
            with mock.patch('os.path.exists', return_value=False):
                result = ParsingCPUData.get_cpu_id(sample_config, cpu_type)
            self.assertEqual(result, '')
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.join', return_value='test\\test'), \
                    mock.patch('os.path.realpath', return_value='test\\test'):
                InfoConfReader()._info_json = {'DeviceInfo': None}
                result = ParsingCPUData.get_cpu_id(sample_config, cpu_type)
                self.assertEqual(result, '')
                InfoConfReader()._info_json = {'DeviceInfo': [{'ai_cpu': 'a, 2, 3, 4, 5, 6, 7'}]}
                result = ParsingCPUData.get_cpu_id(sample_config, cpu_type)
                self.assertEqual(result, '')
                InfoConfReader()._info_json = {'DeviceInfo': [{'ai_cpu': '1,2,3,4,5,6,7'}]}
                result = ParsingCPUData.get_cpu_id(sample_config, cpu_type)
                self.assertEqual(result, '1,2,3,4,5,6,7')


if __name__ == '__main__':
    unittest.main()
