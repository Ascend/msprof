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
import json
import sqlite3
import unittest
from unittest import mock
import decimal
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen
from constant.info_json_construct import InfoJson
from viewer.get_msvp_llc_timeline_training import get_llc_nomini_data, get_llc_timeline, pre_check_llc, \
    get_llc_mini_data, get_llc_bandwidth, get_llc_capacity, get_llc_db_table, get_ddr_timeline, get_hbm_timeline

NAMESPACE = 'viewer.get_msvp_llc_timeline_training'
param = {'project': "", "project_path": '', "device_id": 0,
         "start_time": 0, "end_time": 9223372036854775807,
         "data_type": "llc_aicpu"}
sample_config = {StrConstant.LLC_PROF: "bandwidth"}


class TestLLCTimelineTrain(unittest.TestCase):
    def test_get_llc_nomini_data_1(self):
        with DBOpen(DBNameConstant.DB_LLC) as db_open:
            with mock.patch(NAMESPACE + '.get_llc_metric_data', side_effect=sqlite3.Error):
                res = get_llc_nomini_data(param, sample_config, db_open.db_curs)
            self.assertEqual(len(res), 0)

            with mock.patch(NAMESPACE + '.get_llc_metric_data', return_value=[]):
                InfoConfReader()._info_json = {'pid': "0"}
                res = get_llc_nomini_data(param, sample_config, db_open.db_curs)
            self.assertEqual(len(res), 0)

    def test_get_llc_nomini_data_2(self):
        param['llc_id'] = '0'
        create_sql = "CREATE TABLE IF NOT EXISTS " + StrConstant.LLC_METRICS_TABLE + \
                     "(device_id, l3tid, timestamp, hitrate, throughput)"
        data = ((0, 0, 1466, 0.6, 9564.52737881),)
        insert_sql = "insert into {0} values ({value})".format(
            StrConstant.LLC_METRICS_TABLE, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC, create_sql, insert_sql, data)
        InfoConfReader()._info_json = {'pid': "0"}
        res = get_llc_nomini_data(param, sample_config, test_sql[1])
        self.assertEqual(len(res), 3)
        (test_sql[1]).execute("drop Table {}".format(StrConstant.LLC_METRICS_TABLE))
        db_manager.destroy(test_sql)

    def test_pre_check_llc_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC)
        res_1 = pre_check_llc(None, None, sample_config, "")
        self.assertEqual(len(res_1), 0)

        res_2 = pre_check_llc(test_sql[0], test_sql[1], None, "")
        self.assertEqual(len(res_2), 0)
        db_manager.destroy(test_sql)

    def test_pre_check_llc_2(self):
        sample_config[StrConstant.LLC_PROF] = "aicpu"
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC)
        res_1 = pre_check_llc([0], test_sql[1], sample_config, "")
        self.assertEqual(len(res_1), 0)

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res_2 = pre_check_llc(test_sql[0], test_sql[1], sample_config, "")
        self.assertEqual(len(res_2), 0)
        sample_config[StrConstant.LLC_PROF] = "bandwidth"

        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res_3 = pre_check_llc(test_sql[0], test_sql[1], sample_config, "")
        self.assertEqual(res_3, [])
        db_manager.destroy(test_sql)

    def test_get_llc_mini_data_1(self):
        with DBOpen(DBNameConstant.DB_LLC) as db_open:
            with mock.patch(NAMESPACE + '.get_llc_bandwidth', return_value=[]):
                res = get_llc_mini_data(param, sample_config, db_open.db_curs)
            self.assertEqual(res, [])

            sample_config[StrConstant.LLC_PROF] = "capacity"
            with mock.patch(NAMESPACE + '.get_llc_capacity', return_value=[]):
                res = get_llc_mini_data(param, sample_config, db_open.db_curs)
            self.assertEqual(res, [])

            sample_config[StrConstant.LLC_PROF] = "read"
            with mock.patch(NAMESPACE + '.get_llc_capacity', return_value=[]):
                res = get_llc_mini_data(param, sample_config, db_open.db_curs)
            self.assertEqual(len(res), 0)
            sample_config[StrConstant.LLC_PROF] = "bandwidth"

    def test_get_llc_bandwidth_1(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_LLC_METRIC_DATA + \
                     "(device_id, replayid, timestamp, read_allocate, read_noallocate," \
                     " read_hit, write_allocate, write_noallocate, write_hit)"
        data = ((0, 0, 1466.706340169, 166393, 21, 151150, 81601, 0, 1),
                (0, 0, 1466.726545742, 15753, 21, 12840, 0, 0, 0))
        with DBOpen(DBNameConstant.DB_LLC) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_LLC_METRIC_DATA, data)
            InfoConfReader()._info_json = {'pid': "0"}
            res = get_llc_bandwidth(db_open.db_curs)
            with mock.patch(NAMESPACE + '.get_bandwidth_value', return_value=[]):
                res_1 = get_llc_bandwidth(db_open.db_curs)
            self.assertEqual(len(res), 5)
            self.assertEqual(len(res_1), 0)

            with mock.patch(NAMESPACE + '.get_bandwidth_value', return_value=[]), \
                 mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
                res_1 = get_llc_bandwidth(db_open.db_curs)
            self.assertEqual(len(res), 5)
            self.assertEqual(len(res_1), 0)

            with mock.patch(NAMESPACE + '.get_bandwidth_value', return_value=[]), \
                 mock.patch(NAMESPACE + '._format_llc_trace_data', return_value=[]):
                res_1 = get_llc_bandwidth(db_open.db_curs)
            self.assertEqual(len(res), 5)
            self.assertEqual(len(res_1), 0)

    def test_get_llc_capacity_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC)
        with mock.patch(NAMESPACE + '.TraceViewManager.metadata_event', side_effect=OSError):
            InfoConfReader()._info_json = {'pid': "0"}
            res = get_llc_capacity(param, test_sql[1])
        self.assertEqual(len(res), 0)
        db_manager.destroy(test_sql)

    def test_get_llc_capacity_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS " + DBNameConstant.TABLE_LLC_DSID + \
                     "(device_id, replayid, timestamp, dsid0, dsid1, dsid2, dsid3, dsid4, dsid5, dsid6, dsid7)"
        data = ((0, 0, 631006637428508, 85, 26, 0, 0, 0, 0, 0, 1),
                (0, 0, 631006637428508, 162, 50, 0, 0, 0, 0, 0, 1))
        insert_sql = "insert into {0} values ({value})".format(
            DBNameConstant.TABLE_LLC_DSID, value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC, create_sql, insert_sql, data)
        InfoConfReader()._info_json = {'pid': "0"}
        InfoConfReader()._local_time_offset = 0
        decimal.getcontext().prec = 40
        res = get_llc_capacity(param, test_sql[1])
        self.assertEqual(len(res), 5)
        (test_sql[1]).execute("drop Table {}".format(DBNameConstant.TABLE_LLC_DSID))
        db_manager.destroy(test_sql)

        with mock.patch(NAMESPACE + '._get_dsid_sql_data', return_value=[]):
            InfoConfReader()._info_json = {'pid': "0"}
            res = get_llc_capacity(param, test_sql[1])
        self.assertEqual(len(res), 0)

    def test_get_llc_db_table(self):
        sample_config[StrConstant.LLC_PROF] = ""
        res = get_llc_db_table(sample_config)
        self.assertEqual(res, StrConstant.LLC_METRICS_TABLE)

        sample_config[StrConstant.LLC_PROF] = StrConstant.LLC_CAPACITY_ITEM
        res = get_llc_db_table(sample_config)
        self.assertEqual(res, DBNameConstant.TABLE_LLC_DSID)

        sample_config[StrConstant.LLC_PROF] = StrConstant.LLC_BAND_ITEM
        res = get_llc_db_table(sample_config)
        self.assertEqual(res, DBNameConstant.TABLE_LLC_METRIC_DATA)

    def test_get_llc_timeline_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=sample_config), \
                mock.patch(NAMESPACE + '.get_llc_db_table', return_value=''), \
                mock.patch(NAMESPACE + '.pre_check_llc', return_value=[]), \
                mock.patch(NAMESPACE + '.get_llc_mini_data', side_effect=OSError):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            res = get_llc_timeline(param)
        self.assertEqual(len(res), 0)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_llc_db_table', return_value=''), \
             mock.patch(NAMESPACE + '.pre_check_llc', return_value=[1]):
            res = get_llc_timeline(param)
        self.assertEqual(res, [1])
        db_manager.destroy(test_sql)

    def test_get_llc_timeline_2(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_LLC)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_llc_db_table', return_value=''), \
             mock.patch(NAMESPACE + '.pre_check_llc', return_value=[]), \
             mock.patch(NAMESPACE + '.get_llc_mini_data', return_value=[]):
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            res = get_llc_timeline(param)
        self.assertEqual(res, [])

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_llc_db_table', return_value=''), \
             mock.patch(NAMESPACE + '.pre_check_llc', return_value=[]), \
             mock.patch(NAMESPACE + '.get_llc_nomini_data', return_value=[]):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            res = get_llc_timeline(param)
        self.assertEqual(res, [])
        db_manager.destroy(test_sql)

    def test_get_ddr_timeline_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("ddr.db")
        db_manager.destroy(test_sql)
        test_sql = db_manager.create_table("ddr.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.get_ddr_metric_data', return_value=[1]),\
                mock.patch(NAMESPACE + '._reformat_ddr_data', return_value=[]):
            res = get_ddr_timeline(param)
        self.assertEqual(len(res), 0)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_ddr_timeline(param)
        self.assertEqual(len(res), 0)

        test_sql = db_manager.create_table("ddr.db")
        db_manager.destroy(test_sql)
        test_sql = db_manager.create_table("ddr.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_ddr_timeline(param)
        self.assertEqual(len(res), 0)

        db_manager.destroy(test_sql)
        test_sql = db_manager.create_table("ddr.db")
        db_manager.destroy(test_sql)
        test_sql = db_manager.create_table("ddr.db")
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_ddr_timeline(param)
        self.assertEqual(len(res), 0)
        db_manager.destroy(test_sql)

    def test_get_hbm_timeline_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS DDRMetricData (device_id, replayid, timestamp, flux_read, " \
                     "flux_write, fluxid_read, fluxid_write)"
        data = ((0, 0, 1467, 1184.08203125, 439.453125, None, None),)
        insert_sql = "insert into {0} values ({value})".format(
            "DDRMetricData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("ddr.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {'pid': "0"}
            res = get_ddr_timeline(param)
        self.assertEqual(len(res), 4)
        test_sql = db_manager.connect_db("ddr.db")
        (test_sql[1]).execute("drop Table DDRMetricData")
        db_manager.destroy(test_sql)

    def test_get_hbm_timeline_1(self):
        db_manager = DBManager()
        test_sql = db_manager.create_table("hbm.db")
        db_manager.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '._reformat_hbm_data', side_effect=OSError):
            res = get_hbm_timeline(param)
        self.assertEqual(len(res), 0)

        test_sql = db_manager.create_table("hbm.db")
        db_manager.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(None, None)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            res = get_hbm_timeline(param)
        self.assertEqual(len(res), 0)

        test_sql = db_manager.create_table("hbm.db")
        db_manager.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
             mock.patch(NAMESPACE + '._reformat_hbm_data', return_value=[]):
            InfoJson(pid="0").apply()
            res = get_hbm_timeline(param)
        self.assertEqual(len(res), 0)

        test_sql = db_manager.create_table("hbm.db")
        db_manager.destroy(test_sql)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_hbm_timeline(param)
        self.assertEqual(len(res), 0)

    def test_get_hbm_timeline_3(self):
        param['end_time'] = 0
        create_sql = "CREATE TABLE IF NOT EXISTS HBMbwData (device_id, timestamp, bandwidth, hbmid, event_type)"
        data = ((0, 1761, 0, 0, "read"),
                (0, 1762, 0, 0, "write"))
        insert_sql = "insert into {0} values ({value})".format(
            "HBMbwData", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table("hbm.db", create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=test_sql), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            InfoConfReader()._info_json = {'pid': "0"}
            res = get_hbm_timeline(param)
        self.assertEqual(len(res), 3)
        test_sql = db_manager.connect_db("hbm.db")
        (test_sql[1]).execute("drop Table HBMbwData")
        db_manager.destroy(test_sql)


if __name__ == '__main__':
    unittest.main()
