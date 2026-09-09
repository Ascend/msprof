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

from msinterface.msprof_c_interface import _dump_cann_trace
from msinterface.msprof_c_interface import _dump_device_data
from msinterface.msprof_c_interface import _export_unified_db
from msinterface.msprof_c_interface import _export_timeline
from msinterface.msprof_c_interface import _export_summary
from msinterface.msprof_c_interface import run_pipeline
from msinterface.msprof_c_interface import MSPROF_ERROR
from msinterface.msprof_c_interface import MSPROF_INVALID_PARAM
from msinterface.msprof_c_interface import MSPROF_OK
from msinterface.msprof_c_interface import MSPROF_PIPELINE_DEVICE_DATA

NAMESPACE = 'msinterface.msprof_c_interface'


class TestMsprofCInterface(unittest.TestCase):
    def test_dump_cann_trace(self):
        with mock.patch('importlib.import_module'):
            _dump_cann_trace("")

    def test_dump_device_data(self):
        with mock.patch('importlib.import_module'):
            _dump_device_data("")

    def test_export_unified_db(self):
        with mock.patch('importlib.import_module'):
            _export_unified_db("")

    def test_export_timeline(self):
        with mock.patch('importlib.import_module'):
            _export_timeline("", "")

    def test_export_summary(self):
        with mock.patch('importlib.import_module'):
            _export_summary("")

    def test_run_pipeline(self):
        analysis_module = mock.Mock()
        analysis_module.parser.run_pipeline.return_value = MSPROF_OK
        with mock.patch(NAMESPACE + '.check_so_valid', return_value=True), \
                mock.patch('importlib.import_module', return_value=analysis_module):
            ret = run_pipeline('/tmp/PROF_0', MSPROF_PIPELINE_DEVICE_DATA)
        self.assertEqual(ret, MSPROF_OK)
        analysis_module.parser.run_pipeline.assert_called_once_with(
            '/tmp/PROF_0', MSPROF_PIPELINE_DEVICE_DATA, None, None, None)

    def test_run_pipeline_returns_parser_error_code(self):
        analysis_module = mock.Mock()
        analysis_module.parser.run_pipeline.return_value = MSPROF_INVALID_PARAM
        with mock.patch(NAMESPACE + '.check_so_valid', return_value=True), \
                mock.patch('importlib.import_module', return_value=analysis_module):
            self.assertEqual(run_pipeline('', MSPROF_PIPELINE_DEVICE_DATA), MSPROF_INVALID_PARAM)

    def test_run_pipeline_returns_error_when_so_is_invalid(self):
        with mock.patch(NAMESPACE + '.check_so_valid', return_value=False), \
                mock.patch('importlib.import_module') as import_module:
            self.assertEqual(run_pipeline('/tmp/PROF_0', MSPROF_PIPELINE_DEVICE_DATA), MSPROF_ERROR)
        import_module.assert_not_called()

    def test_run_pipeline_returns_error_when_module_load_fails(self):
        with mock.patch(NAMESPACE + '.check_so_valid', return_value=True), \
                mock.patch('importlib.import_module', side_effect=ImportError):
            self.assertEqual(run_pipeline('/tmp/PROF_0', MSPROF_PIPELINE_DEVICE_DATA), MSPROF_ERROR)
