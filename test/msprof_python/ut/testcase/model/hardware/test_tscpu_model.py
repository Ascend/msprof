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
from msmodel.hardware.tscpu_model import TscpuModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.tscpu_model'


class TestTscpuModel(unittest.TestCase):

    def test_init(self):
        db_manager = DBManager()
        res = db_manager.create_table('tscpu_0.db')
        with mock.patch(NAMESPACE + '.TscpuModel.create_table'), \
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            self.assertEqual(check.init(), True)
        with mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            self.assertEqual(check.init(), False)
        db_manager.destroy(res)

    def test_flush(self):
        db_manager = DBManager()
        res = db_manager.create_table('tscpu_0.db')
        with mock.patch(NAMESPACE + '.TscpuModel.insert_data_to_db'), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.TscpuModel._create_ts_event_count_table'), \
                mock.patch(NAMESPACE + '.TscpuModel._create_ts_hot_ins_table'), \
                mock.patch(NAMESPACE + '.TscpuModel._insert_ts_hot_ins'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.flush([])
        db_manager.destroy(res)

    def test_create_ts_event_count_table(self):
        db_manager = DBManager()
        res = db_manager.create_table("tscpu_0.db")
        with mock.patch(NAMESPACE + '.TscpuModel.get_pmu_event_name',
                        return_value=('0x11', '0x8', '0x0')):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check._create_ts_event_count_table()
        res[1].execute("drop table EventCount")
        db_manager.destroy(res)

    def test_create_ts_hot_ins_table(self):
        db_manager = DBManager()
        res = db_manager.create_table("tscpu_0.db")
        with mock.patch(NAMESPACE + '.TscpuModel.get_pmu_event_name',
                        return_value=('0x11', '0x8', '0x0')):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check._create_ts_hot_ins_table()
        res[1].execute("drop table HotIns")
        db_manager.destroy(res)

    def test_insert_ts_event_count(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TsOriginalData(replayid INTEGER,timestamp numeric," \
                     "pc TEXT,callstack TEXT,event TEXT,count INTEGER,function TEXT)"
        insert_sql = "insert into TsOriginalData values(?,?,?,?,?,?,?)"
        data = ((0, 111542377976, '0xfb04e10', '', '0x11', 768001, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x8', 176049, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'))
        db_manager = DBManager()
        res = db_manager.create_table('tscpu_0.db', create_sql, insert_sql, data)
        sql = "CREATE TABLE IF NOT EXISTS  EventCount(func text,callstack text,module text," \
              "common text,pid INT,tid INT,core INT,r11 INT,r8 INT,r0 INT)"
        res[1].execute(sql)
        with mock.patch(NAMESPACE + '.TscpuModel.get_pmu_event_name',
                        return_value=('0x11', '0x8', '0x0')):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check._insert_ts_event_count()
        res[1].execute("drop table EventCount")
        res[1].execute("drop table TsOriginalData")
        db_manager.destroy(res)

    def test_insert_ts_hot_ins(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TsOriginalData(replayid INTEGER,timestamp numeric," \
                     "pc TEXT,callstack TEXT,event TEXT,count INTEGER,function TEXT)"
        insert_sql = "insert into TsOriginalData values(?,?,?,?,?,?,?)"
        data = ((0, 111542377976, '0xfb04e10', '', '0x11', 768002, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x8', 176049, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'))
        db_manager = DBManager()
        res = db_manager.create_table('tscpu_0.db', create_sql, insert_sql, data)
        sql = "CREATE TABLE IF NOT EXISTS  HotIns(ip text,function text,module text,pid INT,tid INT," \
              "core INT,r11 INT,r8 INT,r0 INT)"
        res[1].execute(sql)
        with mock.patch(NAMESPACE + '.TscpuModel.get_pmu_event_name',
                        return_value=('0x11', '0x8', '0x0')):
            InfoConfReader()._info_json = {'devices': '0'}
            check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check._insert_ts_hot_ins()
        res[1].execute("drop table HotIns")
        res[1].execute("drop table TsOriginalData")
        db_manager.destroy(res)

    def test_get_pmu_event_name(self):
        create_sql = "CREATE TABLE IF NOT EXISTS TsOriginalData(replayid INTEGER,timestamp numeric," \
                     "pc TEXT,callstack TEXT,event TEXT,count INTEGER,function TEXT)"
        insert_sql = "insert into TsOriginalData values(?,?,?,?,?,?,?)"
        data = ((0, 111542377976, '0xfb04e10', '', '0x11', 768000, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x8', 176049, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'),
                (0, 111542377976, '0xfb04e10', '', '0x0', 0, '0xfb04e10'))
        db_manager = DBManager()
        res = db_manager.create_table('tscpu_0.db', create_sql, insert_sql, data)
        InfoConfReader()._info_json = {'devices': '0'}
        check = TscpuModel('test', 'tscpu_0.db', ['TsOriginalData'])
        check.conn, check.cur = res[0], res[1]
        result = check.get_pmu_event_name()
        self.assertEqual(result, ('0x11', '0x8', '0x0'))
        res[1].execute('drop table TsOriginalData')
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
