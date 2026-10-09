#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.constant import CONFIG
from mscalculate.aic.mini_aic_calculator import MiniAicCalculator
from profiling_bean.prof_enum.data_tag import DataTag
from sqlite.db_manager import DBManager
from sqlite.db_manager import DBOpen

NAMESPACE = 'mscalculate.aic.mini_aic_calculator'


class TestMiniMiniAicCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_parse_ai_core_pmu_event(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.AicPmuUtils.get_pmu_event_name'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check._parse_ai_core_pmu_event()

    def test_ms_run(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_mode': 'sample-based'}):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.ms_run()
        ProfilingScene()._scene = "step_info"
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_mode': 'task-based'}),\
                mock.patch(NAMESPACE + '.MiniAicCalculator.save', return_value='test'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.ms_run()
            ProfilingScene().init('')
            ProfilingScene()._scene = "single_op"
            check.ms_run()

    def test_ms_run_when_table_exist_then_do_not_execute(self):
        with mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config",
                        return_value={'ai_core_profiling_mode': 'task-based'}), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.calculate = mock.Mock()
            check.save = mock.Mock()
            check.ms_run()
            check.calculate.assert_not_called()
            check.save.assert_not_called()

    def test_ms_run_should_return_ok_when_EventCount_exist(self):
        metrics = [
            'total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio',
            'mte1_ratio', 'mte2_ratio', 'mte3_ratio', 'icache_miss_rate'
        ]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        ProfilingScene()._scene = "step_info"
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount " \
                     "(r8, ra, r9, rb, rc, rd, r54, r55, task_cyc, task_id, stream_id, block_num, core_num, device_id)"
        data = ((26050, 0, 526, 0, 14002, 26675, 224, 12, 58006, 3, 5, 2, 2, 0),)
        insert_sql = "insert into {0} values ({value})".format(
            "EventCount", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with DBOpen(DBNameConstant.DB_METRICS_SUMMARY) as db_open:
            with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                    mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                    mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                    mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                    mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                               return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                    mock.patch(NAMESPACE + ".get_metrics_from_sample_config", return_value=metrics):
                check = MiniAicCalculator(self.file_list, CONFIG)
                check.ms_run()
        (test_sql[1]).execute("drop Table EventCount")
        test_sql[0].commit()
        ProfilingScene().init("")
        db_manager.destroy(test_sql)

    def test_insert_metric_summary_table_2(self):
        freq = 680000000.0
        metrics = [
            'total_time(ms)', 'total_cycles', 'vec_ratio', 'mac_ratio', 'scalar_ratio',
            'mte1_ratio', 'mte2_ratio', 'mte3_ratio', 'icache_miss_rate'
        ]
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        ProfilingScene()._scene = "step_info"
        create_sql = "CREATE TABLE IF NOT EXISTS EventCount " \
                     "(r8, ra, r9, rb, rc, rd, r54, r55, task_cyc, task_id, stream_id, block_num, core_num, device_id)"
        data = ((26050, 0, 526, 0, 14002, 26675, 224, 12, 23456, 3, 5, 2, 2, 10),)
        insert_sql = "insert into {0} values ({value})".format(
            "EventCount", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_RUNTIME, create_sql, insert_sql, data)
        with DBOpen(DBNameConstant.DB_METRICS_SUMMARY) as db_open:
            with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                    mock.patch(NAMESPACE + ".DBManager.judge_table_exist", return_value=True), \
                    mock.patch(NAMESPACE + ".DBManager.check_connect_db_path", return_value=test_sql), \
                    mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"), \
                    mock.patch("common_func.config_mgr.ConfigMgr.read_sample_config", return_value={}), \
                    mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                               return_value=(db_open.db_conn, db_open.db_curs)), \
                    mock.patch(NAMESPACE + '.get_limit_and_offset',
                               return_value=(10, 0)), \
                    mock.patch(NAMESPACE + ".get_metrics_from_sample_config", return_value=metrics):
                check = MiniAicCalculator(self.file_list, CONFIG)
                check.ms_run()
        (test_sql[1]).execute("drop Table EventCount")
        test_sql[0].commit()
        ProfilingScene().init("")
        db_manager.destroy(test_sql)

    def test_save_should_run_success_when_normal(self):
        with mock.patch(NAMESPACE + '.MiniAicCalculator.create_metrics_summary_db'), \
                mock.patch('viewer.calculate_rts_data.create_metric_table'), \
                mock.patch('common_func.db_manager.DBManager.insert_data_into_table'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.save()

    def test_save_should_catch_exception_when_raise(self):
        with mock.patch(NAMESPACE + '.MiniAicCalculator.create_metrics_summary_db', side_effect=ValueError), \
                mock.patch('viewer.calculate_rts_data.create_metric_table'), \
                mock.patch('common_func.db_manager.DBManager.insert_data_into_table'):
            check = MiniAicCalculator(self.file_list, CONFIG)
            check.save()

    def test_get_metric_summary_data_should_return_empty_when_no_table(self):
        check = MiniAicCalculator(self.file_list, CONFIG)
        self.assertEqual(check.get_metric_summary_data(1800), [])


    def test_get_metric_summary_data_should_return_data_when_is_ALLEXPORT(self):
        with mock.patch('common_func.db_manager.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[(1,2,3)]):
            check = MiniAicCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'devices': '0'}
            self.assertEqual(check.get_metric_summary_data(1800), [(1,2,3)])
            InfoConfReader()._info_json.clear()

    def test_get_metric_summary_data_should_return_data_when_is_not_ALLEXPORT(self):
        with mock.patch('common_func.db_manager.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True):

            check = MiniAicCalculator(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'devices': '0'}
            ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
            self.assertEqual(check.get_metric_summary_data(1800), [])
            with mock.patch('viewer.calculate_rts_data._query_limit_and_offset', return_value=[1, 2]), \
                    mock.patch('common_func.db_manager.DBManager.get_table_headers', return_value=["ai_core_num"]), \
                    mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[(1, 2, 3)]):
                self.assertEqual(check.get_metric_summary_data(1800), [(1, 2, 3)])
            InfoConfReader()._info_json.clear()
            ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

