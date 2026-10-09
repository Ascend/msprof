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
from msmodel.hardware.hccs_model import HccsModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.hccs_model'


class TestHccsModel(unittest.TestCase):

    def test_drop_tab(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res = db_manager.create_table('hccs.db')
        check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.drop_tab()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.drop_tab()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HccsModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
            check.flush([])

    def test_insert_metrics(self):
        db_manager = DBManager()
        res = db_manager.create_table('hccs.db')
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.metric_data = [(5, 2, 2, 2), (5, 0, 0, 0)]
            check.insert_metrics(5)
        res[1].execute("CREATE TABLE IF NOT EXISTS HCCSEventsData(device_id INT,timestamp REAL,"
                       "txthroughput INT,rxthroughput INT)")
        check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.metric_data = [(5, 2, 2, 2), (5, 0, 0, 0)]
        check.insert_metrics(5)
        res[1].execute("drop table HCCSEventsData")
        db_manager.destroy(res)

    def test_calculate_metrics(self):
        db_manager = DBManager()
        res = db_manager.create_table('hccs.db')
        check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check._calculate_metrics(5)
        db_manager.destroy(res)
        original_sql = "CREATE TABLE IF NOT EXISTS HCCSOriginalData(device_id INT,timestamp REAL," \
                       "txamount INT,rxamount INT)"
        insert_sql = "insert into {} values (?,?,?,?)".format('HCCSOriginalData')
        data = ((5, 2, 2, 2), (5, 0, 0, 0),
                (5, 1, 1, 1), (5, 3, 3, 3))
        db_manager = DBManager()
        res = db_manager.create_table('hccs.db', original_sql, insert_sql, data)
        check = HccsModel('test', 'hccs.db', ['HCCSEventsData', 'HCCSOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check._calculate_metrics(5)
        res[1].execute("drop table HCCSOriginalData")
        db_manager.destroy(res)
