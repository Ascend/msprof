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
import unittest

from constant.constant import CONFIG
from msparser.hardware.ctrl_cpu_parser import ParsingCtrlCPUData
from profiling_bean.prof_enum.data_tag import DataTag


class TestParsingAICPUData(unittest.TestCase):
    file_list = {DataTag.CTRLCPU: ['ctrlcpu.data.0.slice_0']}

    def test_init(self):
        check = ParsingCtrlCPUData(self.file_list, CONFIG)
        self.assertEqual(check.type, 'ctrl')
