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
from unittest.mock import patch
from msparser.compact_info.stream_expand_spec_reader import StreamExpandSpecReader


class TestStreamExpandSpecReader(unittest.TestCase):

    @patch('os.path.exists')
    @patch('common_func.path_manager.PathManager.get_db_path')
    @patch('msmodel.compact_info.stream_expand_spec_model.StreamExpandSpecViewModel')
    def test_load_stream_expand_spec_success(self, mock_stream_expand_spec_view_model, mock_get_db_path,
                                             mock_path_exists):
        mock_get_db_path.return_value = '/host/sqlite/stream_expand_spec.db'
        mock_path_exists.return_value = True
        mock_model = mock_stream_expand_spec_view_model.return_value.__enter__.return_value
        mock_model.get_stream_expand_spec_data.return_value = [[1, 2], [3, 4]]
        reader = StreamExpandSpecReader()
        reader.load_stream_expand_spec('/host/sqlite/stream_expand_spec.db')
        self.assertEqual(reader._stream_expand_spec, 0)
        self.assertFalse(reader._is_load_stream_expand)

    @patch('msmodel.compact_info.stream_expand_spec_model.StreamExpandSpecViewModel')
    def test_load_stream_expand_spec_empty_data(self, mock_stream_expand_spec_view_model):
        mock_model = mock_stream_expand_spec_view_model.return_value.__enter__.return_value
        mock_model.get_stream_expand_spec_data.return_value = []

        reader = StreamExpandSpecReader()
        reader.load_stream_expand_spec('/host/sqlite/stream_expand_spec.db')

        self.assertFalse(reader.is_stream_expand)