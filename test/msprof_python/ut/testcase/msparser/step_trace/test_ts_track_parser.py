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
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.data_struct_size_constant import StructFmt
from msparser.step_trace.ts_track_parser import TstrackParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.step_trace.ts_track_parser"

file_list = {DataTag.TS_TRACK: ['ts_track.data.0.slice_0'],
             DataTag.HELPER_MODEL_WITH_Q: ['DATA_PREPROCESS.AICPU_MODEL.0.slice_0']}
sample_config = CONFIG


class TestTstrackParser(unittest.TestCase):
    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TstrackParser.parse'), \
                mock.patch(NAMESPACE + '.TstrackParser.save'), \
                mock.patch(NAMESPACE + '.StepTableBuilder.run'):
            data_parser = TstrackParser(file_list, sample_config)
            data_parser.ms_run()

    def test_ms_run_error(self):
        with mock.patch(NAMESPACE + '.TstrackParser.parse', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.TstrackParser.save'), \
                mock.patch(NAMESPACE + '.StepTableBuilder.run'):
            data_parser = TstrackParser(file_list, sample_config)
            data_parser.ms_run()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.TstrackParser.parse_binary_data'):
            data_parser = TstrackParser(file_list, sample_config)
            data_parser.parse()

    def test_parse_binary_data(self):
        data = b'\x01\x03\x0e\x00\x00\x00\x00\x02\x00\x01\x00\x00\x00\x03\x00\xf2\xf8\xb2\xeb\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.TstrackParser.get_file_path_and_check', return_value=""), \
                mock.patch('os.path.getsize', return_value=40), \
                mock.patch('builtins.open', mock.mock_open(read_data="")), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.TaskFlipReader.read_binary_data'):
            data_parser = TstrackParser(file_list, sample_config)
            data_parser.parse_binary_data(file_list.get(DataTag.TS_TRACK, []), StructFmt.HELPER_MODEL_WITH_Q_FMT_SIZE,
                                          StructFmt.HELPER_HEADER_FMT)
