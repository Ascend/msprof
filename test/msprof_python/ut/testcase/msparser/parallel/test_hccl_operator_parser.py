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

from constant.constant import clear_dt_project
from msmodel.parallel.cluster_hccl_model import ClusterHCCLViewModel
from msparser.parallel.hccl_operator_parser import HCCLOperatorParser
from profiling_bean.db_dto.step_trace_ge_dto import StepTraceGeDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.parallel.hccl_operator_parser"


class TestHCCLOperatorParser(unittest.TestCase):
    FILE_LIST_1 = {1: ["test"]}
    FILE_LIST_2 = {DataTag.PARALLEL_STRATEGY: ["test"]}
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HCCLOperatorParser')
    SAMPLE_CONFIG = {"result_dir": DIR_PATH}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_ms_run(self):
        hccl_data1 = StepTraceGeDto()
        hccl_data1.tag_id = 10000
        hccl_data1.timestamp = 33
        hccl_data2 = StepTraceGeDto()
        hccl_data2.tag_id = 10001
        hccl_data2.timestamp = 44
        with mock.patch(NAMESPACE + ".TsTrackViewModel.get_hccl_operator_exe_data",
                        return_value=[hccl_data1, hccl_data2]), \
            mock.patch(NAMESPACE + ".ParallelViewModel.get_parallel_table_name", return_value="cluster"), \
                mock.patch(NAMESPACE + ".GeHashViewModel.get_ge_hash_data", return_value={}):
            check = HCCLOperatorParser(self.FILE_LIST_2, self.SAMPLE_CONFIG)
            check.ms_run()
        with ClusterHCCLViewModel(self.DIR_PATH) as _model:
            data = _model.get_hccl_op_data()
        self.assertEqual(not data, False)
