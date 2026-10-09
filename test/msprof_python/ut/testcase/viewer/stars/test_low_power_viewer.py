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
from common_func.info_conf_reader import InfoConfReader
from viewer.stars.low_power_viewer import LowPowerViewer


class TestLowPowerViewer(unittest.TestCase):

    def test_get_trace_timeline_should_return_timeline_format_when_only_ddie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(4, len(ret))

    def test_get_trace_timeline_should_return_timeline_format_when_only_udie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 2, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        # udie 数据 (die_id 2,3) 应该被忽略，所以只返回元数据
        self.assertEqual(2, len(ret))

    def test_get_trace_timeline_should_return_timeline_format_when_ddie_udie_data_exists(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        data = [
            (69869729100545, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),
            (69869749580545, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20)
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        self.assertEqual(3, len(ret))

    def test_get_trace_timeline_should_return_timeline_format_with_new_logic_only_ddie_processed(self):
        InfoConfReader()._info_json = {"pid": '0'}
        low_power_viewer = LowPowerViewer({}, {'data_type': "low_power"})
        # 包含 die_id 0, 1 (应该被处理) 和 die_id 2, 3 (现在应该被忽略) 的数据
        data = [
            (69869729100545, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20),  # die_id 0 - 应该被处理
            (69869729200545, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30),  # die_id 1 - 应该被处理
            (69869729300545, 2, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40),  # die_id 2 - 现在应该被忽略
            (69869729400545, 3, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50),  # die_id 3 - 现在应该被忽略
        ]
        ret = low_power_viewer.get_trace_timeline(data)
        # 只处理 die_id 0 和 1，
        self.assertEqual(4, len(ret))
