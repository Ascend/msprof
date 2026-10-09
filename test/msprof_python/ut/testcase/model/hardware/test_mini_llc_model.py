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
from constant.constant import CONFIG, INFO_JSON
from msmodel.hardware.mini_llc_model import MiniLlcModel, cal_core2cpu
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.mini_llc_model'


class TestMiniLlcModel(unittest.TestCase):
    table_list = ["LLCOriginalData", "LLCEvents", "LLCMetrics", "LLCMetricData", "LLCCapacity"]

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.create_table()

    def test_drop_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        check = MiniLlcModel('test', 'llc.db', self.table_list)
        check.cur, check.conn = res[1], res[0]
        check.drop_tab()
        db_manager.destroy(res)

    def test_flush(self):
        with mock.patch(NAMESPACE + '.MiniLlcModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.flush({})

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.MiniLlcModel.calculate_bandwidth_data'), \
                mock.patch(NAMESPACE + '.MiniLlcModel.calculate_capacity_data'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.calculate({})

    def test_drop_tab(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res = db_manager.create_table('llc.db')
        check = MiniLlcModel('test', 'llc.db', self.table_list)
        check.conn, check.cur = res[0], res[1]
        check.drop_tab()
        db_manager.destroy(res)

    def test_calculate_bandwidth_data(self):
        create_metric_sql = "CREATE TABLE IF NOT EXISTS LLCMetricData(device_id INT,replayid INT," \
                            "timestamp REAL,read_allocate REAL,read_noallocate REAL,read_hit REAL," \
                            "write_allocate REAL,write_noallocate REAL,write_hit REAL)"
        insert_sql = "insert into LLCMetricData values(?,?,?,?,?,?,?,?,?)"
        data = ((7, 0, 1466.726545742, 166393, 21.0, 151150.0, 81601.0, 1.0, 0.0),
                (7, 0, 0, 0, 21.0, 151150.0, 81601.0, 1.0, 0.0),
                (7, 0, 1466.746695638, 166393, 21.0, 151150.0, 81601.0, 1.0, 0.0),
                (7, 0, 1466.807142096, 166393, 21.0, 151150.0, 0, 0, 0.0))
        db_manager = DBManager()
        res = db_manager.create_table('llc.db', create_metric_sql, insert_sql, data)
        res[1].execute("CREATE TABLE IF NOT EXISTS LLCBandwidth(device_id INT,timestamp REAL,"
                       "read_hit_rate REAL,read_total REAL,read_hit REAL,write_hit_rate REAL,"
                       "write_total REAL,write_hit REAL)")
        CONFIG['llc_profiling'] = 'bandwidth'
        check = MiniLlcModel('test', 'llc.db', self.table_list)
        check.conn, check.cur = res[0], res[1]
        check.calculate_bandwidth_data('bandwidth')
        with mock.patch(NAMESPACE + '.MiniLlcModel.get_bandwidth_insert_data',
                        side_effect=SystemError):
            CONFIG['llc_profiling'] = 'bandwidth'
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.calculate_bandwidth_data('bandwidth')
        res[1].execute("drop table LLCMetricData")
        res[1].execute("drop table LLCBandwidth")
        db_manager.destroy(res)

    def test_calculate_read_bandwidth(self):
        with mock.patch(NAMESPACE + '.logging.error'):
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.calculate_read_bandwidth(1, 0)

    def test_calculate_write_bandwidth(self):
        with mock.patch(NAMESPACE + '.logging.error'):
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.calculate_write_bandwidth(1, 0)

    def test_calculate_capacity_data(self):
        create_dsid_sql = "CREATE TABLE IF NOT EXISTS LLCDsidData(device_id INT,replayid INT," \
                          "timestamp REAL,dsid0 REAL,dsid1 REAL,dsid2 REAL,dsid3 REAL,dsid4 REAL," \
                          "dsid5 REAL,dsid6 REAL,dsid7 REAL)"
        insert_sql = "insert into LLCDsidData values(?,?,?,?,?,?,?,?,?,?,?)"
        data = ((0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        db_manager = DBManager()
        res = db_manager.create_table('llc.db', create_dsid_sql, insert_sql, data)
        res[1].execute("CREATE TABLE IF NOT EXISTS LLCCapacity(device_id INT,timestamp REAL,"
                       "ctrlcpu REAL,aicpu REAL)")
        CONFIG['llc_profiling'] = 'capacity'
        check = MiniLlcModel('test', 'llc.db', self.table_list)
        check.conn, check.cur = res[0], res[1]
        check.calculate_capacity_data('capacity')
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.cal_core2cpu', side_effect=TypeError):
            CONFIG['llc_profiling'] = 'capacity'
            check = MiniLlcModel('test', 'llc.db', self.table_list)
            check.conn, check.cur = res[0], res[1]
            check.calculate_capacity_data('capacity')
        res[1].execute("drop table LLCCapacity")
        res[1].execute("drop table LLCDsidData")
        db_manager.destroy(res)


def test_cal_core2cpu():
    with mock.patch('os.path.isfile', return_value=True):
        InfoConfReader()._info_json = INFO_JSON
        result = cal_core2cpu('test', 0)
        unittest.TestCase().assertEqual(result,
                                        {'aicpu': ['dsid1', 'dsid 2', 'dsid 3', 'dsid 4', 'dsid 5',
                                                   'dsid 6', 'dsid 7'],
                                         'ctrlcpu': ['dsid0']})
        with mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = None
            cal_core2cpu('test', 0)

            InfoConfReader()._info_json = {
                'DeviceInfo': [
                    {
                        'ctrl_cpu': '0',
                        'ai_cpu': '1,2,d,3',
                    }
                ]
            }
            InfoConfReader()._info_json['DeviceInfo'][0]['ai_cpu'] = '1,2,d,3'
            result = cal_core2cpu('test', 0)
            unittest.TestCase().assertEqual(result, {'aicpu': [],
                                                     'ctrlcpu': ['dsid0']}
                                            )

