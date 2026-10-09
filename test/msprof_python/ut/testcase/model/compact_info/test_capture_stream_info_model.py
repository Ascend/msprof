#!/usr/bin/python3
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from unittest.mock import patch, MagicMock
from msmodel.compact_info.capture_stream_info_model import CaptureStreamInfoViewModel
from profiling_bean.db_dto.capture_stream_info_dto import CaptureStreamInfoDto
from common_func.db_name_constant import DBNameConstant


class TestCaptureStreamInfoViewModel(unittest.TestCase):

    @patch('msmodel.compact_info.capture_stream_info_model.DBManager')
    def test_should_get_capture_stream_info_data_when_have_capture_stream_info_dto(self, mock_db_manager):
        mock_cur = MagicMock()
        mock_db_manager.judge_table_exist.return_value = True

        expected_dtos = [
            CaptureStreamInfoDto(
                device_id=0, model_id=5, original_stream_id=10,
                stream_id=1, batch_id=100, capture_status=0, timestamp=1000.0
            ),
            CaptureStreamInfoDto(
                device_id=0, model_id=5, original_stream_id=10,
                stream_id=1, batch_id=100, capture_status=1, timestamp=2000.0
            ),
        ]
        mock_db_manager.fetch_all_data.return_value = expected_dtos

        with CaptureStreamInfoViewModel("/fake/path") as view_model:
            view_model.cur = mock_cur
            result = view_model.get_capture_stream_info_data()

        mock_db_manager.judge_table_exist.assert_called_once_with(
            mock_cur, DBNameConstant.TABLE_CAPTURE_STREAM_INFO
        )
        expected_sql = (
            "SELECT device_id, model_id, original_stream_id, stream_id, batch_id, capture_status, "
            f"timestamp FROM {DBNameConstant.TABLE_CAPTURE_STREAM_INFO}"
        )
        mock_db_manager.fetch_all_data.assert_called_once_with(
            mock_cur, expected_sql, dto_class=CaptureStreamInfoDto
        )

        self.assertEqual(result, expected_dtos)
