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

from msparser.parallel.parallel_query.pipeline_parallel_analysis import PipelineParallelAnalysis

NAMESPACE = "msparser.parallel.parallel_query.pipeline_parallel_analysis"


class TestPipelineParallelAnalysis(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_PipelineParallelAnalysis')
    PARAMS1 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": 1,
               "iteration_id": 2}
    PARAMS2 = {"collection_path": DIR_PATH,
               "npu_id": -1,
               "model_id": -1,
               "iteration_id": -1}

    def test_get_parallel_data(self) -> None:
        with mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_first_field_name", return_value=("rank_id", "R")), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_parallel_condition_and_query_params",
                           return_value=[1, 2]), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_pipeline_parallel_data", return_value=[]):
            check = PipelineParallelAnalysis(self.PARAMS1)
            self.assertEqual(check.get_parallel_data().get("headers")[0], "R")

    def test_get_tuning_suggestion(self) -> None:
        with mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_pipeline_parallel_tuning_data",
                        return_value=[[0.01, 0.05, 0]]), \
                mock.patch(NAMESPACE + ".ClusterParallelViewModel.get_parallel_type"):
            check = PipelineParallelAnalysis(self.PARAMS2)
            self.assertEqual(not check.get_tuning_suggestion().get("suggestion"), False)
