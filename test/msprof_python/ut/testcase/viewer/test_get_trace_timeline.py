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
import json
import sqlite3
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from sqlite.db_manager import DBManager
from viewer.get_trace_timeline import TraceViewer
from viewer.get_trace_timeline import get_aicore_utilization_timeline
from viewer.get_trace_timeline import get_dvpp_timeline
from viewer.get_trace_timeline import get_hccs_timeline
from viewer.get_trace_timeline import get_network_timeline
from viewer.get_trace_timeline import get_pcie_timeline

NAMESPACE = 'viewer.get_trace_timeline'
param = {"project_path": "", "device_id": 0, "start_time": 0,
         "end_time": NumberConstant.DEFAULT_END_TIME, "dvppid": "all"}


class TestTraceViewer(unittest.TestCase):

    def test_multiple_name_dump(self):
        res = TraceViewer("").multiple_name_dump([], '', '')
        self.assertEqual(res, [])

    def test_get_hccs_timeline_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_hccs_timeline('', 0, 0, 0)
        self.assertEqual(len(res), 0)

    def test_get_hccs_timeline_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_HCCS_EVENTS + \
                     " (device_id, timestamp, txthroughput, rxthroughput)"
        data = ((0, 176168489516040, 201, 201),)
        insert_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_HCCS_EVENTS, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("hccs.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_hccs_timeline('', 0, 0, 0)

        test_sql = db_manager.connect_db("hccs.db")
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_HCCS_EVENTS))
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 3)

    def test_get_hccs_timeline_3(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_HCCS_EVENTS \
                     + " (device_id, timestamp, txthroughput, rxthroughput)"
        data = ((0, 176168489516040, 201, 201),)
        insert_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_HCCS_EVENTS, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("hccs.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {"pid": 0}
            res_1 = get_hccs_timeline('', 0, 0, NumberConstant.DEFAULT_END_TIME)
        test_sql = db_manager.connect_db("hccs.db")
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_HCCS_EVENTS))
        db_manager.destroy(test_sql)
        self.assertEqual(len(res_1), 3)

    def test_get_pcie_timeline_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)):
            res = get_pcie_timeline(param)
        self.assertEqual(len(res), 0)

    def test_get_pcie_timeline_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS PcieOriginalData" \
                     " (timestamp, device_id, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, " \
                     "tx_np_bandwidth_min, tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, " \
                     "tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg, tx_np_latency_min, tx_np_latency_max, " \
                     "tx_np_latency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max, rx_p_bandwidth_avg, " \
                     "rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min, " \
                     "rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg)"
        data = ((176168204982690, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0),
                (176168404973340, 0, 0, 1073, 20, 0, 41, 0, 0, 0, 0, 271, 597, 108, 0, 28, 0, 0, 0, 0, 0, 164, 0))
        insert_sql = "insert into {0} values ({value})".format(
            "PcieOriginalData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("pcie.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_pcie_timeline(param)
        test_sql = db_manager.connect_db("pcie.db")
        (test_sql[1]).execute("drop Table PcieOriginalData")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 5)

    def test_get_dvpp_timeline_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_dvpp_timeline(param)
        self.assertEqual(len(res), 0)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
            res = get_dvpp_timeline(param)
        self.assertEqual(len(res), 0)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_dvpp_timeline(param)
        self.assertEqual(len(res), 0)

    def test_get_dvpp_timeline_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DvppOriginalData" \
                     " (device_id, replayid, timestamp, dvppid, enginetype, engineid, alltime, allframe, " \
                     "allutilization, proctime, procframe, procutilization, lasttime, lastframe)"
        data = ((0, 0, 176167.88990126, 8, 0, 3, 0, 0, "0%", 0, 0, "0%", 0, 0),)
        insert_sql = "insert into {0} values ({value})".format(
            "DvppOriginalData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("peripheral.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_dvpp_timeline(param)
        test_sql = db_manager.connect_db("peripheral.db")
        (test_sql[1]).execute("drop Table DvppOriginalData")
        db_manager.destroy(test_sql)
        self.assertEqual(len(res), 12)

    def test_get_network_timeline_1(self):
        res = get_network_timeline("", 0, 0, NumberConstant.DEFAULT_END_TIME, "")
        self.assertEqual(len(res), 0)

        res = get_network_timeline("", 0, 0, NumberConstant.DEFAULT_END_TIME, "roce")
        self.assertEqual(len(res), 0)

    def test_get_network_timeline_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_NIC_RECEIVE + \
                     " (device_id, timestamp, rx_bandwidth_efficiency, rx_packets, rx_error_rate," \
                     " rx_dropped_rate, tx_bandwidth_efficiency, tx_packets, tx_error_rate, tx_dropped_rate, func_id)"
        data = ((0, 176168008028161, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        insert_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_NIC_RECEIVE, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_NIC_RECEIVE, create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_network_timeline("", 0, 0, NumberConstant.DEFAULT_END_TIME, "nic")
        self.assertEqual(len(res), 3)
        test_sql = db_manager.connect_db(DBNameConstant.DB_NIC_RECEIVE)
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_NIC_RECEIVE))
        db_manager.destroy(test_sql)

    def test_get_aicore_utilization_timeline_1(self):
        with mock.patch(NAMESPACE + '.get_aicore_utilization', side_effect=sqlite3.Error):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_aicore_utilization_timeline("")
        self.assertEqual(len(res), 0)

        with mock.patch(NAMESPACE + '.JsonManager.loads', return_value=None):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_aicore_utilization_timeline("")
        self.assertEqual(len(res), 0)

        aicore_data = {"status": 0,
                       "data": {"maxTime": "0.00", "minTime": 0,
                                "usage": {"0": [["176256.335450", "0.000000"]],
                                          "average": [["176256.335450", "0.000000"]]}}}
        with mock.patch(NAMESPACE + '.get_aicore_utilization', return_value=json.dumps(aicore_data)):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_aicore_utilization_timeline("")
        self.assertEqual(len(res), 3)


if __name__ == '__main__':
    unittest.main()
