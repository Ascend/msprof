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

from common_func.ms_constant.number_constant import NumberConstant
from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.host_task_collector import HostTaskCollector

file_list = {}
NAMESPACE = 'mscalculate.ascend_task.host_task_collector'


class TestHostTaskCollector(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_HostTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    collector = HostTaskCollector(PROF_DIR)

    def test__generate_host_task_objs_should_return_2_traditional_task_when_no_context_id_tasks(self):
        data = [
            [1, 2, 3, 4, f'{NumberConstant.DEFAULT_GE_CONTEXT_ID}', 0, "", 0, 1000, 0],
            [1, 2, 3, 4, f'{NumberConstant.DEFAULT_GE_CONTEXT_ID}', 0, "", 0, 1000, 1]
        ]
        self.assertEqual(len(HostTaskCollector._generate_host_task_objs(data)), 2)

    def test__generate_host_task_objs_should_return_1_traditional_and_2_context_when_context_id_tasks(self):
        data = [
            [1, 2, 3, 4, f'{NumberConstant.DEFAULT_GE_CONTEXT_ID}', 0, "", 0, 1000, 0],
            [1, 2, 3, 4, "2,3", 0, "", 0, 1000, 1]
        ]
        self.assertEqual(len(HostTaskCollector._generate_host_task_objs(data)), 3)

    def test__get_host_tasks_should_return_empty_when_no_table(self):
        self.assertEqual([], self.collector._get_host_tasks(True, 1, 1, 0))

    def test__get_host_tasks_should_return_not_empty(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".RuntimeHostTaskModel.get_host_tasks",
                           return_value=[[1, 2, 3, 4, '1', 5, "", 0, 1000, 0]]):
            self.assertEqual(len(self.collector._get_host_tasks(True, 1, 1, 0)), 1)

    def test__check_host_tasks_exists(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertTrue(self.collector._check_host_tasks_exists())
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertFalse(self.collector._check_host_tasks_exists())

    def test_get_host_tasks_by_model_and_iter(self):
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=False):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 1, 0)
            self.assertEqual(len(visible_host_tasks), 0)

        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=[]):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 1, 0)
            self.assertEqual(len(visible_host_tasks), 0)

        all_host_tasks = [
            HostTask(1, 2, 3, 4, 1, 0, "", '', 0, 1000, 0),
            HostTask(1, 2, 4, 7, 2, 1, "", '', 0, 1000, 1),
        ]
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=all_host_tasks):
            visible_host_tasks = self.collector.get_host_tasks_by_model_and_iter(1, 2, 0)
            self.assertEqual(len(visible_host_tasks), 2)

    def test_get_host_tasks(self):
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=False):
            visible_host_tasks = self.collector.get_host_tasks(0)
            self.assertEqual(len(visible_host_tasks), 0)

        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=[]):
            visible_host_tasks = self.collector.get_host_tasks(0)
            self.assertEqual(len(visible_host_tasks), 0)

        all_host_tasks = [
            HostTask(1, 2, 3, 4, 1, 0, "", '', 0, 1000, 0),
            HostTask(1, 2, 4, 7, 2, 1, "", '', 0, 1000, 1),
        ]
        with mock.patch(NAMESPACE + ".HostTaskCollector._check_host_tasks_exists", return_value=True), \
                mock.patch(NAMESPACE + ".HostTaskCollector._get_host_tasks", return_value=all_host_tasks):
            visible_host_tasks = self.collector.get_host_tasks(0)
            self.assertEqual(len(visible_host_tasks), 2)
