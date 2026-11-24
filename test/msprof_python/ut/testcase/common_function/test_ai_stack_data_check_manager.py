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
import os
import sqlite3
import tempfile
import unittest
from unittest import mock

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.system_data_check_manager import SystemDataCheckManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager


class TestAiStackDataCheckManager(unittest.TestCase):

    def test_contain_block_log_data_does_not_create_missing_pmu_database(self):
        for output_exists in (False, True):
            with self.subTest(output_exists=output_exists), tempfile.TemporaryDirectory() as result_dir:
                pmu_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_METRICS_SUMMARY)
                if output_exists:
                    os.makedirs(os.path.dirname(pmu_path))
                with mock.patch.object(ChipManager(), 'is_chip_v6', return_value=True):
                    self.assertFalse(AiStackDataCheckManager.contain_block_log_data(result_dir, 0))
                self.assertFalse(os.path.exists(pmu_path))

    def test_contain_block_log_data_does_not_modify_database_when_pmu_table_is_missing(self):
        with tempfile.TemporaryDirectory() as result_dir:
            pmu_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_METRICS_SUMMARY)
            os.makedirs(os.path.dirname(pmu_path))
            with sqlite3.connect(pmu_path) as conn:
                conn.execute('CREATE TABLE existing_data (value INT)')
                conn.execute('INSERT INTO existing_data VALUES (7)')
            with open(pmu_path, 'rb') as handle:
                original = handle.read()
            with mock.patch.object(ChipManager(), 'is_chip_v6', return_value=True):
                self.assertFalse(AiStackDataCheckManager.contain_block_log_data(result_dir, 0))
            with open(pmu_path, 'rb') as handle:
                self.assertEqual(handle.read(), original)

    def test_contain_block_log_data_preserves_pmu_table_presence_check(self):
        with tempfile.TemporaryDirectory() as result_dir:
            pmu_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_METRICS_SUMMARY)
            os.makedirs(os.path.dirname(pmu_path))
            with sqlite3.connect(pmu_path) as conn:
                conn.execute(f'CREATE TABLE {DBNameConstant.TABLE_V6_BLOCK_PMU} (stream_id INT)')
            with mock.patch.object(ChipManager(), 'is_chip_v6', return_value=True):
                self.assertTrue(AiStackDataCheckManager.contain_block_log_data(result_dir, 0))
            with sqlite3.connect(pmu_path) as conn:
                self.assertEqual(conn.execute('SELECT name FROM sqlite_master WHERE type="table"').fetchall(),
                                 [(DBNameConstant.TABLE_V6_BLOCK_PMU,)])

    def test_contain_block_log_data_falls_back_to_soc_without_creating_pmu_database(self):
        with tempfile.TemporaryDirectory() as result_dir:
            soc_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_SOC_LOG)
            os.makedirs(os.path.dirname(soc_path))
            with sqlite3.connect(soc_path) as conn:
                conn.execute(f'CREATE TABLE {DBNameConstant.TABLE_BLOCK_LOG} (stream_id INT)')
            with mock.patch.object(ChipManager(), 'is_chip_v6', return_value=True):
                self.assertTrue(AiStackDataCheckManager.contain_block_log_data(result_dir, 0))
            self.assertFalse(os.path.exists(PathManager.get_db_path(result_dir, DBNameConstant.DB_METRICS_SUMMARY)))

    def test_contain_block_log_data_does_not_access_database_on_other_chips(self):
        with mock.patch.object(ChipManager(), 'is_chip_v6', return_value=False), \
                mock.patch('common_func.ai_stack_data_check_manager.DBManager.check_tables_in_db') as check_tables:
            self.assertFalse(AiStackDataCheckManager.contain_block_log_data('/unused', 0))
            check_tables.assert_not_called()

    def test_is_device_exist_should_return_true_when_contains_dir_ends_with_device(self):
        with mock.patch('os.listdir', return_value=['device_1']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertTrue(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_host(self):
        with mock.patch('os.listdir', return_value=['host']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_timeline(self):
        with mock.patch('os.listdir', return_value=['timeline']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_false_when_contains_dir_ends_with_timeline_and_host(self):
        with mock.patch('os.listdir', return_value=['host', 'timeline']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertFalse(result)

    def test_is_device_exist_should_return_true_when_contains_dir_ends_with_device_and_host(self):
        with mock.patch('os.listdir', return_value=['device_1', 'host']):
            result = AiStackDataCheckManager._is_device_exist('/path/to/result_dir')
            self.assertTrue(result)

    def test_check_outputs_should_return_true_when_device_id_not_equal_host_id(self):
        result = AiStackDataCheckManager._check_output('/path/to/result_dir', device_id=2)
        self.assertTrue(result)

    def test_check_output_should_return_false_when_result_dir_ends_with_device(self):
        result = AiStackDataCheckManager._check_output('/path/to/result_dir/device_1', device_id=NumberConstant.HOST_ID)
        self.assertFalse(result)

    def test_check_output_should_return_false_when_result_dir_ends_with_host_and_device_subdir_exists(self):
        with mock.patch('os.listdir', return_value=['device_1']):
            result = AiStackDataCheckManager._check_output('/path/to/result_dir/host', device_id=NumberConstant.HOST_ID)
            self.assertFalse(result)

    def test_check_output_should_return_true_when_result_dir_ends_with_host_but_device_subdir_not_exist(self):
        with mock.patch('os.listdir', return_value=['other_subdir']):
            result = AiStackDataCheckManager._check_output('/path/to/result_dir/host', device_id=NumberConstant.HOST_ID)
            self.assertTrue(result)

    def test_export_summary_csv_with_c_should_return_false_when_so_exist(self):
        with mock.patch('common_func.data_check_manager.DataCheckManager.check_export_with_so',
                        return_value=True):
            result_op_statistic = AiStackDataCheckManager.contain_op_statistic_data('/path/to/result_dir/host',
                                                                                    device_id=0)
            self.assertFalse(result_op_statistic)
            result_hccl_statistic = AiStackDataCheckManager.contain_hccl_statistic_data('/path/to/result_dir/host',
                                                                                        device_id=0)
            self.assertFalse(result_hccl_statistic)
            result_api_statistic = AiStackDataCheckManager.contain_api_statistic_data('/path/to/result_dir/host',
                                                                                      device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_api_statistic)
            result_fusion_op = AiStackDataCheckManager.contain_fusion_op_data('/path/to/result_dir/host',
                                                                              device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_fusion_op)
            result_npu_mem = SystemDataCheckManager.contain_npu_mem_summary_data('/path/to/result_dir/host',
                                                                                 device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_npu_mem)
            result_npu_module = AiStackDataCheckManager.contain_npu_module_mem_data('/path/to/result_dir/host',
                                                                                    device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_npu_module)
            result_step_trace = AiStackDataCheckManager.contain_training_trace_data_or_step(
                '/path/to/result_dir/host', device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_step_trace)
            result_task_time = AiStackDataCheckManager.contain_task_time_task('/path/to/result_dir/host',
                                                                              device_id=NumberConstant.HOST_ID)
            self.assertFalse(result_task_time)
            with mock.patch('common_func.ai_stack_data_check_manager.AiStackDataCheckManager._check_output',
                            return_value=True), mock.patch('common_func.db_manager.DBManager.check_connect_db',
                                                           return_value=(False, None)):
                result_op_summary = AiStackDataCheckManager.contain_op_summary_data('/path/to/result_dir/host',
                                                                                    device_id=NumberConstant.HOST_ID)
                self.assertFalse(result_op_summary)

    def test_contain_op_statistic_data_should_return_false_when_database_check_fails(self):
        ChipManager().chip_id = 16
        with mock.patch('common_func.data_check_manager.DataCheckManager.check_export_with_so', return_value=True), \
             mock.patch('common_func.ai_stack_data_check_manager.AiStackDataCheckManager._check_output', return_value=True), \
             mock.patch('common_func.db_manager.DBManager.check_connect_db', return_value=(False, None)):
                result_op_statistic = AiStackDataCheckManager.contain_op_statistic_data('/path/to/result_dir/host',
                                                                                        device_id=NumberConstant.HOST_ID)
                self.assertFalse(result_op_statistic)
