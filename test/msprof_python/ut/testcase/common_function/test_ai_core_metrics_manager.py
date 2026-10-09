# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

import unittest

from common_func.platform.ai_core_metrics_manager import AiCoreMetricsManager
from profiling_bean.prof_enum.chip_model import ChipModel


class TestAiCoreMetricsManager(unittest.TestCase):
    def test_set_aicore_metrics_list_should_set_v6_list_when_give_chip_v6(self):
        AiCoreMetricsManager.set_aicore_metrics_list(ChipModel.CHIP_V6_1_0)
        metrics_list = AiCoreMetricsManager.AICORE_METRICS_LIST
        valid_metrics = AiCoreMetricsManager.VALID_METRICS_SET
        ret = ("ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),"
               "ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
               "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s),fixp2ub_write_bw(GB/s)")
        valid_metrics_ret = set(
            AiCoreMetricsManager.AICORE_METRICS_LIST.get(AiCoreMetricsManager.PMU_PIPE).split(",")[:-1]
        )
        self.assertEqual(ret, metrics_list.get(AiCoreMetricsManager.PMU_MEM_UB))
        self.assertEqual(valid_metrics_ret, valid_metrics)

    def test_set_aicore_metrics_list_should_set_default_list_when_give_chip_v1(self):
        AiCoreMetricsManager.set_aicore_metrics_list(ChipModel.CHIP_V1_1_3)
        metrics_list = AiCoreMetricsManager.AICORE_METRICS_LIST
        valid_metrics = AiCoreMetricsManager.VALID_METRICS_SET
        ret = ("ub_read_bw_mte(GB/s),ub_write_bw_mte(GB/s),ub_read_bw_vector(GB/s),ub_write_bw_vector(GB/s),"
               "ub_read_bw_scalar(GB/s),ub_write_bw_scalar(GB/s)")
        valid_metrics_ret = set(
            AiCoreMetricsManager.AICORE_METRICS_LIST.get(AiCoreMetricsManager.PMU_PIPE).split(",")[:-1] +
            AiCoreMetricsManager.AICORE_METRICS_LIST.get(AiCoreMetricsManager.PMU_PIPE_EXCT).split(",")[:-1] +
            AiCoreMetricsManager.AICORE_METRICS_LIST.get(AiCoreMetricsManager.PMU_PIPE_EXECUT).split(",")
        )
        self.assertEqual(ret, metrics_list.get(AiCoreMetricsManager.PMU_MEM_UB))
        self.assertEqual(valid_metrics_ret, valid_metrics)

    def tearDown(self):
        AiCoreMetricsManager.AICORE_METRICS_LIST = AiCoreMetricsManager.AICORE_METRICS_LIST_DEFAULT
        AiCoreMetricsManager.VALID_METRICS_SET = AiCoreMetricsManager.VALID_METRICS_SET_DEFAULT


if __name__ == '__main__':
    unittest.main()
