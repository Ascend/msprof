#!/usr/bin/env python
# coding=utf-8
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
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
from common_func.platform.chip_manager import ChipManager
from framework.config_data_parsers import ConfigDataParsers
from msconfig.config_manager import ConfigManager
from profiling_bean.prof_enum.chip_model import ChipModel


class TestConfigDataParsers(unittest.TestCase):
    def test_get_parsers(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        ConfigDataParsers.get_parsers(ConfigManager.DATA_CALCULATOR, str(ChipModel.CHIP_V3_1_0.value), False)

    def test_load_can_cpp_parse_or_calculate_host_data(self):
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("NpuMemParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("HashDicParser")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("AscendTaskCalculator")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_not_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("NpuMemParser")
        self.assertFalse(ret)
