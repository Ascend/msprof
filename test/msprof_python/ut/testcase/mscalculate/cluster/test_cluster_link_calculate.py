#!/usr/bin/env python
# coding=utf-8
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from mscalculate.cluster.cluster_link_calculate import ClusterLinkCalculator
from mscalculate.cluster.cluster_link_calculate import ClusterSingleLinkCalculator
from mscalculate.hccl.hccl_task import HcclTask

NAMESPACE = 'mscalculate.cluster.cluster_link_calculate'


class TestClusterLinkCalculate(unittest.TestCase):

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.ms_run',
                        return_value={"SDMA": [['30', '0', '1', 30]]}):
            ret = ClusterLinkCalculator(["test"]).run()
            self.assertEqual(1, len(ret))


def construct_hccl_dto():
    data = [1, 2, 3, 4,
            "{'notify id': 4294967840, 'duration estimated': 0.8800048828125, 'stage': 4294967295, "
            "'step': 4294967385, 'bandwidth': 'NULL', 'stream id': 8, 'task id': 34, 'task type': 'Notify Record', "
            "'src rank': 2, 'dst rank': 1, 'transport type': 'SDMA', 'size': None, 'tag': 'all2allvc_1_5'}"]
    col = ["hccl_name", "plane_id", "timestamp", "duration", "args"]
    hccl_dto = HcclTask()
    for index, i in enumerate(data):
        if hasattr(hccl_dto, col[index]):
            setattr(hccl_dto, col[index], i)
    return hccl_dto


class TestClusterSingleLinkCalculator(unittest.TestCase):
    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ClusterSingleLinkCalculator.calculate'):
            ret = ClusterSingleLinkCalculator("test").ms_run()
            self.assertEqual(0, len(ret))

    def test_calculate(self):
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.CommunicationModel.get_all_communication_data',
                           return_value=[construct_hccl_dto(), construct_hccl_dto()]):
            ClusterSingleLinkCalculator("test").calculate()

        with mock.patch('os.path.exists', return_value=False):
            ClusterSingleLinkCalculator("test").calculate()
