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
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.hardware_info_report import get_ddr_data, get_llc_bandwidth, llc_capacity_data

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

    def test_get_llc_bandwidth_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs):
            res = get_llc_bandwidth('', 0)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '._get_bandwidth_res', side_effect=OSError), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_llc_bandwidth('', 0)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_llc_bandwidth('', 0)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_llc_bandwidth_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS LLCMetricData" \
                     " (device_id, replayid, timestamp, read_allocate, read_noallocate, read_hit, " \
                     "write_allocate, write_noallocate, write_hit)"
        data = ((0, 0, 1466.706340169, 166393, 21, 151150, 81601, 0, 1),
                (0, 0, 1566.706340169, 166393, 21, 151150, 81601, 0, 1))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("LLCMetricData", data)

        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            configs[StrConstant.LLC_PROF] = StrConstant.LLC_BAND_ITEM
            InfoConfReader()._info_json = {"pid": 0}
            res = get_llc_bandwidth('', 0)
        test_sql = db_manager.connect_db("llc.db")
        (test_sql[1]).execute("drop Table LLCMetricData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 3)

    def test_llc_capacity_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs):
            res = llc_capacity_data('', 0, "ctrlcpu")
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '._get_llc_capacity_data', side_effect=TypeError):
            res = llc_capacity_data('', 0, "ctrlcpu")
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(True, True)), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = llc_capacity_data('', 0, "ctrlcpu")
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_llc_capacity_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS LLCDsidData" \
                     " (device_id, replayid, timestamp, dsid0, dsid1, dsid2, dsid3, dsid4, dsid5, dsid6, dsid7)"
        data = ((0, 0, 631006.658544967, 41, 14, 0, 0, 0, 0, 0, 1),
                (0, 0, 631006.678778508, 67, 20, 0, 0, 0, 0, 0, 5))
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql("LLCDsidData", data)

        test_sql = db_manager.create_table("llc.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            configs[StrConstant.LLC_PROF] = StrConstant.LLC_CAPACITY_ITEM
            InfoConfReader()._info_json = {"pid": 0}
            res = llc_capacity_data('', 0, "ctrlcpu")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=configs), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            configs[StrConstant.LLC_PROF] = StrConstant.LLC_CAPACITY_ITEM
            InfoConfReader()._info_json = {"pid": 0}
            res_1 = llc_capacity_data('', 0, "")
        test_sql = db_manager.connect_db("llc.db")
        (test_sql[1]).execute("drop Table LLCDsidData")
        db_manager.destroy(test_sql)
        self.assertEqual(res[2], 1)
        self.assertEqual(res_1, MsvpConstant.MSVP_EMPTY_DATA)



if __name__ == '__main__':
    unittest.main()
