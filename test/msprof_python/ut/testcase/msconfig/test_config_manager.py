#!/usr/bin/python3
# -*- coding: utf-8 -*-
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

import unittest

from msconfig.config_manager import ConfigManager


class TestConfigManager(unittest.TestCase):
    def test_get(self):
        config = ConfigManager.get(ConfigManager.PROF_CONDITION)
        self.assertEqual(len(config.get_data()), 29)
        config = ConfigManager.get(ConfigManager.DATA_PARSERS)
        self.assertEqual(len(config.sections()), 70)
        config = ConfigManager.get(ConfigManager.STARS)
        self.assertEqual(config.has_section('AA'), False)
        config = ConfigManager.get(ConfigManager.TABLES)
        self.assertEqual(len(config.options('BiuCyclesMap')), 9)
        config = ConfigManager.get(ConfigManager.TABLES_TRAINING)
        self.assertEqual(config.get('BiuCyclesMap', 'AA'), '')
        config = ConfigManager.get(ConfigManager.TABLES_OPERATOR)
        self.assertEqual(len(config.items('SummaryGeMap')), 21)
        config = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA)
        self.assertEqual(len(config.sections()), 66)
        config = ConfigManager.get(ConfigManager.DATA_CALCULATOR)
        self.assertEqual(len(config.sections()), 27)
