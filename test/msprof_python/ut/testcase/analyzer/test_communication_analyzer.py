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
import os
import unittest
from unittest import mock

import pytest

from analyzer.communication_analyzer import CommunicationAnalyzer
from common_func.info_conf_reader import InfoConfReader
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.meta_parser import OpTaskBundle
from msmodel.cluster_info.communication_model import OpTaskData
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
NAMESPACE = 'analyzer.communication_analyzer'


class Event:
    def __init__(self, hccl_name: str, timestamp: int):
        self.hccl_name = hccl_name
        self.op_name = 'hcom_allReduce__1'
        self.group_name = 'group_name'
        self.size = 1000 ** 2
        self.duration = 10
        self.plane_id = 0
        self.iteration = 0
        self.transport_type = StrConstant.RDMA
        self.timestamp = timestamp
        self.src_rank = 0
        self.dst_rank = 1


class TestCommunicationAnalyzer(unittest.TestCase):
    collection_path = 'test'
    analyzed_data = {
        'op_name@group_name': {
            -1: {
                "Communication Time Info": {
                    "Start Timestamp(us)": 0,
                    "Elapse Time(ms)": 2,
                    "Transit Time(ms)": 1,
                    "Wait Time(ms)": 0.5,
                    "Synchronization Time(ms)": 0.5,
                    "Idle Time(ms)": 0
                },
                "Communication Bandwidth Info": {
                    "RDMA": {
                        "Transit Size(MB)": 0,
                        "Transit Time(ms)": 0,
                        "Bandwidth(GB/s)": 0,
                        "Bandwidth(Utilization)": 0,
                        "Size Distribution": {}
                    },
                    "SDMA": {
                        "Transit Size(MB)": 0,
                        "Transit Time(ms)": 0,
                        "Bandwidth(GB/s)": 0,
                        "Bandwidth(Utilization)": 0,
                        "Size Distribution": {}
                    },
                    "HCCS": {
                        "Transit Size(MB)": 0,
                        "Transit Time(ms)": 0,
                        "Bandwidth(GB/s)": 0,
                        "Bandwidth(Utilization)": 0,
                        "Size Distribution": {}
                    },
                    "PCIE": {
                        "Transit Size(MB)": 0,
                        "Transit Time(ms)": 0,
                        "Bandwidth(GB/s)": 0,
                        "Bandwidth(Utilization)": 0,
                        "Size Distribution": {}
                    },
                    "SIO": {
                        "Transit Size(MB)": 0,
                        "Transit Time(ms)": 0,
                        "Bandwidth(GB/s)": 0,
                        "Bandwidth(Utilization)": 0,
                        "Size Distribution": {}
                    }
                }
            }
        }
    }
    expected_data = {
        'op_name@group_name': {
            "Communication Time Info": {
                "Start Timestamp(us)": 0,
                "Elapse Time(ms)": 2,
                "Transit Time(ms)": 1,
                "Wait Time(ms)": 0.5,
                "Synchronization Time(ms)": 0.5,
                "Idle Time(ms)": 0
            },
            "Communication Bandwidth Info": {
                "RDMA": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Size Distribution": {}
                },
                "SDMA": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Size Distribution": {}
                },
                "HCCS": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Size Distribution": {}
                },
                "PCIE": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Size Distribution": {}
                },
                "SIO": {
                    "Transit Size(MB)": 0,
                    "Transit Time(ms)": 0,
                    "Bandwidth(GB/s)": 0,
                    "Size Distribution": {}
                }
            }
        }
    }

    def test_get_hccl_data_from_db_should_return_hccl_data_when_get_hccl_data_correct(self):
        events = [
            Event(StrConstant.RDMA_SEND, 0), Event(StrConstant.RDMA_SEND, 11), Event(StrConstant.NOTIFY_WAIT, 21),
            Event(StrConstant.RDMA_SEND, 31), Event(StrConstant.NOTIFY_WAIT, 41)
        ]
        expected_result = {
            'hcom_allReduce__1@group_name': {
                -1: OpTaskBundle(op_name='hcom_allReduce__1', tasks=events)
            }
        }
        InfoConfReader()._start_info = {'collectionTimeBegin': 0}
        InfoConfReader()._end_info = {}
        with mock.patch(NAMESPACE + ".CommunicationModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".CommunicationModel.get_all_events_from_db",
                           return_value={(1, 0): OpTaskData(
                               op_name='hcom_allReduce__1', group_name='group_name', tasks=events)}):
            analyzer = CommunicationAnalyzer(self.collection_path, 'text')
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))

            self.assertEqual(analyzer.rank_hccl_data_dict, expected_result)
        InfoConfReader()._start_info.clear()

    def test_process_dict_should_return_correct_dict_when_process_analyzed_data(self):
        analyzer = CommunicationAnalyzer(self.collection_path, 'text')
        processed_data = analyzer._process_dict(self.analyzed_data)
        self.assertEqual(processed_data, self.expected_data)

    def test_generate_parser_should_raise_exception_when_no_rank_hccl_data_dict(self):
        with mock.patch(NAMESPACE + ".CommunicationAnalyzer._get_hccl_data_from_db"),  \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationAnalyzer(self.collection_path, 'text')
            analyzer._generate_parser(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_hccl_data_from_db_should_raise_exception_when_no_hccl_data(self):
        InfoConfReader()._start_info = {'collectionTimeBegin': 0}
        InfoConfReader()._end_info = {}
        with mock.patch(NAMESPACE + ".CommunicationModel.check_db", return_value=True), \
                mock.patch(NAMESPACE + ".CommunicationModel.get_all_events_from_db", return_value=[]), \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationAnalyzer(self.collection_path, 'text')
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)
        InfoConfReader()._start_info.clear()

    def test_get_hccl_data_from_db_should_raise_exception_when_no_hccl_allReduce_table(self):
        InfoConfReader()._start_info = {'collectionTimeBegin': 0}
        InfoConfReader()._end_info = {}
        with mock.patch(NAMESPACE + ".CommunicationModel.check_table", return_value=False), \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationAnalyzer(self.collection_path, 'text')
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_CONNECT_ERROR, err.value.code)
        InfoConfReader()._start_info.clear()

    def test_dump_dict_to_db_should_flush_correct_data_to_db(self):
        with mock.patch("msmodel.cluster_info.communication_analyzer_model.CommunicationAnalyzerModel."
                        "flush_communication_data_to_db"):
            analyzer = CommunicationAnalyzer(self.collection_path, 'db')
            analyzer._dump_dict_to_db(self.expected_data)

    def test_generate_output_should_dump_dict_to_db_when_export_type_is_db(self):
        communication_parser = CommunicationParser(self.analyzed_data)
        with mock.patch("msparser.cluster.communication_parser.CommunicationParser.run"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzer._dump_dict_to_db"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzer._generate_parser", return_value=communication_parser):
            analyzer = CommunicationAnalyzer(self.collection_path, 'db')
            analyzer._generate_output(os.path.join(self.collection_path, "device_1"))

    def test_generate_output_should_dump_dict_to_db_when_export_type_is_text(self):
        communication_parser = CommunicationParser(self.analyzed_data)
        with mock.patch("msparser.cluster.communication_parser.CommunicationParser.run"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzer._dump_dict_to_json"), \
                mock.patch(NAMESPACE + ".CommunicationAnalyzer._generate_parser", return_value=communication_parser):
            analyzer = CommunicationAnalyzer(self.collection_path, 'text')
            analyzer._generate_output(os.path.join(self.collection_path, "device_1"))
