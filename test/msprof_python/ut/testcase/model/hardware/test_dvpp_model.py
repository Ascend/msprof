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
from msmodel.hardware.dvpp_model import DvppModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.dvpp_model'


class TestDvppModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DvppModel.insert_data_to_db'),\
                mock.patch(NAMESPACE + '.DvppModel._create_index'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
            check.flush([])

    def test_create_index(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData(device_id TEXT,replayid TEXT," \
                     "timestamp REAL,dvppid TEXT,enginetype TEXT,engineid TEXT,alltime REAL," \
                     "allframe REAL,allutilization TEXT,proctime REAL,procframe REAL," \
                     "procutilization TEXT,lasttime REAL,lastframe REAL)"
        insert_sql = "insert into DvppOriginalData values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 171617, 8, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('peripheral.db', create_sql, insert_sql, data)
        check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check._create_index()
        res[1].execute("drop table DvppOriginalData")
        db_manager.destroy(res)

    def test_report_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        side_effect=TypeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
            check.report_data(True)
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData(device_id TEXT,replayid TEXT," \
                     "timestamp REAL,dvppid TEXT,enginetype TEXT,engineid TEXT,alltime REAL," \
                     "allframe REAL,allutilization TEXT,proctime REAL,procframe REAL," \
                     "procutilization TEXT,lasttime REAL,lastframe REAL)"
        insert_sql = "insert into DvppOriginalData values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 171617, 8, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('peripheral.db', create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DvppModel._create_dvpp_report_data'), \
                mock.patch(NAMESPACE + '.DvppModel._create_dvpp_tree_data'):
            check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.has_dvpp_id = None
            check.report_data(False)
        res[1].execute("drop table DvppOriginalData")
        db_manager.destroy(res)


    def test_create_dvpp_tree_data(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData(device_id TEXT,replayid TEXT," \
                     "timestamp REAL,dvppid TEXT,enginetype TEXT,engineid TEXT,alltime REAL," \
                     "allframe REAL,allutilization TEXT,proctime REAL,procframe REAL," \
                     "procutilization TEXT,lasttime REAL,lastframe REAL)"
        insert_sql = "insert into DvppOriginalData values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 171617, 8, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('peripheral.db', create_sql, insert_sql, data)
        check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.has_dvpp_id = None
        check._create_dvpp_tree_data()
        res[1].execute("drop table DvppOriginalData")
        res[1].execute("drop table DvppTreeData")
        db_manager.destroy(res)

    def test_create_dvpp_report_data(self):
        create_report_sql = "CREATE TABLE IF NOT EXISTS DvppReportData(dvppid TEXT,device_id TEXT," \
                            "enginetype TEXT,engineid TEXT,alltime TEXT,allframe TEXT,allutilization TEXT)"
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData(device_id TEXT,replayid TEXT," \
                     "timestamp REAL,dvppid TEXT,enginetype TEXT,engineid TEXT,alltime REAL," \
                     "allframe REAL,allutilization TEXT,proctime REAL,procframe REAL," \
                     "procutilization TEXT,lasttime REAL,lastframe REAL)"
        insert_sql = "insert into DvppOriginalData values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 171617, 8, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('peripheral.db', create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=create_report_sql), \
                mock.patch(NAMESPACE + '.DvppModel._DvppModel__get_dvpp_data_by_dvpp_id'):
            check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check._create_dvpp_report_data(True)
        res[1].execute("drop table DvppOriginalData")
        res[1].execute("drop table DvppReportData")
        res[0].commit()
        db_manager.destroy(res)

    def test_get_dvpp_data_by_dvpp_id(self):
        target_data = [('8', '5', '0', '3', 0.0, 0.0, '0.0%')]
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData(device_id TEXT,replayid TEXT," \
                     "timestamp REAL,dvppid TEXT,enginetype TEXT,engineid TEXT,alltime REAL," \
                     "allframe REAL,allutilization TEXT,proctime REAL,procframe REAL," \
                     "procutilization TEXT,lasttime REAL,lastframe REAL)"
        insert_sql = "insert into DvppOriginalData values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 171617, 9, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('peripheral.db', create_sql, insert_sql, data)
        check = DvppModel('test', 'dvpp.db', ['DvppReportData', 'DvppOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check._DvppModel__get_dvpp_data_by_dvpp_id(('5',), ('9',), target_data)
        self.assertEqual(target_data, [('8', '5', '0', '3', 0.0, 0.0, '0.0%'),
                                       ('9', '5', '0', '3', 0.0, 0.0, '0')])
        res[1].execute("insert into DvppOriginalData values "
                       "(5, 0, 180000, 9, 0, 3, 0, 0, 0 % 100, 0, 0, 0 % 100, 0, 0)")
        check._DvppModel__get_dvpp_data_by_dvpp_id(('5',), ('9',), target_data)
        self.assertEqual(target_data, [('8', '5', '0', '3', 0.0, 0.0, '0.0%'),
                                       ('9', '5', '0', '3', 0.0, 0.0, '0'),
                                       ('9', '5', '0', '3', 0.0, 0.0, '0')])
        res[1].execute("drop table DvppOriginalData")
        res[0].commit()
        db_manager.destroy(res)
