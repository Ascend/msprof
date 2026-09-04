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
from msmodel.hardware.hbm_model import HbmModel
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.hardware.hbm_model'


class TestHbmModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.HbmModel.drop_tab'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.create_table()

    def test_drop_tab(self):
        InfoConfReader()._info_json = {'devices': '0'}
        db_manager = DBManager()
        res = db_manager.create_table('hbm.db')
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.drop_tab()
        db_manager.destroy(res)
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.conn, check.cur = res[0], res[1]
            check.drop_tab()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HbmModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.flush([])

    def test_insert_bw_data(self):
        original_sql = "CREATE TABLE IF NOT EXISTS HBMOriginalData(device_id INT,replayid INT," \
                       "timestamp REAL,counts INT,event_type TEXT,hbmid INT)"
        insert_sql = "insert into {} values (?,?,?,?,?,?)".format('HBMOriginalData')
        data = ((5, 0, 2, 3, 4, 5), (5, 0, 0, 0, 0, 0),
                (5, 0, 1, 1, 1, 1), (5, 0, 3, 3, 3, 3), (5, 0, 1, 2, 3, 3))
        db_manager = DBManager()
        res = db_manager.create_table('hbm.db', original_sql, insert_sql, data)
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.insert_bw_data(["read", "write"])
        res[1].execute('CREATE TABLE IF NOT EXISTS HBMbwData(device_id INT,timestamp REAL,'
                       'bandwidth REAL,hbmid INT,event_type TEXT)')
        check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
        check.conn, check.cur = res[0], res[1]
        check.insert_bw_data(["read"])
        with mock.patch(NAMESPACE + '.logging.error'):
            check = HbmModel('test', 'hbm.db', ['HBMbwData', 'HBMOriginalData'])
            check.conn, check.cur = res[0], res[1]
            try:
                check.insert_bw_data(["read", "write", "write"])
            except ProfException as err:
                self.assertEqual(err.code, ProfException.PROF_SYSTEM_EXIT)
        res[1].execute("drop table HBMOriginalData")
        res[1].execute("drop table HBMbwData")
        db_manager.destroy(res)

    def _build_rows(self, timestamps, event_types=('read',)):
        """按 (device_id, timestamp, counts, hbmId, event_type) 构造原始数据。"""
        rows = []
        for ts, n in timestamps:
            for i in range(n):
                for event_type in event_types:
                    rows.append((5, ts, i, i, event_type))
        return rows

    def test_filter_abnormal_timestamps_no_loss(self):
        """无丢数据：每个时间戳条数一致，全部保留。"""
        bw_data = self._build_rows([(1000, 8), (1010, 8), (1020, 8)])
        filtered, check_len = HbmModel._filter_abnormal_timestamps(bw_data, 5)
        self.assertEqual(check_len, 8)
        self.assertEqual(len(filtered), len(bw_data))
        self.assertEqual(filtered, bw_data)

    def test_filter_abnormal_timestamps_remove_anomaly(self):
        """部分丢数据：条数与众数不一致的时间戳被去除。"""
        bw_data = self._build_rows([(1000, 8), (1010, 8), (1020, 6)])
        filtered, check_len = HbmModel._filter_abnormal_timestamps(bw_data, 5)
        self.assertEqual(check_len, 8)
        self.assertEqual(len(filtered), 16)
        self.assertTrue(all(row[1] != 1020 for row in filtered))

    def test_filter_abnormal_timestamps_mode(self):
        """众数选取：多数时间戳的条数作为 check_len。"""
        bw_data = self._build_rows([(1000, 8), (1010, 8), (1020, 8), (1030, 6), (1040, 4)])
        filtered, check_len = HbmModel._filter_abnormal_timestamps(bw_data, 5)
        self.assertEqual(check_len, 8)
        self.assertEqual(len(filtered), 24)

    def test_filter_abnormal_timestamps_log_warning(self):
        """丢数据时打印告警日志。"""
        bw_data = self._build_rows([(1000, 8), (1010, 6)])
        with mock.patch(NAMESPACE + '.logging.warning') as warn:
            HbmModel._filter_abnormal_timestamps(bw_data, 5)
            warn.assert_called_once()

    def test_filter_abnormal_timestamps_dual_event(self):
        """混合 read+write：每个 timestamp 含 8 读+8 写=16 条，check_len=16，异常时间戳被去除。"""
        bw_data = self._build_rows([(1000, 8), (1010, 8), (1020, 6)], event_types=('read', 'write'))
        filtered, check_len = HbmModel._filter_abnormal_timestamps(bw_data, 5)
        self.assertEqual(check_len, 16)
        self.assertEqual(len(filtered), 32)
        self.assertTrue(all(row[1] != 1020 for row in filtered))

    def test_filter_abnormal_timestamps_tie_prefers_larger(self):
        """众数平局（条数 2 与 4 各出现一次）时取较大条数，保留更多有效数据。"""
        bw_data = self._build_rows([(1000, 2), (1010, 4)])
        filtered, check_len = HbmModel._filter_abnormal_timestamps(bw_data, 5)
        self.assertEqual(check_len, 4)
        self.assertEqual(len(filtered), 4)
        self.assertTrue(all(row[1] == 1010 for row in filtered))
