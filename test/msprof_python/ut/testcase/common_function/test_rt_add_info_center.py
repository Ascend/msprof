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
from unittest.mock import patch

from common_func.constant import Constant
from common_func.rt_add_info_center import RTAddInfoCenter
from profiling_bean.db_dto.capture_stream_info_dto import CaptureStreamInfoDto


class TestRTAddInfoCenter(unittest.TestCase):

    @patch('os.path.exists', return_value=False)
    def test_should_build_capture_info_time_range_dict_when_capture_info_time_valid(self, mock_exists):
        center = RTAddInfoCenter(project_path="/fake/path")

        # Manually inject test data into _capture_info_list
        center._capture_info_list = [
            CaptureStreamInfoDto(
                device_id=0, stream_id=1, batch_id=100,
                model_id=5, capture_status=0, timestamp=1000.0
            ),
            CaptureStreamInfoDto(
                device_id=0, stream_id=1, batch_id=100,
                model_id=5, capture_status=1, timestamp=2000.0
            ),
            CaptureStreamInfoDto(
                device_id=0, stream_id=2, batch_id=200,
                model_id=6, capture_status=0, timestamp=1500.0
            ),
        ]

        # Act: Rebuild the time range dict using injected data
        center._capture_info_time_range_dict = center.build_capture_info_time_range_dict()

        # Assert
        expected_dict = {
            (0, 1, 100): (1000.0, 2000.0, 5),
            (0, 2, 200): (1500.0, float('inf'), 6),
        }

        self.assertEqual(center._capture_info_time_range_dict, expected_dict)

    @patch('os.path.exists', return_value=False)
    def test_should_find_matching_model_id_found_when_capture_info_list_valid(self, mock_exists):
        # Arrange
        center = RTAddInfoCenter(project_path="/fake/path")
        center._capture_info_list = [
            CaptureStreamInfoDto(
                device_id=1, stream_id=2, batch_id=300,
                model_id=42, capture_status=0, timestamp=500.0
            ),
            CaptureStreamInfoDto(
                device_id=1, stream_id=2, batch_id=300,
                model_id=42, capture_status=1, timestamp=800.0
            ),
        ]
        center._capture_info_time_range_dict = center.build_capture_info_time_range_dict()

        # Act & Assert: Timestamp inside interval
        self.assertEqual(
            center.find_matching_model_id(device_id=1, stream_id=2, batch_id=300, timestamp=600.0),
            42
        )

        # Boundary timestamps
        self.assertEqual(
            center.find_matching_model_id(device_id=1, stream_id=2, batch_id=300, timestamp=500.0),
            42
        )
        self.assertEqual(
            center.find_matching_model_id(device_id=1, stream_id=2, batch_id=300, timestamp=800.0),
            42
        )

    @patch('os.path.exists', return_value=False)
    def test_should_find_matching_model_id_when_capture_stream_info_not_found_or_outside(self, mock_exists):
        # Arrange
        center = RTAddInfoCenter(project_path="/fake/path")
        center._capture_info_list = [
            CaptureStreamInfoDto(
                device_id=1, stream_id=2, batch_id=300,
                model_id=42, capture_status=0, timestamp=500.0
            ),
            CaptureStreamInfoDto(
                device_id=1, stream_id=2, batch_id=300,
                model_id=42, capture_status=1, timestamp=800.0
            ),
        ]
        center._capture_info_time_range_dict = center.build_capture_info_time_range_dict()

        # Non-existent key
        self.assertEqual(
            center.find_matching_model_id(device_id=99, stream_id=99, batch_id=99, timestamp=600.0),
            Constant.GE_OP_MODEL_ID
        )

        # Timestamp before start
        self.assertEqual(
            center.find_matching_model_id(device_id=1, stream_id=2, batch_id=300, timestamp=400.0),
            Constant.GE_OP_MODEL_ID
        )

        # Timestamp after end
        self.assertEqual(
            center.find_matching_model_id(device_id=1, stream_id=2, batch_id=300, timestamp=900.0),
            Constant.GE_OP_MODEL_ID
        )

    @patch('os.path.exists', return_value=False)
    def test_should_find_matching_model_id_when_capture_stream_info_only_start_no_end(self, mock_exists):
        # Arrange
        center = RTAddInfoCenter(project_path="/fake/path")
        center._capture_info_list = [
            CaptureStreamInfoDto(
                device_id=2, stream_id=3, batch_id=400,
                model_id=99, capture_status=0, timestamp=100.0
            ),
        ]
        center._capture_info_time_range_dict = center.build_capture_info_time_range_dict()

        # Timestamp at start
        self.assertEqual(
            center.find_matching_model_id(2, 3, 400, 100.0),
            99
        )

        # Timestamp far after start (still valid due to missing end → end = inf)
        self.assertEqual(
            center.find_matching_model_id(2, 3, 400, 5000.0),
            99
        )

        # Timestamp just before start → invalid
        self.assertEqual(
            center.find_matching_model_id(2, 3, 400, 99.9),
            Constant.GE_OP_MODEL_ID
        )
