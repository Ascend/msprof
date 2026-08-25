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
from collections import defaultdict
from unittest import mock

import pytest

from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from mscalculate.hccl.hccl_task import HcclTask
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.meta_parser import HcclAnalysisTool
from msparser.cluster.meta_parser import OpTaskBundle

NAMESPACE = 'msparser.cluster.communication_parser'


class Event:
    def __init__(self, hccl_name: str, *args):
        self.hccl_name = hccl_name
        self.op_name = 'hcom_allReduce_1'
        self.size = 1000 ** 2
        self.duration = 1000000
        self.transport_type = StrConstant.RDMA
        self.timestamp = 0
        self.bandwidth = 1
        self.rdma_type = args


class TestHcclAnalysisTool(unittest.TestCase):
    def test_get_value(self):
        ret = HcclAnalysisTool.get_value(1, 'none')
        self.assertEqual(ret, 1)

    def test_get_value_str(self):
        ret = HcclAnalysisTool.get_value('1', 'none')
        self.assertEqual(ret, 0)

    def test_get_value_none(self):
        ret = HcclAnalysisTool.get_value(None, 'none')
        self.assertEqual(ret, 0)

    def test_determine_rdma1(self):
        events = [Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
                  Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)]
        ret = HcclAnalysisTool.determine_rdma(events, 0, 5)
        self.assertEqual(ret, True)

    def test_determine_rdma2(self):
        events = [Event(StrConstant.RDMA_SEND)]
        ret = HcclAnalysisTool.determine_rdma(events, 0, 5)
        self.assertEqual(ret, False)

    def test_get_rdma_time_info(self):
        events = [Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
                  Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)]
        ret = HcclAnalysisTool.get_rdma_time_info(events, 0, 5)
        self.assertEqual(ret, [1, 1000 ** 2 / 1024 ** 2])

    def test_find_consecutive_payload_tasks_count(self):
        RDMA = 'RDMA'
        OP_NAME = 'hcom_allReduce__721_0_1'
        RDMASend = 'RDMASend'
        RDMA_SEND_PAYLOAD = 'RDMA_SEND_PAYLOAD'

        events = [
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                         size=104857600,
                         bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                         size=104857600,
                         bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                         size=104857600,
                         bandwidth=-1),
        ]
        idx = 0
        payload_cnt = HcclAnalysisTool.find_consecutive_payload_tasks_count(events, idx)
        ans = 3
        self.assertAlmostEqual(payload_cnt, ans)


class TestCommunicationParser(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.CommunicationParser.parse'), \
                mock.patch(NAMESPACE + '.CommunicationParser.combine'):
            CommunicationParser({}).run()

    def test_parse(self):
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).parse()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_parse_ops(self):
        with mock.patch(NAMESPACE + '.CommunicationParser.op_bandwidth_parser'):
            hccl_data_ffts = [
                HcclTask(plane_id=0, hccl_name="0", duration=0, timestamp=0,
                         group_name="1", is_master=1)
            ]
            CommunicationParser({}).parse_ops(
                {-1: OpTaskBundle(tasks=hccl_data_ffts, op_name='hcom_broadcast__752_0_1')},
                'hcom_broadcast__752_0_1@9577302986659220752'
            )
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).parse_ops({1: OpTaskBundle()}, 'name')
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_op_time_parser(self):
        LOCAL = 'LOCAL'
        ROCE = 'ROCE'
        Notify_Wait = 'Notify_Wait'
        RDMASEND = 'RDMASend'
        OP_NAME = 'hcom_allReduce__721_0_1'
        RDMA_SEND_PAYLOAD = 'RDMA_SEND_PAYLOAD'
        RDMA = 'RDMA'
        # test ProfException when master_events is empty
        err_hccl_data_ffts = [
            HcclTask(plane_id=0, hccl_name="0", duration=0, timestamp=0,
                     group_name="1"),
            HcclTask(plane_id=2, hccl_name="0", duration=0, timestamp=0,
                     group_name="1")
        ]
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).op_time_parser(err_hccl_data_ffts, OP_NAME, 0)
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)
        # test whether Idle Time(ms) = Elapse Time(ms) - Transit Time(ms) - Wait Time(ms)
        # when all event's 'is_master' is 1
        events = [
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='Memcpy', rdma_type='INVALID_TYPE',
                     timestamp=63888072593921.055, duration=319959.1875, transport_type='SDMA', task_id=1,
                     size=209715200, bandwidth=0.65544359466158, link_type='ON_CHIP', is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888072915640.34, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE, is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_PREPARE',
                     timestamp=63888072917700.47, duration=20, transport_type=LOCAL, task_id=1, size=0, bandwidth=0,
                     link_type='INVALID_TYPE', is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_PAYLOAD',
                     timestamp=63888072919960.61, duration=320.015625, transport_type='RDMA', task_id=1, size=104857600,
                     bandwidth=24.28991694888519, link_type=ROCE, is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888072921720.71, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE, is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_CHECK',
                     timestamp=63888072923480.82, duration=4310758.46875, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=0, link_type='INVALID_TYPE', is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888077234799.32, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE, is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_ACK',
                     timestamp=63888077236859.445, duration=20, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=-1, link_type='INVALID_TYPE', is_master=1),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='Memcpy', rdma_type='INVALID_TYPE',
                     timestamp=63888077238999.58, duration=160429.6171875, transport_type='SDMA', task_id=1,
                     size=104857600, bandwidth=0.65360500036255, link_type='ON_CHIP', is_master=1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1, is_master=1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1, is_master=1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1, is_master=1),
        ]

        op_start = min(event.timestamp for event in events)
        op_end = max(event.timestamp + event.duration for event in events)
        op_time_dict = CommunicationParser({}).op_time_parser(events, OP_NAME, op_end - op_start)
        self.assertAlmostEqual(op_time_dict["Transit Time(ms)"], 4.3169188359375)
        self.assertAlmostEqual(op_time_dict["Wait Time(ms)"], 0.00002)
        self.assertAlmostEqual(op_time_dict["Idle Time(ms)"], 0.48856930468750026)
        self.assertAlmostEqual(op_time_dict["Synchronization Time(ms)"], 0.00002)
        self.assertAlmostEqual(op_time_dict["Transit Time(ms)"] + op_time_dict["Wait Time(ms)"] +
                               op_time_dict["Idle Time(ms)"], op_time_dict['Elapse Time(ms)'])

    def test_op_bandwidth_parser(self):
        RDMA = 'RDMA'
        SDMA = 'SDMA'
        LOCAL = 'LOCAL'
        ROCE = 'ROCE'
        Notify_Wait = 'Notify_Wait'
        RDMASEND = 'RDMASend'
        OP_NAME = 'hcom_allReduce__721_0_1'
        RDMA_SEND_NOTIFY = 'RDMA_SEND_NOTIFY'
        RDMA_SEND_PAYLOAD = 'RDMA_SEND_PAYLOAD'
        Bandwidth_GB_S = 'Bandwidth(GB/s)'
        Bandwidth_Utilization = 'Bandwidth(Utilization)'
        Large_Packet_Ratio = 'Large Packet Ratio'
        Size_Distribution = 'Size Distribution'
        Transit_Size_MB = 'Transit Size(MB)'
        Transit_Time_ms = 'Transit Time(ms)'
        INVALID_TYPE = 'INVALID_TYPE'
        RESERVED = "RESERVED"
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        expect_bandwidth_dict = {
            'HCCS': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'PCIE': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'SIO': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'RDMA': {
                Bandwidth_GB_S: 22.6217,
                Bandwidth_Utilization: 1.8097,
                Large_Packet_Ratio: 1.0,
                Size_Distribution: {100.0: [1, 4.3169188359375]},
                Transit_Size_MB: 100.0,
                Transit_Time_ms: 4.3169188359375
            },
            SDMA: {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
        }

        events = [
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='Memcpy', rdma_type=INVALID_TYPE,
                     timestamp=63888072593921.055, duration=319959.1875, transport_type='SDMA', task_id=1,
                     size=209715200, bandwidth=0.65544359466158, link_type='ON_CHIP'),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888072915640.34, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_PREPARE',
                     timestamp=63888072917700.47, duration=20, transport_type=LOCAL, task_id=1, size=0, bandwidth=0,
                     link_type=INVALID_TYPE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_PAYLOAD',
                     timestamp=63888072919960.61, duration=320.015625, transport_type='RDMA', task_id=1, size=104857600,
                     bandwidth=24.28991694888519, link_type=ROCE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888072921720.71, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_CHECK',
                     timestamp=63888072923480.82, duration=4310758.46875, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=0, link_type=INVALID_TYPE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='RDMASend', rdma_type='RDMA_SEND_NOTIFY',
                     timestamp=63888077234799.32, duration=320.0234375, transport_type='RDMA', task_id=1, size=4,
                     bandwidth=0.00001249908454, link_type=ROCE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name=Notify_Wait, rdma_type='RDMA_PAYLOAD_ACK',
                     timestamp=63888077236859.445, duration=20, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=-1, link_type=INVALID_TYPE),
            HcclTask(op_name='hcom_allReduce__721_0_1', hccl_name='Memcpy', rdma_type=INVALID_TYPE,
                     timestamp=63888077238999.58, duration=160429.6171875, transport_type='SDMA', task_id=1,
                     size=104857600, bandwidth=0.65360500036255, link_type='ON_CHIP'),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASEND, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
        ]
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            op_bandwidth_dict = CommunicationParser({}).op_bandwidth_parser(events, OP_NAME)
            self.assertEqual(op_bandwidth_dict, expect_bandwidth_dict)
        mc2_events = [
            HcclTask(op_name='AllGatherMatmulMc2AicpuKernel_491_0_1', hccl_name='Memcpy', rdma_type=RESERVED,
                     timestamp=1832380406858.8928, duration=9680.193359375, transport_type=SDMA, task_id=1,
                     size=209715200, bandwidth=324.9654095962298, link_type=RESERVED)
        ]
        mc2_expect_bandwidth_dict = {
            'HCCS': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'PCIE': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'SIO': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            'RDMA': {
                Bandwidth_GB_S: 0,
                Bandwidth_Utilization: 0.0,
                Large_Packet_Ratio: 0,
                Size_Distribution: defaultdict(lambda: [0, 0]),
                Transit_Size_MB: 0,
                Transit_Time_ms: 0
            },
            SDMA: {
                Bandwidth_GB_S: 20176.5081,
                Bandwidth_Utilization: 0,
                Large_Packet_Ratio: 0,
                Size_Distribution: {200.0: [1, 0.009680193359375]},
                Transit_Size_MB: 200.0,
                Transit_Time_ms: 0.009680193359375
            },
        }
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            op_bandwidth_dict = CommunicationParser({}).op_bandwidth_parser(
                mc2_events, 'AllGatherMatmulMc2AicpuKernel_491_0_1'
            )
            self.assertEqual(op_bandwidth_dict, mc2_expect_bandwidth_dict)

    def test_combine_time(self):
        com = CommunicationParser({})
        com.op_info = {
            "allReduce_1_1": {
                0: {
                    "Communication Time Info": {
                        "Elapse Time(ms)": 2,
                        "Transit Time(ms)": 1,
                        "Wait Time(ms)": 1,
                        "Synchronization Time(ms)": 1,
                        "Wait Time Ratio": 0.5,
                        "Synchronization Time Ratio": 0.5
                    }
                }
            },
            "allReduce_1_2": {
                0: {
                    "Communication Time Info": {
                        "Elapse Time(ms)": 2,
                        "Transit Time(ms)": 1,
                        "Wait Time(ms)": 1,
                        "Synchronization Time(ms)": 1,
                        "Wait Time Ratio": 0.5,
                        "Synchronization Time Ratio": 0.5
                    }
                }
            }
        }
        com.combine()
        ret = com.op_info
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Elapse Time(ms)"], 4)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Transit Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Wait Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Synchronization Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Wait Time Ratio"], 0.5)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Synchronization Time Ratio"], 0.5)

    def test_combine_bandwidth(self):
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            com = CommunicationParser({})
            com.op_info = {
                "allReduce_1_1": {
                    0: {
                        "Communication Bandwidth Info": {
                            "RDMA": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "HCCS": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "SDMA": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0,
                                "Large Packet Ratio": 0,
                                "Size Distribution": {}
                            }
                        }
                    }
                },
                "allReduce_1_2": {
                    0: {
                        "Communication Bandwidth Info": {
                            "RDMA": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "HCCS": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "SDMA": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0,
                                "Large Packet Ratio": 0,
                                "Size Distribution": {}
                            }
                        }
                    }
                },
            }
            com.combine()
            ret = com.op_info
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Size(MB)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Time(ms)"], 2000
            )
            self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(GB/s)"], 0.9766)
            self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Large Packet Ratio"], 1)
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(Utilization)"],
                0.0781
            )
            self.assertEqual(ret[StrConstant.TOTAL][0]
                             ["Communication Bandwidth Info"]["RDMA"]["Size Distribution"][20], [4, 4])
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Size(MB)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Time(ms)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Bandwidth(GB/s)"], 0.9766
            )
