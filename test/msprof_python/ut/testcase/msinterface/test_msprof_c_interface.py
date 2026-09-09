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
from unittest import mock

from common_func.ms_constant.number_constant import NumberConstant
from msinterface.msprof_c_interface import _dump_cann_trace
from msinterface.msprof_c_interface import _dump_device_data
from msinterface.msprof_c_interface import _export_unified_db
from msinterface.msprof_c_interface import _export_timeline
from msinterface.msprof_c_interface import _export_summary
from msinterface.msprof_c_interface import dump_cann_trace
from msinterface.msprof_c_interface import dump_device_data

NAMESPACE = 'msinterface.msprof_c_interface'


class TestMsprofCInterface(unittest.TestCase):
    def test_dump_cann_trace(self):
        with mock.patch('importlib.import_module') as module:
            module.return_value.parser.dump_cann_trace.return_value = NumberConstant.SUCCESS
            _dump_cann_trace("")

    def test_dump_device_data(self):
        with mock.patch('importlib.import_module'):
            _dump_device_data("")

    def test_dump_device_data_uses_existing_scene_and_subprocess(self):
        for enabled in (True, False):
            with self.subTest(enabled=enabled), \
                    mock.patch(NAMESPACE + '.DeviceParseScene') as scene, \
                    mock.patch(NAMESPACE + '.ConfigMgr') as config, \
                    mock.patch(NAMESPACE + '.ProfilingScene') as profiling, \
                    mock.patch(NAMESPACE + '.InfoConfReader') as info, \
                    mock.patch(NAMESPACE + '.run_in_subprocess') as run:
                scene.return_value.is_cpp_enable.return_value = enabled
                config.is_ai_core_sample_based.return_value = False
                config.is_custom_pmu_scene.return_value = False
                profiling.return_value.is_all_export.return_value = True
                info.return_value.is_all_export_version.return_value = True
                dump_device_data("PROF/device_0")
                if enabled:
                    run.assert_called_once_with(_dump_device_data, "PROF/device_0")
                else:
                    run.assert_not_called()

    def test_dump_device_data_passes_prof_root_to_native(self):
        with mock.patch('importlib.import_module') as module:
            _dump_device_data("PROF/device_0")
        module.return_value.parser.dump_device_data.assert_called_once_with("PROF")

    def test_export_unified_db(self):
        with mock.patch('importlib.import_module'):
            _export_unified_db("")

    def test_export_timeline(self):
        with mock.patch('importlib.import_module'):
            _export_timeline("", "")

    def test_export_summary(self):
        with mock.patch('importlib.import_module'):
            _export_summary("")
    def test_host_native_status_preserves_existing_wrapper_contract(self):
        with mock.patch('importlib.import_module') as module:
            module.return_value.parser.dump_cann_trace.return_value = NumberConstant.ERROR
            _dump_cann_trace("host")
            module.return_value.parser.dump_cann_trace.assert_called_once_with("host")

    def test_host_native_uses_existing_subprocess(self):
        with mock.patch(NAMESPACE + '.run_in_subprocess') as run:
            dump_cann_trace("host")
            run.assert_called_once_with(_dump_cann_trace, "host")
