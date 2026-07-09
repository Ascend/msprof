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
function: test msproftx_parser
Copyright Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.
"""
import os
import shutil
import struct
import unittest
from unittest import mock

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from msparser.data_struct_size_constant import StructFmt
from msparser.msproftx.msproftx_parser import MsprofTxParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.msproftx.msproftx_parser'


class TestMsprofTxParser(unittest.TestCase):
    file_list = {
        DataTag.MSPROFTX: [
            'aging.additional.msproftx.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "msproftx")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0',
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setUpClass(cls):
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_msproftx_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_msproftx_data(cls):
        tx_data = [23130] + [0] * 5 + \
                  [0, 0, 0, 0, 0, 153729, 153789, 0, 1, 0, 0, 8675044945407, 8675044945407, 3, 1, 0] + \
                  [b's' * 155 + b'\0']
        tx_ex_data = [23130] + [0] * 5 + \
                     [1, 0, 0, 0, 0, 153729, 153789, 0, 1, 0, 0, 8675044945407, 8675044945407, 3, 1, 0] + \
                     [b's' * 155 + b'\0']
        msproftx_data = [tx_data, tx_ex_data]
        with FdOpen(os.path.join(cls.DATA_PATH, "aging.additional.msproftx.slice_0"), operate="wb") as f:
            for data in msproftx_data:
                bytes_data = struct.pack(StructFmt.MSPROFTX_FMT, *data)
                f.write(bytes_data)

    def test_ms_run_should_return_when_bin_data_is_lost(self):
        with mock.patch(NAMESPACE + '.MsprofTxParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxParser.save'):
            file_list = {DataTag.STATIC_OP_MEM: []}
            MsprofTxParser(file_list, self.CONFIG).ms_run()

    def test_parse_should_raise_OSError_when_open_file_failed(self):
        with mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, self.CONFIG)
            check.parse()
            self.assertFalse(check._msproftx_data)
            self.assertFalse(check._msproftx_ex_data_dict)

    def test_parse_should_run_success_when_open_file_success(self):
        with mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = MsprofTxParser(self.file_list, self.CONFIG)
            check.parse()
            self.assertTrue(check._msproftx_data)
            self.assertTrue(check._msproftx_ex_data_dict)
            check.save()
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MsprofTxParser.parse'), \
                mock.patch(NAMESPACE + '.MsprofTxParser.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            MsprofTxParser(self.file_list, self.CONFIG).ms_run()
