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

import os
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.cluster_info.communication_analyzer_model import CommunicationAnalyzerModel

NAMESPACE = 'msmodel.cluster_info.communication_analyzer_model'


class TestCommunicationAnalyzerModel(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "analyze")
    communication_data = {
        "op_name@group_name": {
            "Communication Time Info": {
                "Start Timestamp(us)": 0.5,
                "Elapse Time(ms)": 0.5,
                "Transit Time(ms)": 0.1,
                "Wait Time(ms)": 0.5,
                "Synchronization Time(ms)": 0.0,
                "Idle Time(ms)": 0.5,
                "Wait Time Ratio": 0.2,
                "Synchronization Time Ratio": 0.0
            },
            "Communication Bandwidth Info": {
                "RDMA": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Large Packet Ratio": 0,
                    "Size Distribution": {}
                },
                "HCCS": {
                    "Transit Size(MB)": 0.1,
                    "Transit Time(ms)": 0.1,
                    "Bandwidth(GB/s)": 1.0,
                    "Large Packet Ratio": 0.0,
                    "Size Distribution": {"0.001024": [7, 0.1]}
                },
                "SDMA": {
                    "Transit Size(MB)": 0.1,
                    "Transit Time(ms)": 0.1,
                    "Bandwidth(GB/s)": 1.0,
                    "Large Packet Ratio": 0,
                    "Size Distribution": {}
                }
            }
        }
    }
    matrix_data = {
        "op_name@group_name": {
            "0-0": {
                "Transport Type": "LOCAL",
                "Transit Size(MB)": 0.1,
                "Transit Time(ms)": 0.2,
                "Bandwidth(GB/s)": 1.4
            },
            "0-7": {
                "Transport Type": "HCCS",
                "Transit Size(MB)": 0.3,
                "Transit Time(ms)": 0.4,
                "Bandwidth(GB/s)": 1.1
            },
            "0-6": {
                "Transport Type": "HCCS",
                "Transit Size(MB)": 0.5,
                "Transit Time(ms)": 0.6,
                "Bandwidth(GB/s)": 1.2
            }
        }
    }

    def test_flush_communication_data_to_db_when_analyzer_type_is_communication(self):
        with mock.patch("msmodel.interface.base_model.BaseModel.insert_data_to_db"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzerModel.get_op_data", return_value=([], [])):
            model = CommunicationAnalyzerModel(self.DIR_PATH,
                                               [DBNameConstant.TABLE_COMM_ANALYZER_TIME,
                                                DBNameConstant.TABLE_COMM_ANALYZER_BAND])
            model.flush_communication_data_to_db(self.communication_data)

    def test_flush_communication_data_to_db_when_analyzer_type_is_communication_matrix(self):
        with mock.patch("msmodel.interface.base_model.BaseModel.insert_data_to_db"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzerModel.get_matrix_data", return_value=([], [])):
            model = CommunicationAnalyzerModel(self.DIR_PATH, [DBNameConstant.TABLE_COMM_ANALYZER_MATRIX])
            model.flush_communication_data_to_db(self.matrix_data)

    def test_get_op_data_should_return_correct_result(self):
        op_name = "op_name"
        group_name = "group_name"
        expected_time = [[op_name, group_name, 0.5, 0.5, 0.1, 0.5, 0.0, 0.5]]
        expected_band = [
            [op_name, group_name, "HCCS", 0.1, 0.1, 1.0, 0.0, '0.001024', 7, 0.1],
            [op_name, group_name, "SDMA", 0.1, 0.1, 1.0, 0, 0, 0, 0]
        ]
        model = CommunicationAnalyzerModel(self.DIR_PATH,
                                           [DBNameConstant.TABLE_COMM_ANALYZER_TIME,
                                            DBNameConstant.TABLE_COMM_ANALYZER_BAND])
        time_result, band_result = model.get_op_data(self.communication_data)
        self.assertEqual(expected_time, time_result)
        self.assertEqual(expected_band, band_result)

    def test_get_op_data_should_return_none_when_input_is_none(self):
        model = CommunicationAnalyzerModel(self.DIR_PATH,
                                           [DBNameConstant.TABLE_COMM_ANALYZER_TIME,
                                            DBNameConstant.TABLE_COMM_ANALYZER_BAND])
        time_result, band_result = model.get_op_data(None)
        self.assertEqual([], time_result)
        self.assertEqual([], band_result)

    def test_get_matrix_data_should_return_correct_result(self):
        op_name = "op_name"
        group_name = "group_name"
        expected_matrix = [
            [op_name, group_name, "0", "0", "LOCAL", 0.1, 0.2, 1.4],
            [op_name, group_name, "0", "7", "HCCS", 0.3, 0.4, 1.1],
            [op_name, group_name, "0", "6", "HCCS", 0.5, 0.6, 1.2]
        ]
        model = CommunicationAnalyzerModel(self.DIR_PATH, [DBNameConstant.TABLE_COMM_ANALYZER_MATRIX])
        matrix_result = model.get_matrix_data(self.matrix_data)
        self.assertEqual(expected_matrix, matrix_result)

    def test_get_matrix_data_should_return_none_when_input_is_none(self):
        model = CommunicationAnalyzerModel(self.DIR_PATH, [DBNameConstant.TABLE_COMM_ANALYZER_MATRIX])
        matrix_result = model.get_matrix_data(None)
        self.assertEqual([], matrix_result)