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
import os.path
import sqlite3
import unittest
from unittest import mock

from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBOpen
from viewer.cpu_data_report import get_aictrl_pmu_events, get_ts_pmu_events, get_cpu_hot_function

NAMESPACE = 'viewer.cpu_data_report'
headers = ['Event', 'Name', 'Count']
sample_config = {"ts_cpu_profiling_events": "0x0,0x1,0x2"}


class TestCPUDataReport(unittest.TestCase):
    def test_get_aictrl_pmu_events_1(self):
        db_name = "test_get_aictrl_pmu_events_1_aicpu_0.db"
        with DBOpen(db_name) as db_open:
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            # table not exist
            res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, res)
            # sqlite error
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
                res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
                self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, res)

    def test_get_aictrl_pmu_events_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (common, pid, tid, core, timestamp," \
                     " pmucount, pmuevent, ip, function, offset, module, replayid)"
        data = (("swapper", 0, 0, 7, 1467.43561, 8017770, "r11", "ffff000008087e80", "unknown",
                 "unknown", "[unknown]", 0),)
        db_name = "test_get_aictrl_pmu_events_2_aicpu_0.db"
        with DBOpen(db_name) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data("OriginalData", data)
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(res[2], 1)
            with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=None):
                res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
                self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_ts_pmu_events_1(self):
        db_name = "test_get_ts_pmu_events_1_tscpu_0.db"
        # no db, connect failed
        res = get_ts_pmu_events('', db_name, 'OriginalData', headers)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

        with DBOpen(db_name) as db_open, \
                mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=sample_config):
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            # table not exist
            res = get_ts_pmu_events(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)
            # sqlite error
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
                res = get_ts_pmu_events(project_path, db_name, 'OriginalData', headers)
                self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_ts_pmu_events_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (count, event)"
        data = ((1, "0x0"),)
        db_name = "test_get_ts_pmu_events_2_tscpu_0.db"
        with DBOpen(db_name) as db_open, \
                mock.patch("common_func.config_mgr.ConfigMgr.pre_check_sample", return_value=sample_config):
            db_open.create_table(create_sql)
            db_open.insert_data("OriginalData", data)
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            # valid read_cpu_cfg
            res = get_ts_pmu_events(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(res[2], 1)
            # invalid read_cpu_cfg
            with mock.patch(NAMESPACE + '.read_cpu_cfg', return_value=None):
                res = get_ts_pmu_events(project_path, db_name, 'OriginalData', headers)
                self.assertEqual(res[2], 1)

    def test_get_cpu_hot_function_1(self):
        db_name = "test_get_cpu_hot_function_1_aicpu_0.db"
        with DBOpen(db_name) as db_open:
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            # table not exist
            res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, res)
            # sqlite error
            with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', side_effect=sqlite3.Error):
                res = get_aictrl_pmu_events(project_path, db_name, 'OriginalData', headers)
                self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, res)

    def test_get_cpu_hot_function_2(self):
        db_name = "test_get_cpu_hot_function_2_aicpu_0.db"
        create_sql = "CREATE TABLE IF NOT EXISTS OriginalData (common, pid, tid, core, timestamp," \
                     " pmucount, cycles, ip, func, offset, module, r11)"
        data = (("swapper", 0, 0, 7, 1467.43561, 8017770, "r11", "ffff000008087e80", "unknown",
                 "unknown", "[unknown]", 3),)
        with DBOpen(db_name) as db_open:
            db_open.create_table(create_sql)
            project_path = os.path.dirname(os.path.dirname(db_open.db_path))
            # no  data
            res = get_cpu_hot_function(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(res[2], 0)

            # 原来就是插了2条一样的，意义不明
            db_open.insert_data("OriginalData", data)
            db_open.insert_data("OriginalData", data)
            res = get_cpu_hot_function(project_path, db_name, 'OriginalData', headers)
            self.assertEqual(res[2], 1)


if __name__ == '__main__':
    unittest.main()
