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
from msmodel.hardware.nic_model import NicModel
from profiling_bean.db_dto.nic_dto import NicDto
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.nic_model'


class TestParsingMemoryData(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.NicModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.flush([])

    def test_report_data(self):
        device_list = ['5']
        db_manager = DBManager()
        res = db_manager.create_table('nic.db')
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        return_value=True), \
                mock.patch(NAMESPACE + '.NicModel.create_nic_data_report'), \
                mock.patch(NAMESPACE + '.NicModel.create_nic_tree_data'), \
                mock.patch(NAMESPACE + '.NicModel.get_func_list',
                           return_value=[(0,)]), \
                mock.patch(NAMESPACE + '.NicModel.create_nicreceivesend_table'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.report_data(device_list)
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.report_data(device_list)
        self.assertEqual(result, None)
        db_manager.destroy(res)

    def test_create_nic_data_report(self):
        create_nic_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                         "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                         "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                         "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOrigin" \
                     "alData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_nic_sql, insert_sql, data)
        create_sql = "CREATE TABLE if not exists NicReportData(device_id INTEGER,duration TEXT,bandwidth TEXT," \
                     "rxbandwidth TEXT,txbandwidth TEXT,rxpacket TEXT,rxerrorrate TEXT,rxdroppedrate TEXT," \
                     "txpacket TEXT,txerrorrate TEXT,txdroppedrate TEXT,funcid INTEGER)"
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                           return_value=create_sql):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_nic_data_report()
        with mock.patch(NAMESPACE + '.NicModel._create_nic_data_report_per_device',
                        side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.create_nic_data_report()
        res[1].execute('insert into NicReportData values (1,5,3,6,8,7,1,2,56,9,4,12)')
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=create_sql), \
                mock.patch(NAMESPACE + '.NicModel.get_nic_report_data'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_nic_data_report()
        res[1].execute('insert into NicReportData values (1,2,3,4,5,6,7,8,9,10,11,12)')
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=create_sql), \
                mock.patch(NAMESPACE + '.NicModel.calculate_nic_report_data'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_nic_data_report()
        res[1].execute('drop table NicReportData')
        db_manager.destroy(res)

    def test_create_nic_tree_data(self):
        db_manager = DBManager()
        res = db_manager.create_table('nic.db')
        InfoConfReader()._info_json = {'devices': '0'}
        check = NicModel('test', 'nic.db', ['NicOriginalData'])
        check.curr_file_name = 'abc'
        with mock.patch(NAMESPACE + '.logging.error'):
            check.curr_file_name = 'nic.data.5.slice_0'
            check.conn, check.cur = res[0], res[1]
            check.create_nic_tree_data()
        res[1].execute("CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER,"
                       "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL,"
                       "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL,"
                       "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)")
        check.curr_file_name = 'nic.data.5.slice_0'
        check.conn, check.cur = res[0], res[1]
        check.create_nic_tree_data()
        res[1].execute('drop table NicOriginalData')
        res[1].execute('drop table NicTreeData')
        db_manager.destroy(res)

    def test_get_func_list(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_nic_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                         "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                         "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                         "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOrigin" \
                     "alData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_nic_sql, insert_sql, data)
        check = NicModel('test', 'nic.db', ['NicOriginalData'])
        check.conn, check.cur = res[0], res[1]
        result = check.get_func_list(5)
        self.assertEqual(result, [(0,)])
        res[1].execute('drop table NicOriginalData')
        with mock.patch(NAMESPACE + '.logging.error'):
            check.conn, check.cur = res[0], res[1]
            result = check.get_func_list(5)
            self.assertEqual(result, [])
        db_manager.destroy(res)

    def test_get_nic_report_data(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_nic_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                         "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                         "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                         "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),
                (5, 0, 19, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0))
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_nic_sql, insert_sql, data)
        res[1].execute("create table if not exists NicReportData(device_id INTEGER,duration TEXT,"
                       "bandwidth TEXT,rxBandwidth TEXT,txBandwidth TEXT,rxPacket TEXT,"
                       "rxErrorRate TEXT,rxDroppedRate TEXT,txPacket TEXT,txErrorRate TEXT,"
                       "txDroppedRate TEXT,funcId INTEGER)")
        check = NicModel('test', 'nic.db', ['NicOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.get_nic_report_data((5,), 0)
        res[1].execute('drop table NicOriginalData')
        res[1].execute('drop table NicReportData')
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check.conn, check.cur = res[0], res[1]
            check.get_nic_report_data((5,), 0)

    def test_calculate_nic_report_data(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_nic_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                         "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                         "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                         "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 10, 7, 8, 9, 4, 5, 6, 1, 3, 0),
                (5, 0, 19, 1, 3, 4, 5, 10, 7, 8, 9, 4, 5, 6, 1, 3, 0),
                (6, 0, 1, 1, 0, 4, 5, 10, 7, 8, 0, 4, 5, 6, 1, 3, 0),
                (6, 0, 19, 1, 0, 4, 5, 10, 7, 8, 0, 4, 5, 6, 1, 3, 0))
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_nic_sql, insert_sql, data)
        res[1].execute("create table if not exists NicReportData(device_id INTEGER,duration TEXT,"
                       "bandwidth TEXT,rxBandwidth TEXT,txBandwidth TEXT,rxPacket TEXT,rxErrorRate TEXT,"
                       "rxDroppedRate TEXT,txPacket TEXT,txErrorRate TEXT,"
                       "txDroppedRate TEXT,funcId INTEGER)")
        check = NicModel('test', 'nic.db', ['NicOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.calculate_nic_report_data((5,), 0)
        check.calculate_nic_report_data((6,), 0)
        res[1].execute('drop table NicOriginalData')
        res[1].execute('drop table NicReportData')
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check.conn, check.cur = res[0], res[1]
            check.calculate_nic_report_data((5,), 0)

    def test_create_receivesend_db(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res_receive = db_manager.create_table('nicreceivesend_table.db')
        res = db_manager.create_table('nic.db')

        target_data = [('5', 176168008028160.0, 0.0, 0.0, 0, 0, 0.0, 0, 0, 0, 0),
                       ('5', 176168112015020.0, 0.0, 0.0, 0, 0, 0.0, 0, 0, 0, 0)]
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'),\
                mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res_receive), \
                mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.curr_file_name = 'nic.data.5.slice_0'
            result = check.create_receivesend_db(target_data)
        self.assertEqual(result, None)
        db_manager.destroy(res)
        db_manager.destroy(res_receive)


    def test_create_nicreceivesend_data(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                     "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                     "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                     "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_sql, insert_sql, data)
        check = NicModel('test', 'nic.db', ['NicOriginalData'])
        check.conn, check.cur = res[0], res[1]

        nic_dto = NicDto()
        nic_dto.rx_packet = 0
        nic_dto.rx_error_rate = 0
        nic_dto.rx_dropped_rate = 0
        nic_dto.tx_packet = 0
        nic_dto.tx_error_rate = 0
        nic_dto.tx_dropped_rate = 0
        nic_dto.timestamp = 123
        nic_dto.bandwidth = 0
        nic_dto.rxbyte = 0
        nic_dto.txbyte = 0

        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[nic_dto, nic_dto]):
            result = check.create_nicreceivesend_data(0)
        self.assertEqual(len(result), 2)
        res[1].execute("drop table NicOriginalData")
        db_manager.destroy(res)

    def test_create_nicreceivesend_table(self):
        InfoConfReader()._info_json = {'devices': '0'}
        create_sql = "CREATE TABLE IF NOT EXISTS NicOriginalData(device_id INTEGER,replayid INTEGER," \
                     "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                     "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                     "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into NicOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((0, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('nic.db', create_sql, insert_sql, data)

        nic_dto = NicDto()
        nic_dto.rx_packet = 0
        nic_dto.rx_error_rate = 0
        nic_dto.rx_dropped_rate = 0
        nic_dto.tx_packet = 0
        nic_dto.tx_error_rate = 0
        nic_dto.tx_dropped_rate = 0
        nic_dto.timestamp = 123
        nic_dto.bandwidth = 0
        nic_dto.rxbyte = 0
        nic_dto.txbyte = 0

        with mock.patch(NAMESPACE + '.NicModel.create_nicreceivesend_data',
                        return_value=(nic_dto, nic_dto)), \
                mock.patch(NAMESPACE + '.NicModel.create_receivesend_db'):
            check = NicModel('test', 'nic.db', ['NicOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.create_nicreceivesend_table(0)
            self.assertEqual(result, None)
        res[1].execute("drop table NicOriginalData")
        db_manager.destroy(res)
