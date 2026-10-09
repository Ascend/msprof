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
import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.ge.ge_logic_stream_parser import GeLogicStreamParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_logic_stream_parser'
FILE_DICT = {DataTag.GE_LOGIC_STREAM_INFO: ['unaging.additional.logic_stream_info.slice_0']}


class TestGeLogicStreamInfoParser(unittest.TestCase):
    def test_ms_run_should_failed_when_the_fileList_is_empty(self):
        check = GeLogicStreamParser({}, CONFIG)
        check.ms_run()

    def test_ms_run_should_success_when_the_fileList_is_not_empty(self):
        ctx_data = (23130, 0, 0, 0, 0, 0, 1, 2, *(120,) * 56)
        struct_data = struct.pack("HHIIIQII56I", *ctx_data)
        with mock.patch('builtins.open', mock.mock_open(read_data=struct_data)), \
                mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=struct_data):
            check = GeLogicStreamParser(FILE_DICT, CONFIG)
            check.ms_run()

    def test_parse_should_success_when_the_data_is_correct(self):
        ctx_data = (23130, 0, 0, 0, 0, 0, 1, 2, *(120,) * 56)
        struct_data = struct.pack("HHIIIQII56I", *ctx_data)
        with mock.patch('builtins.open', mock.mock_open(read_data=struct_data)), \
                mock.patch('msparser.interface.data_parser.DataParser.get_file_path_and_check'), \
                mock.patch('os.path.getsize', return_value=256), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('framework.offset_calculator.OffsetCalculator.pre_process', return_value=struct_data):
            check = GeLogicStreamParser(FILE_DICT, CONFIG)
            check.parse()
