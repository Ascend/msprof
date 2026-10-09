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
from collections import OrderedDict
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.roce_model import RoceModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.hardware.roce_model'


class TestParsingMemoryData(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.RoceModel.insert_data_to_db'):
            InfoConfReader()._sample_json = {'devices': '0'}
            InfoConfReader()._info_json = {'devices': '0'}
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.flush([])

    def test_report_data(self):
        devcie_list = ['5']
        db_manager = DBManager()
        res = db_manager.create_table('roce.db')
        InfoConfReader()._info_json = {'devices': '0'}
        InfoConfReader()._sample_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        return_value=True), \
                mock.patch(NAMESPACE + '.RoceModel.create_roce_data_report'), \
                mock.patch(NAMESPACE + '.RoceModel.create_roce_tree_data'), \
                mock.patch(NAMESPACE + '.RoceModel.get_func_list',
                           return_value=[(0,)]), \
                mock.patch(NAMESPACE + '.RoceModel.create_rocereceivesend_table'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.report_data(devcie_list)
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.RoceModel.get_func_list',
                        side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.report_data(devcie_list)
        self.assertEqual(result, None)
        db_manager.destroy(res)

    def test_create_roce_data_report(self):
        create_roce_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                          "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                          "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                          "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into RoceOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('roce.db', create_roce_sql, insert_sql, data)

        create_sql = "CREATE TABLE IF NOT EXISTS RoceReportData(device_id INTEGER,duration TEXT," \
                     "bandwidth TEXT,rxbandwidth TEXT,txbandwidth TEXT,rxpacket TEXT," \
                     "rxerrorrate TEXT,rxdroppedrate TEXT,txpacket TEXT,txerrorrate TEXT," \
                     "txdroppedrate TEXT,funcid INTEGER)"
        InfoConfReader()._info_json = {'devices': '0'}
        InfoConfReader()._sample_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist',
                        return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                           return_value=create_sql):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_roce_data_report()
        with mock.patch(NAMESPACE + '.RoceModel._do_roce_data_report',
                        side_effect=ValueError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.create_roce_data_report()
        res[1].execute('insert into RoceReportData values (1,5,3,6,8,7,1,2,56,9,4,12)')
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=create_sql), \
                mock.patch(NAMESPACE + '.RoceModel.get_roce_report_data'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_roce_data_report()
        res[1].execute('insert into RoceReportData values (1,2,3,4,5,6,7,8,9,10,11,12)')
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=create_sql), \
                mock.patch(NAMESPACE + '.RoceModel.calculate_roce_report_data'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.create_roce_data_report()
        res[1].execute('drop table RoceReportData')
        db_manager.destroy(res)

    def test_create_roce_tree_data(self):
        db_manager = DBManager()
        res = db_manager.create_table('roce.db')
        InfoConfReader()._info_json = {'devices': '0'}
        InfoConfReader()._sample_json = {'devices': '0'}
        check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
        check.curr_file_name = 'abc'
        with mock.patch(NAMESPACE + '.logging.error'):
            check.curr_file_name = 'roce.data.5.slice_0'
            check.conn, check.cur = res[0], res[1]
            check.create_roce_tree_data()
        res[1].execute("CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER,"
                       "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL,"
                       "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL,"
                       "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)")
        check.curr_file_name = 'roce.data.5.slice_0'
        check.conn, check.cur = res[0], res[1]
        check.create_roce_tree_data()
        res[1].execute('drop table RoceOriginalData')
        res[1].execute('drop table RoceTreeData')
        db_manager.destroy(res)

    def test_get_func_list(self):
        InfoConfReader()._info_json = {'devices': '0'}
        InfoConfReader()._sample_json = {'devices': '0'}
        create_roce_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                          "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                          "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                          "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into RoceOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('roce.db', create_roce_sql, insert_sql, data)
        check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
        check.conn, check.cur = res[0], res[1]
        result = check.get_func_list(5)
        self.assertEqual(result, [(0,)])
        res[1].execute('drop table RoceOriginalData')
        with mock.patch(NAMESPACE + '.logging.error'):
            check.conn, check.cur = res[0], res[1]
            result = check.get_func_list(5)
            self.assertEqual(result, [])
        db_manager.destroy(res)

    def test_get_roce_report_data(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        InfoConfReader()._info_json = {'devices': '0'}
        create_roce_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                          "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                          "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                          "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into RoceOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = (
            (5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),
            (5, 0, 19, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0)
        )
        db_manager = DBManager()
        res = db_manager.create_table('roce.db', create_roce_sql, insert_sql, data)
        res[1].execute("create table if not exists RoceReportData(device_id INTEGER,duration TEXT,"
                       "bandwidth TEXT,rxBandwidth TEXT,txBandwidth TEXT,rxPacket TEXT,"
                       "rxErrorRate TEXT,rxDroppedRate TEXT,txPacket TEXT,txErrorRate TEXT,"
                       "txDroppedRate TEXT,funcId INTEGER)")
        check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.get_roce_report_data((5,), 0)
        res[1].execute('drop table RoceOriginalData')
        res[1].execute('drop table RoceReportData')
        db_manager.destroy(res)

    def test_calculate_roce_report_data(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        InfoConfReader()._info_json = {'devices': '0'}
        create_roce_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                          "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                          "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                          "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into RoceOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = (
            (5, 0, 1, 1, 3, 4, 5, 10, 7, 8, 9, 4, 5, 6, 1, 3, 0),
            (5, 0, 19, 1, 3, 4, 5, 10, 7, 8, 9, 4, 5, 6, 1, 3, 0)
        )
        db_manager = DBManager()
        res = db_manager.create_table('roce.db', create_roce_sql, insert_sql, data)
        res[1].execute("create table if not exists RoceReportData(device_id INTEGER,duration TEXT,"
                       "bandwidth TEXT,rxBandwidth TEXT,txBandwidth TEXT,rxPacket TEXT,rxErrorRate TEXT,"
                       "rxDroppedRate TEXT,txPacket TEXT,txErrorRate TEXT,"
                       "txDroppedRate TEXT,funcId INTEGER)")
        check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.calculate_roce_report_data((5,), 0)
        res[1].execute('drop table RoceOriginalData')
        res[1].execute('drop table RoceReportData')
        db_manager.destroy(res)

    def test_create_receivesend_db(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        InfoConfReader()._info_json = {'devices': '0'}
        target_data = [
            ('5', 176168008028160.0, 0.0, 0.0, 0, 0, 0.0, 0, 0, 0, 0),
            ('5', 176168112015020.0, 0.0, 0.0, 0, 0, 0.0, 0, 0, 0, 0)
        ]
        with DBOpen('rocereceivesend_table.db') as roce_receive_open:
            with DBOpen('roce.db') as roce_open:
                with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
                        mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                                   return_value=(roce_receive_open.db_conn, roce_receive_open.db_curs)), \
                        mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                        mock.patch(NAMESPACE + '.logging.error'):
                    check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
                    check.conn, check.cur = roce_open.db_conn, roce_open.db_curs
                    check.curr_file_name = 'roce.data.5.slice_0'
                    result = check.create_receivesend_db(target_data)
                self.assertEqual(result, None)

    def test_create_rocereceivesend_data(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        InfoConfReader()._info_json = {'devices': '0'}
        create_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                     "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                     "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                     "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        data = ((5, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)

        with DBOpen('roce.db') as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("RoceOriginalData", data)
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = db_open.db_conn, db_open.db_curs
            result = check.create_rocereceivesend_data(0)
        self.assertEqual(len(result), 2)

        with DBOpen('roce.db') as db_open:
            with mock.patch(NAMESPACE + '.logging.error'):
                check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
                check.conn, check.cur = db_open.db_conn, db_open.db_curs
                result = check.create_rocereceivesend_data(0)
        self.assertEqual(result,
                         (OrderedDict([('rxPacket/s', []), ('rxError rate', []), ('rxDropped rate', [])]),
                          OrderedDict([('txPacket/s', []), ('txError rate', []), ('txDropped rate', [])])))

    def test_create_rocereceivesend_table(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        InfoConfReader()._info_json = {'devices': '0'}
        create_sql = "CREATE TABLE IF NOT EXISTS RoceOriginalData(device_id INTEGER,replayid INTEGER," \
                     "timestamp REAL,bandwidth INTEGER,rxpacket REAL,rxbyte REAL,rxpackets REAL," \
                     "rxbytes REAL,rxerrors REAL,rxdropped REAL,txpacket REAL,txbyte REAL," \
                     "txpackets REAL,txbytes REAL,txerrors REAL,txdropped REAL,funcid INTEGER)"
        insert_sql = "insert into RoceOriginalData values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        data = ((0, 0, 1, 1, 3, 4, 5, 6, 7, 8, 9, 4, 5, 6, 1, 3, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('roce.db', create_sql, insert_sql, data)
        rx_data = OrderedDict([('rxPacket/s', [(0.0,)]), ('rxError rate', [(None,)]), ('rxDropped rate', [(None,)])])
        tx_data = OrderedDict([('txPacket/s', [(0.0,)]), ('txError rate', [(None,)]), ('txDropped rate', [(None,)])])
        with mock.patch(NAMESPACE + '.RoceModel.create_rocereceivesend_data',
                        return_value=(rx_data, tx_data)), \
                mock.patch(NAMESPACE + '.RoceModel.create_receivesend_db'):
            check = RoceModel('test', 'roce.db', ['RoceOriginalData'])
            check.conn, check.cur = res[0], res[1]
            result = check.create_rocereceivesend_table(0)
            self.assertEqual(result, None)
        res[1].execute("drop table RoceOriginalData")
        db_manager.destroy(res)


if __name__ == '__main__':
    unittest.main()
