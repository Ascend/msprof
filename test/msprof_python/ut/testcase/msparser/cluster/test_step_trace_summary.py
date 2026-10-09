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

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.cluster.step_trace_summary import StepTraceSummary

NAMESPACE = 'msparser.cluster.step_trace_summary'


class TestStepTraceSummary(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_StepTraceSummary')
    param = {
        "collection_path": DIR_PATH,
        "is_cluster": True,
        "npu_id": 0,
        "model_id": 1,
        "iteration_id": 1
    }

    def setUp(self) -> None:
        os.mkdir(self.DIR_PATH)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_should_raise_exception_when_npu_id_and_iter_id_is_no_cluster_scene(self):
        case_param = {}
        case_param.update(self.param)
        with pytest.raises(ProfException) as err:
            check = StepTraceSummary(case_param)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_should_raise_exception_when_npu_id_and_iter_id_is_export_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1, "iteration_id": -1})
        with pytest.raises(ProfException) as err:
            check = StepTraceSummary(case_param)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        check = StepTraceSummary(case_param)
        check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_rank_db_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True):
            check = StepTraceSummary(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch("os.path.getsize", return_value=0), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=False):
            check = StepTraceSummary(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_get_device_and_rank_ids(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch("os.path.getsize", return_value=0), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_device_and_rank_ids',
                           return_value=[(1, 11), (2, 12), (3, 13)]):
            check = StepTraceSummary(case_param)
            check.process()

    def test_should_cover_cluster_scene_when_npu_id_is_export_all_cluster_get_no_device_and_rank_ids(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch("os.path.getsize", return_value=0), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_device_and_rank_ids', return_value=[]):
            check = StepTraceSummary(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all_cluster_db_table_not_exist(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummary._get_rank_or_device_ids', return_value=set([1])):
            check = StepTraceSummary(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all_query_no_db_data(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummary._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummary(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_npu_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"npu_id": -1})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummary._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.ClusterStepTraceViewModel.get_sql_data', return_value=[[1]]), \
                mock.patch(NAMESPACE + '.StepTraceSummary._storage_summary_data'):
            check = StepTraceSummary(case_param)
            check.process()

    def test_query_in_cluster_scene_should_cover_cluster_scene_when_iteration_id_is_all(self):
        case_param = {}
        case_param.update(self.param)
        case_param.update({"iteration_id": None})
        with mock.patch(NAMESPACE + '.StepTraceSummary._check_iteration_id_valid'), \
                mock.patch(NAMESPACE + '.StepTraceSummary._check_step_trace_db', return_value=True), \
                mock.patch(NAMESPACE + '.StepTraceSummary._get_rank_or_device_ids', return_value=set([1])), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[]):
            check = StepTraceSummary(case_param)
            check.process()
