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
from msparser.parallel.parallel_query.cluster_parallel_analysis_tuning import ClusterParallelAnalysisTuning

NAMESPACE = "msparser.parallel.parallel_query.cluster_parallel_analysis_tuning"


class TestClusterParallelAnalysisTuning(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelAnalysisTuning')
    PARAMS1 = {"collection_path": DIR_PATH,
               "npu_id": 1,
               "model_id": -1,
               "iteration_id": -1}
    PARAMS2 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": 1,
               "iteration_id": -1}
    PARAMS3 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": -1,
               "iteration_id": 5}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_process_should_raise_exception_when_without_db(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=False):
                check = ClusterParallelAnalysisTuning(self.PARAMS1)
                check.process()
                self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_process_should_raise_exception_when_invalid_npu_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True):
                check = ClusterParallelAnalysisTuning(self.PARAMS1)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process_should_raise_exception_when_invalid_model_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True):
                check = ClusterParallelAnalysisTuning(self.PARAMS2)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process_should_raise_exception_when_invalid_iteration_id(self) -> None:
        with pytest.raises(ProfException) as err:
            with mock.patch(NAMESPACE + ".os.path.exists", return_value=True):
                check = ClusterParallelAnalysisTuning(self.PARAMS3)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_process(self):
        with mock.patch(NAMESPACE + ".ClusterParallelAnalysisTuning._prepare_parallel_analysis"), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_table_name", return_value="test"), \
                mock.patch(NAMESPACE + ".ClusterParallelAnalysis.get_tuning_suggestion",
                           return_value={"model": "data_parallel"}), \
                mock.patch('common_func.file_manager.check_path_valid'):
            check = ClusterParallelAnalysisTuning(self.PARAMS1)
            check.process()
            self.assertEqual(os.path.exists(os.path.join(
                self.DIR_PATH, "query", "cluster_parallel_suggestion_-1_-1_-1.json")), True)
