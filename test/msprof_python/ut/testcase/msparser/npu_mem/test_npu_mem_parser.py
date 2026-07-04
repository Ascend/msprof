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

import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.npu_mem.npu_mem_bean import NpuMemDataBeanV2
from msparser.npu_mem.npu_mem_parser import decode_npu_mem_v2
from msparser.npu_mem.npu_mem_parser import NpuMemParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.npu_mem.npu_mem_parser'


class TestNpuMemParser(unittest.TestCase):
    file_list = {DataTag.NPU_MEM: ['npu.app.mem.data.0.slice_0', 'npu.mem.data.0.slice_0']}

    def tearDown(self) -> None:
        InfoConfReader()._info_json = {}

    def test_decode_npu_mem_v2(self):
        bean = decode_npu_mem_v2((2, 0, 123456789, 16, 32))
        self.assertIsInstance(bean, NpuMemDataBeanV2)
        self.assertEqual(bean.timestamp, 123456789)
        self.assertEqual(bean.ddr, 16)
        self.assertEqual(bean.hbm, 32)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NpuMemParser.parse'), mock.patch(NAMESPACE + '.NpuMemParser.save'):
            check = NpuMemParser(self.file_list, CONFIG)
            check.ms_run()
        with (
            mock.patch(NAMESPACE + '.NpuMemParser.parse', side_effect=SystemError),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        with (
            mock.patch(NAMESPACE + '.NpuMemModel.init'),
            mock.patch(NAMESPACE + '.NpuMemModel.flush'),
            mock.patch(NAMESPACE + '.NpuMemModel.finalize'),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_mem_data", [1, 0, 128, 6, 128, 0])
            check.save()
        with (
            mock.patch(NAMESPACE + '.NpuMemModel.init'),
            mock.patch(NAMESPACE + '.NpuMemModel.flush'),
            mock.patch(NAMESPACE + '.NpuMemModel.finalize'),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            setattr(check, "_npu_mem_data", [])
            check.save()

    def test_parse(self):
        with (
            mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'),
            mock.patch('os.path.getsize', return_value=0),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            check.parse()
        with (
            mock.patch('os.path.getsize', return_value=128),
            mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'),
            mock.patch(NAMESPACE + '.logging.info'),
            mock.patch(NAMESPACE + '.FileManager.add_complete_file'),
            mock.patch(NAMESPACE + '.NpuMemParser._process_npu_mem_data'),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            check.parse()

    def test_process_npu_mem_data(self):
        npu_mem_data_new = struct.pack('=IIQQQ', 6, 0, 0, 16, 0)
        info_reader_mock = mock.MagicMock()
        info_reader_mock.get_absolute_time_by_sampling_timestamp.return_value = 6
        with (
            mock.patch('builtins.open', mock.mock_open(read_data=npu_mem_data_new)),
            mock.patch(NAMESPACE + '.FileOpen'),
            mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=npu_mem_data_new),
            mock.patch(NAMESPACE + '.InfoConfReader', return_value=info_reader_mock),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            check._process_npu_mem_data('test', 32, [])
        self.assertEqual(check._npu_mem_data, [[0, 16, 0, 6, 16]])

    def test_process_npu_mem_data_v2(self):
        info_reader_mock = mock.MagicMock()
        info_reader_mock.get_collection_version.return_value = "2.0"
        info_reader_mock.get_absolute_time_by_sampling_timestamp.return_value = 987654321
        with (
            mock.patch('builtins.open', mock.mock_open(read_data=b'test')),
            mock.patch(NAMESPACE + '.FileOpen'),
            mock.patch(
                NAMESPACE + '.OffsetCalculator.pre_process', return_value=struct.pack('=IIQQQ', 2, 0, 123456789, 16, 32)
            ),
            mock.patch(NAMESPACE + '.InfoConfReader', return_value=info_reader_mock),
        ):
            check = NpuMemParser(self.file_list, CONFIG)
            check._process_npu_mem_data('test', 32, [])
        self.assertEqual(check._npu_mem_data, [[0, 16, 32, 987654321, 48]])
