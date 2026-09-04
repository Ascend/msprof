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
from unittest import mock

from common_func.cpp_enable_scene import CannCalculatorScene, DeviceParseScene
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from framework.config_data_parsers import ConfigDataParsers
from msconfig.config_manager import ConfigManager
from profiling_bean.prof_enum.chip_model import ChipModel


class TestConfigDataParsers(unittest.TestCase):
    @staticmethod
    def _parser_names(parsers):
        return {parser.__name__ for level_parsers in parsers.values() for parser in level_parsers}

    def test_get_parsers(self):
        InfoConfReader()._sample_json = {'devices': '0'}
        parsers = ConfigDataParsers.get_parsers(
            ConfigManager.DATA_CALCULATOR, str(ChipModel.CHIP_V3_1_0.value), False)
        self.assertIsInstance(parsers, dict)

    def test_load_can_cpp_parse_or_calculate_host_data(self):
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("NpuMemParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("HashDicParser")
        self.assertTrue(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("CaptureStreamInfoParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("Mc2CommInfoParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("RuntimeOpInfoParser")
        self.assertTrue(ret)

    @mock.patch.object(DeviceParseScene, "is_cpp_enable", return_value=False)
    @mock.patch.object(CannCalculatorScene, "is_cpp_enable", return_value=False)
    def test_get_parsers_should_keep_capture_and_mc2_python_fallback_when_host_cpp_is_disabled(
            self, _host_cpp_enable, _device_cpp_enable):
        InfoConfReader()._sample_json = {'devices': str(64)}

        with mock.patch.object(ProfilingScene(), "is_all_export", return_value=False):
            parsers = ConfigDataParsers.get_parsers(
                ConfigManager.DATA_PARSERS, str(ChipModel.CHIP_V3_3_0.value), False)

        parser_names = self._parser_names(parsers)
        self.assertIn("CaptureStreamInfoParser", parser_names)
        self.assertIn("Mc2CommInfoParser", parser_names)

    @mock.patch.object(DeviceParseScene, "is_cpp_enable", return_value=False)
    @mock.patch.object(CannCalculatorScene, "is_cpp_enable", return_value=True)
    def test_get_parsers_should_keep_capture_and_mc2_when_host_cpp_is_enabled(
            self, _host_cpp_enable, _device_cpp_enable):
        InfoConfReader()._sample_json = {'devices': str(64)}

        with mock.patch.object(ProfilingScene(), "is_all_export", return_value=False):
            parsers = ConfigDataParsers.get_parsers(
                ConfigManager.DATA_PARSERS, str(ChipModel.CHIP_V3_3_0.value), False)

        parser_names = self._parser_names(parsers)
        self.assertIn("CaptureStreamInfoParser", parser_names)
        self.assertIn("Mc2CommInfoParser", parser_names)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("AscendTaskCalculator")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_not_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("NpuMemParser")
        self.assertFalse(ret)
