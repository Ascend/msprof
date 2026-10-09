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

import unittest
from unittest import mock
from unittest.mock import patch, Mock

from common_func.ms_constant.number_constant import NumberConstant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from msparser.compact_info.stream_expand_spec_parser import StreamExpandSpecParser

NAMESPACE = 'msparser.compact_info.stream_expand_spec_parser.StreamExpandSpecParser'
MODEL_NAMESPACE = 'msmodel.compact_info.stream_expand_spec_model'


class TestStreamExpandSpecParser(unittest.TestCase):
    file_list = {
        DataTag.STREAM_EXPAND: [
            'aging.compact.expand_stream_spec.slice_0',
            'unaging.compact.expand_stream_spec.slice_0'
        ]
    }

    sample_config = {
        "result_dir": "/path/to/result_dir"
    }

    @patch('msparser.compact_info.stream_expand_spec_parser.PathManager.get_data_file_path')
    @patch('os.path.exists')
    @patch('os.path.getsize')
    @patch('msparser.compact_info.stream_expand_spec_parser.FileOpen')
    @patch('struct.unpack')
    def test_read_binary_data_success(self, mock_unpack, mock_file_open, mock_getsize, mock_exists, mock_get_path):
        parser = StreamExpandSpecParser(self.file_list, self.sample_config)
        mock_get_path.return_value = "valid_file_path"
        mock_exists.return_value = True
        mock_getsize.return_value = StructFmt.STREAM_EXPAND_FMT_SIZE
        mock_file_open.return_value.__enter__.return_value.file_reader.read.return_value = b'\x00' * StructFmt.STREAM_EXPAND_FMT_SIZE
        parser.calculate.pre_process = Mock()
        parser.calculate.pre_process.return_value = b'\x00' * StructFmt.STREAM_EXPAND_FMT_SIZE
        mock_unpack.return_value = (0, 0, 0, 0, 0, 0, 0)
        result = parser.read_binary_data("valid_file")
        assert result == NumberConstant.SUCCESS
        assert parser._stream_expand_data == [[0]]

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '._original_data_handler') as mock_handler, \
                mock.patch('common_func.msvp_common.is_valid_original_data', return_value=True):
            parser = StreamExpandSpecParser(self.file_list, self.sample_config)
            parser.start_parsing_data_file()

            self.assertEqual(mock_handler.call_count, 2)
            mock_handler.assert_any_call('aging.compact.expand_stream_spec.slice_0')
            mock_handler.assert_any_call('unaging.compact.expand_stream_spec.slice_0')

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.start_parsing_data_file') as mock_parse, \
                mock.patch(NAMESPACE + '.save') as mock_save:
            parser = StreamExpandSpecParser(self.file_list, self.sample_config)
            parser.ms_run()
            mock_parse.assert_called_once()
            mock_save.assert_called_once()

            mock_parse.reset_mock()
            mock_save.reset_mock()
            empty_parser = StreamExpandSpecParser({DataTag.STREAM_EXPAND: []}, self.sample_config)
            empty_parser.ms_run()
            mock_parse.assert_not_called()
            mock_save.assert_not_called()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE) as mock_model_cls:
            mock_model_inst = mock_model_cls.return_value.__enter__.return_value

            parser = StreamExpandSpecParser(self.file_list, self.sample_config)
            parser._stream_expand_data = [[1]]
            parser.save()

            mock_model_inst.reset_mock()
            parser._stream_expand_data = []
            parser.save()
            mock_model_inst.flush.assert_not_called()