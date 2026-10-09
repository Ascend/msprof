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
import struct
import unittest

from msparser.stars.acc_pmu_parser import AccPmuParser
from profiling_bean.stars.acc_pmu import AccPmuDecoder

NAMESPACE = 'msparser.stars.acc_pmu_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestAccPmuParser(unittest.TestCase):

    def test_decoder(self):
        check = AccPmuParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = AccPmuDecoder(struct.pack("4HQ2H3L4Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
