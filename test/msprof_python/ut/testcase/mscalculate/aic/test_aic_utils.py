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

from common_func.platform.chip_manager import ChipManager
from mscalculate.aic.aic_utils import AicPmuUtils
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.aic.aic_utils'


class TestAicPmuUtils(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_remove_redundant(self):
        ai_core_profiling_events = {'icache_req_ratio': [1], 'vec_fp16_128lane_ratio': [2],
                                    'vec_fp16_64lane_ratio': [3], "ub_read_bw_mte(GB/s)": [4],
                                    "ub_write_bw_mte(GB/s)": [5], "l2_write_bw(GB/s)": [6],
                                    "main_mem_write_bw(GB/s)": [7]
                                    }

        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        check = AicPmuUtils()
        check.remove_redundant(ai_core_profiling_events)
        self.assertEqual(ai_core_profiling_events, {})

        ai_core_profiling_events = {"l2_read_bw(GB/s)": [1], "l2_write_bw(GB/s)": [2]
                                    }
        check = AicPmuUtils()
        check.remove_redundant(ai_core_profiling_events)
        self.assertEqual(ai_core_profiling_events, {})
