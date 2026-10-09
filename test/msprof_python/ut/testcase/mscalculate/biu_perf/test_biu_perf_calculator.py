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
from unittest import mock

from common_func.msprof_exception import ProfException
from constant.constant import CONFIG
from mscalculate.biu_perf.biu_perf_calculator import BiuPerfCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.biu_perf.biu_perf_calculator'


class TestBiuPerfCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MonitorFlowCalculator.ms_run'), \
                mock.patch(NAMESPACE + '.MonitorCyclesCalculator.ms_run'):
            BiuPerfCalculator({}, CONFIG).ms_run()

    with mock.patch(NAMESPACE + '.MonitorFlowCalculator.ms_run'), \
            mock.patch(NAMESPACE + '.MonitorCyclesCalculator.ms_run', side_effect=ProfException(0)):
        BiuPerfCalculator({}, CONFIG).ms_run()


if __name__ == '__main__':
    unittest.main()