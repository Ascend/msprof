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
    def test_ccu_uses_existing_device_scene_gates(self):
        for enabled, all_export, version, task in (
                (True, True, True, True), (False, True, True, True),
                (True, False, True, True), (True, True, False, True),
                (True, True, True, False)):
            with self.subTest(enabled=enabled, all_export=all_export, version=version, task=task), \
                    mock.patch.object(DeviceParseScene, "is_cpp_enable", return_value=enabled), \
                    mock.patch.object(CannCalculatorScene, "is_cpp_enable", return_value=False), \
                    mock.patch.object(InfoConfReader(), "get_device_list", return_value=["0"]), \
                    mock.patch.object(InfoConfReader(), "is_all_export_version", return_value=version), \
                    mock.patch.object(ProfilingScene(), "is_all_export", return_value=all_export), \
                    mock.patch.object(ChipManager(), "chip_id", ChipModel.CHIP_V6_1_0), \
                    mock.patch.object(ChipManager(), "is_chip_v4", return_value=False):
                names = self._parser_names(ConfigDataParsers.get_parsers(
                    ConfigManager.DATA_PARSERS, str(ChipModel.CHIP_V6_1_0.value), task))
                for name in ("CCUMissionParser", "CCUChannelParser"):
                    self.assertEqual(name in names, not (enabled and all_export and version and task))
                self.assertIn("AicpuAddInfoParser", names)

    def test_device_scene_whitelist_keeps_v61_disabled_until_unified_switch(self):
        self.assertNotIn(ChipModel.CHIP_V6_1_0, DeviceParseScene.SCENE_CHIP_WHITELIST)
        self.assertIn(ChipModel.CHIP_V4_1_0, DeviceParseScene.SCENE_CHIP_WHITELIST)
        self.assertNotIn(ChipModel.CHIP_V6_2_0, DeviceParseScene.SCENE_CHIP_WHITELIST)

    def test_ccu_native_ownership_is_limited_to_v61(self):
        for chip in (ChipModel.CHIP_V4_1_0, ChipModel.CHIP_V6_1_0, ChipModel.CHIP_V6_2_0):
            with self.subTest(chip=chip), mock.patch.object(ChipManager(), "chip_id", chip):
                for parser in ("CCUMissionParser", "CCUChannelParser"):
                    self.assertEqual(
                        ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data(parser),
                        chip == ChipModel.CHIP_V6_1_0)

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
        self.assertTrue(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("Mc2CommInfoParser")
        self.assertTrue(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("RuntimeOpInfoParser")
        self.assertTrue(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("StaticOpMemParser")
        self.assertFalse(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("StreamExpandSpecParser")
        self.assertTrue(ret)
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_host_data("CCUAddInfoParser")
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
    def test_get_parsers_should_skip_capture_and_mc2_when_host_cpp_is_enabled(
            self, _host_cpp_enable, _device_cpp_enable):
        InfoConfReader()._sample_json = {'devices': str(64)}

        with mock.patch.object(ProfilingScene(), "is_all_export", return_value=False):
            parsers = ConfigDataParsers.get_parsers(
                ConfigManager.DATA_PARSERS, str(ChipModel.CHIP_V3_3_0.value), False)

        parser_names = self._parser_names(parsers)
        self.assertNotIn("CaptureStreamInfoParser", parser_names)
        self.assertNotIn("Mc2CommInfoParser", parser_names)
        self.assertNotIn("RuntimeOpInfoParser", parser_names)
        self.assertIn("StaticOpMemParser", parser_names)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("AscendTaskCalculator")
        self.assertTrue(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_true_when_given_not_in_whitelist(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("NpuMemParser")
        self.assertFalse(ret)

    def test_load_can_cpp_parse_or_calculate_device_data_should_return_false_for_v6_1(self):
        with mock.patch('framework.config_data_parsers.ChipManager') as chip_manager:
            chip_manager.return_value.chip_id = ChipModel.CHIP_V6_1_0
            chip_manager.return_value.is_chip_v4.return_value = False
            ret = ConfigDataParsers._load_can_cpp_parse_or_calculate_device_data("AscendTaskCalculator")
        self.assertFalse(ret)
