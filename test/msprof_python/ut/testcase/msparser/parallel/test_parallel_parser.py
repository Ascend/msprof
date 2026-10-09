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

from common_func.db_manager import DBManager
from constant.constant import clear_dt_project
from msmodel.parallel.parallel_model import ParallelViewModel
from msparser.parallel.parallel_parser import ParallelParser
from profiling_bean.db_dto.hccl_operator_dto import HCCLOperatorDto
from profiling_bean.db_dto.time_section_dto import TimeSectionDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.parallel.parallel_parser"


class TestClusterParallelParser(unittest.TestCase):
    FILE_LIST_1 = {1: ["test"]}
    FILE_LIST_2 = {DataTag.PARALLEL_STRATEGY: ["test"]}
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelParser')
    SAMPLE_CONFIG = {"result_dir": DIR_PATH}
    op_obj = HCCLOperatorDto()
    iter_obj = TimeSectionDto()
    SIDE_EFFECT = [[op_obj], [iter_obj]]

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".AICpuFromTsCalculator.state_to_timeline", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_iter_time_data", return_value=[[5, 6]]), \
                mock.patch(NAMESPACE + ".SectionCalculator.merge_continuous_intervals"), \
                mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name", return_value="cluster"), \
                mock.patch(NAMESPACE + ".SectionCalculator.compute_overlap_time", side_effect=self.SIDE_EFFECT):
            check = ParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), False)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), False)

    def test_ms_run_without_hccl_op_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[]):
            check = ParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)

    def test_ms_run_without_compute_op_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_data", return_value=[]), \
                mock.patch(NAMESPACE + ".AICpuFromTsCalculator.state_to_timeline", return_value=[[3, 4]]):
            check = ParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)

    def test_ms_run_without_step_trace_data(self):
        with mock.patch(NAMESPACE + ".ClusterHCCLViewModel.get_hccl_op_data", return_value=[[1, 2]]), \
                mock.patch(NAMESPACE + ".HwtsIterViewModel.get_ai_core_op_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_ai_cpu_data", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".AICpuFromTsCalculator.state_to_timeline", return_value=[[3, 4]]), \
                mock.patch(NAMESPACE + ".TsTrackViewModel.get_iter_time_data", return_value=[]):
            check = ParallelParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ParallelViewModel(self.DIR_PATH) as _model:
            sql1 = "select count(0) from HcclOperatorOverlap"
            sql2 = "select count(0) from ComputationTime"
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql1), True)
            self.assertEqual(not DBManager.fetch_all_data(_model.cur, sql2), True)
