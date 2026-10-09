#!/usr/bin/python3
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

import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.npu_mem.npu_module_mem_parser import NpuModuleMemParser
from profiling_bean.prof_enum.data_tag import DataTag
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msparser.npu_mem.npu_module_mem_parser'


class TestNpuModuleMemParser(unittest.TestCase):
    file_list = {DataTag.NPU_MODULE_MEM: ['npu_module_mem.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuModuleMemParser.parse'), \
                mock.patch(NAMESPACE + '.NpuModuleMemParser.save'):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NpuModuleMemParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.finalize'):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_module_mem_data", ['123', '1', 1, '123', 1, 1000, 1000, 1, 0, 'NPU:0'])
            result = check.save()
        with mock.patch(NAMESPACE + '.NpuAiStackMemModel.init'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.flush'), \
                mock.patch(NAMESPACE + '.NpuAiStackMemModel.finalize'):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_module_mem_data", [])
            result = check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=0):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.NpuModuleMemParser._process_npu_module_mem_data'):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check.parse()

    def test_process_npu_module_mem_data_should_parse_data_in_correct_format_when_data_exist(self):
        npu_module_mem_data_new = struct.pack('=IIQQ', 7, 0, 1000, 4096)
        with mock.patch('builtins.open', mock.mock_open(read_data=npu_module_mem_data_new)), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=npu_module_mem_data_new):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check._process_npu_module_mem_data('test', 24, [])
        self.assertEqual(check._npu_module_mem_data, [[7, 1000, 4096, 'NPU:0']])

    def test_process_npu_module_mem_data_should_get_warning_skip_data_when_negative_reversed(self):
        npu_module_mem_data_new = struct.pack('=IIQQ', 1, 2, 3, 9223372036854775808)
        with mock.patch('builtins.open', mock.mock_open(read_data=npu_module_mem_data_new)), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=npu_module_mem_data_new):
            check = NpuModuleMemParser(self.file_list, CONFIG)
            check._process_npu_module_mem_data('test', 24, [])
        self.assertEqual(check._npu_module_mem_data, [[1, 3, -1, 'NPU:0']])
