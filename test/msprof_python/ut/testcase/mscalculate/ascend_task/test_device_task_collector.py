#!/usr/bin/python3
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
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from mscalculate.ascend_task.ascend_task import DeviceTask
from mscalculate.ascend_task.device_task_collector import DeviceTaskCollector

file_list = {}
NAMESPACE = 'mscalculate.ascend_task.device_task_collector'


class TestDeviceTaskCollector(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_DeviceTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')

    def setUp(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5", "drvVersion": InfoConfReader().ALL_EXPORT_VERSION}

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}

    def test__gather_device_tasks_from_hwts_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_tasks_from_hwts(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.HwtsLogModel.get_hwts_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_tasks_from_hwts(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_device_tasks_from_hwts_aiv_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_tasks_from_hwts_aiv(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.HwtsAivModel.get_hwts_aiv_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_tasks_from_hwts_aiv(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_ai_cpu_device_tasks_from_ts_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_ai_cpu_device_tasks_from_ts(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.AiCpuModel.get_ai_cpu_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_ai_cpu_device_tasks_from_ts(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_device_acsq_tasks_from_stars_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_acsq_tasks_from_stars(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.AcsqTaskModel.get_acsq_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_acsq_tasks_from_stars(0, 1)
            self.assertEqual(len(tasks), 1)

    def test_device_v5_tasks_from_stars_when_data_exists_then_pass(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_chip_v5_tasks_from_stars(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.V5StarsViewModel.get_v5_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_chip_v5_tasks_from_stars(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_device_ffts_plus_sub_tasks_from_starts_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_ffts_plus_sub_tasks_from_stars(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.FftsLogModel.get_ffts_plus_sub_task_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_ffts_plus_sub_tasks_from_stars(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_device_tasks_from_runtime_when_data_exists(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector._gather_device_tasks_from_runtime(float("-inf"), float("inf"))
        self.assertEqual(tasks, [])

        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + '.RuntimeTaskTimeModel.get_runtime_task_data_within_time_range',
                           return_value=[[1, 2, 4294967295, 1000, 2000, ""]]):
            collector = DeviceTaskCollector(self.PROF_DIR)
            tasks = collector._gather_device_tasks_from_runtime(0, 1)
            self.assertEqual(len(tasks), 1)

    def test__gather_chip_v1_1_0_device_tasks_should_return_aicore_and_aicpu_task(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_runtime',
                        return_value=[]):
            tasks = collector._gather_chip_v1_1_0_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 0)

        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_runtime',
                        return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CORE")]):
            tasks = collector._gather_chip_v1_1_0_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 1)

    def test__gather_chip_v2_1_0_device_tasks_should_return_none(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_hwts',
                        return_value=[]), \
                mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_ai_cpu_device_tasks_from_ts', return_value=[]):
            tasks = collector._gather_chip_v2_1_0_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 0)

    def test__gather_chip_v2_1_0_device_tasks_should_return_only_aicore_task_when_no_aicpu_task(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_hwts',
                        return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CORE")]), \
                mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_ai_cpu_device_tasks_from_ts', return_value=[]):
            tasks = collector._gather_chip_v2_1_0_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 1)

    def test__gather_chip_v2_1_0_device_tasks_should_return_aicore_and_aicpu_task(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_hwts',
                        return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CORE")]), \
                mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_ai_cpu_device_tasks_from_ts',
                           return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CPU")]):
            tasks = collector._gather_chip_v2_1_0_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 2)

    def test__gather_chip_v3_device_tasks_should_return_0_task_when_no_hwts_task(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_hwts',
                        return_value=[]), \
                mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_tasks_from_hwts_aiv',
                           return_value=[]):
            tasks = collector._gather_chip_v3_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 0)

    def test__gather_chip_stars_device_tasks_should_return_empty_when_no_hwts_task(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_acsq_tasks_from_stars',
                        return_value=[]):
            tasks = collector._gather_chip_stars_device_tasks(float("-inf"), float("inf"))
            self.assertEqual(len(tasks), 0)

    def test_get_all_device_tasks(self):
        collector = DeviceTaskCollector(self.PROF_DIR)

        tasks = collector.get_all_device_tasks()
        self.assertEqual(len(tasks), 0)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._check_device_data_db_exists',
                        return_value=True), \
                mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_acsq_tasks_from_stars',
                           return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CORE")]):
            tasks = collector.get_all_device_tasks()
            self.assertEqual(len(tasks), 1)

    def test_get_device_tasks_by_model_and_iter(self):
        collector = DeviceTaskCollector(self.PROF_DIR)
        tasks = collector.get_device_tasks_by_model_and_iter(1, 1)
        self.assertEqual(len(tasks), 0)
        with mock.patch(NAMESPACE + '.DeviceTaskCollector._check_device_data_db_exists',
                        return_value=True):
            with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[]]):
                tasks = collector.get_device_tasks_by_model_and_iter(1, 1)
                self.assertEqual(len(tasks), 0)
            with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[[0, 1]]), \
                    mock.patch(NAMESPACE + '.DeviceTaskCollector._gather_device_acsq_tasks_from_stars',
                               return_value=[DeviceTask(1, 1, 1, 1, 1, "AI_CORE")]):
                tasks = collector.get_device_tasks_by_model_and_iter(1, 1)
                self.assertEqual(len(tasks), 1)
