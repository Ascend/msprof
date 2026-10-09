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

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from framework.offset_calculator import OffsetCalculator
from msparser.data_struct_size_constant import StructFmt
from msparser.freq.freq_parser import FreqParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.freq.freq_parser'


class TestFreqParser(unittest.TestCase):
    file_list = {DataTag.FREQ: ['lpmFreqConv.data.0.slice_0']}

    def test_parse(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'aic_frequency': 100}]}
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.FreqParser._read_file'), \
                mock.patch('os.path.getsize', return_value=StructFmt.FREQ_DATA_SIZE), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.FreqParser._read_file'), \
                mock.patch('os.path.getsize', return_value=0), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check.parse()
        InfoConfReader()._info_json = {}

    def test_read_file_should_return_2_freq_data_when_parsed_success(self):
        origin_data = [2, 0, 100000000000, 10, 0, 120000000000, 30, 0] + [0] * (len(StructFmt.FREQ_FMT) - 8)
        data = struct.pack(StructFmt.FREQ_FMT, *origin_data)
        offset_calculator = OffsetCalculator(self.file_list.get(DataTag.FREQ), StructFmt.FREQ_DATA_SIZE, 'test')
        InfoConfReader()._dev_cnt = 110
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data),\
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            check = FreqParser(self.file_list, CONFIG)
            check._freq_data = [[InfoConfReader().get_dev_cnt(), 1850]]
            check._read_file('test', StructFmt.FREQ_DATA_SIZE, offset_calculator)
            self.assertEqual(2, len(check._freq_data))
        InfoConfReader()._dev_cnt = 0

    def test_save(self):
        with mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.init'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.check_db'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.create_table'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.insert_data_to_db'), \
                mock.patch('msmodel.freq.freq_parser_model.FreqParserModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = FreqParser(self.file_list, CONFIG)
            check._freq_data = [[100, 10], [120, 30]]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.FreqParser.parse'), \
                mock.patch(NAMESPACE + '.FreqParser.save'):
            FreqParser(self.file_list, CONFIG).ms_run()
        FreqParser({DataTag.FREQ: []}, CONFIG).ms_run()
