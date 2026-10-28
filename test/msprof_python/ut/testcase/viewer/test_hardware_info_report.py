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

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.hardware_info_report import get_ddr_data

NAMESPACE = 'viewer.hardware_info_report'
configs = {'headers': ['Name', 'Type', 'Start Time', 'Duration(us)', 'Process ID', 'Thread ID'],
           'db': 'llc.db'}


class TestHardwareInfo(unittest.TestCase):
    def test_get_ddr_data_1(self):
        with mock.patch(NAMESPACE + '._get_ddr_data_from_db', side_effect=sqlite3.Error):
            res = get_ddr_data('', 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            res = get_ddr_data('', 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_ddr_data('', 0, configs)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_ddr_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DDRMetricData" \
                     " (device_id, replayid, timestamp, flux_read, flux_write, fluxid_read, fluxid_write)"
        data = ((0, 0, 1466685635065, 1184.08203125, 439.453125, None, None),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("DDRMetricData", data)

        test_sql = db_manager.create_table("ddr.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {"pid": 0}
            res = get_ddr_data('', 0, configs)
        test_sql = db_manager.connect_db("ddr.db")
        (test_sql[1]).execute("drop Table DDRMetricData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
