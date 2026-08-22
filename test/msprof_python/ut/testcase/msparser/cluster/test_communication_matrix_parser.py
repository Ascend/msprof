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

import unittest
from unittest import mock

import pytest

from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from msparser.cluster.communication_matrix_parser import CommunicationMatrixParser
from msparser.cluster.communication_matrix_parser import MatrixDataType
from msparser.cluster.meta_parser import OpTaskBundle

NAMESPACE = 'msparser.cluster.communication_matrix_parser'


class Event:
    def __init__(self, transport_type: str, hccl_name: str):
        self.local_rank = 0
        self.remote_rank = 1
        self.op_name = 'hcom_allReduce_1'
        self.hccl_name = hccl_name
        self.size = 1000 ** 2
        self.duration = 1000000
        self.transport_type = transport_type
        self.timestamp = 0
        self.bandwidth = 1
        self.rdma_type = 'INVALID_TYPE'
        self.link_type = StrConstant.HCCS
        self.plane_id = 0



class TestCommunicationMatrixParser(unittest.TestCase):
    link_info1 = {
        "0-1": {
            MatrixDataType.TRANSPORT_TYPE: 0,
            MatrixDataType.TRANS_TIME: 1,
            MatrixDataType.TRANS_SIZE: 1,
            MatrixDataType.PACKET_NUM: 10,
            MatrixDataType.LARGE_PACKET_NUM: 2
        },
        "0-7": {
            MatrixDataType.TRANSPORT_TYPE: 2,
            MatrixDataType.TRANS_TIME: 10,
            MatrixDataType.TRANS_SIZE: 10,
            MatrixDataType.PACKET_NUM: 100,
            MatrixDataType.LARGE_PACKET_NUM: 99
        }
    }
    link_info2 = {
        "0-1": {
            MatrixDataType.TRANSPORT_TYPE: 0,
            MatrixDataType.TRANS_TIME: 0.5,
            MatrixDataType.TRANS_SIZE: 0.5,
            MatrixDataType.PACKET_NUM: 10,
            MatrixDataType.LARGE_PACKET_NUM: 0
        },
        "0-7": {
            MatrixDataType.TRANSPORT_TYPE: 2,
            MatrixDataType.TRANS_TIME: 10.5,
            MatrixDataType.TRANS_SIZE: 10.5,
            MatrixDataType.PACKET_NUM: 105,
            MatrixDataType.LARGE_PACKET_NUM: 100
        },
        "0-4": {
            MatrixDataType.TRANSPORT_TYPE: 1,
            MatrixDataType.TRANS_TIME: 1,
            MatrixDataType.TRANS_SIZE: 1,
            MatrixDataType.PACKET_NUM: 1,
            MatrixDataType.LARGE_PACKET_NUM: 1
        }
    }
    hccl_dict1 = {StrConstant.OP_NAME: 'hccl_name1', StrConstant.LINK_INFO: link_info1}
    hccl_dict2 = {StrConstant.OP_NAME: 'hccl_name2', StrConstant.LINK_INFO: link_info2}

    def test_run(self):
        with mock.patch(NAMESPACE + '.CommunicationMatrixParser.parse'), \
                mock.patch(NAMESPACE + '.CommunicationMatrixParser.combine'), \
                mock.patch(NAMESPACE + '.CommunicationMatrixParser.convert'):
            CommunicationMatrixParser({}).run()

    def test_parse(self):
        with pytest.raises(ProfException) as err:
            CommunicationMatrixParser({}).parse()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_parse_sdma(self):
        events = [Event(StrConstant.SDMA, 'Memcpy')]
        parser = CommunicationMatrixParser({})
        parser.parse_ops(OpTaskBundle(tasks=events, op_name='hcom_allReduce_1'), 'op_name')
        self.assertEqual(parser.op_info[0][StrConstant.LINK_INFO]['0-1'][1], 1)

    def test_parse_rdma(self):
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            events = [Event(StrConstant.RDMA, StrConstant.RDMA_SEND), Event(StrConstant.RDMA, StrConstant.RDMA_SEND),
                      Event(StrConstant.RDMA, StrConstant.NOTIFY_WAIT), Event(StrConstant.RDMA, StrConstant.RDMA_SEND),
                      Event(StrConstant.RDMA, StrConstant.NOTIFY_WAIT)]
            events[0].rdma_type = 'RDMA_SEND_PAYLOAD'
            parser = CommunicationMatrixParser({})
            parser.parse_ops(OpTaskBundle(tasks=events, op_name='hcom_allReduce_1'), 'op_name')
            self.assertEqual(parser.op_info[0][StrConstant.LINK_INFO]['0-1'][2], 1)

    def test_combine(self):
        parser = CommunicationMatrixParser({})
        parser.op_info = [self.hccl_dict1, self.hccl_dict2]
        parser.combine()
        ret = parser.op_info[-1].get(StrConstant.LINK_INFO)
        self.assertEqual(ret['0-1'][MatrixDataType.TRANSPORT_TYPE], 0)
        self.assertEqual(ret['0-1'][MatrixDataType.TRANS_SIZE], 1.5)
        self.assertEqual(ret['0-7'][MatrixDataType.TRANS_TIME], 20.5)
        self.assertEqual(ret['0-7'][MatrixDataType.PACKET_NUM], 205)
        self.assertEqual(ret['0-4'][MatrixDataType.LARGE_PACKET_NUM], 1)

    def test_convert(self):
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            parser = CommunicationMatrixParser({})
            parser.op_info = [self.hccl_dict1, self.hccl_dict2]
            parser.convert()
            ret1 = parser.op_info[0].get(StrConstant.LINK_INFO)
            ret2 = parser.op_info[1].get(StrConstant.LINK_INFO)
            self.assertEqual(ret1[0][CommunicationMatrixInfo.SRC_RANK], '0')
            self.assertEqual(ret1[0][CommunicationMatrixInfo.DST_RANK], '1')
            self.assertEqual(ret1[1][CommunicationMatrixInfo.BANDWIDTH_UTILIZATION], 0.08)
            self.assertEqual(ret2[2][CommunicationMatrixInfo.BANDWIDTH_GB_S], 1.0)
            self.assertEqual(ret2[1][CommunicationMatrixInfo.LARGE_PACKET_RATIO], round(100 / 105, 4))
