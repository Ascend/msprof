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

from common_func.info_conf_reader import InfoConfReader
from viewer.qos_viewer import QosViewer
from viewer.qos_viewer import StarsQosViewer

NAMESPACE = 'msmodel.hardware.qos_model'


class TestQosViewer(unittest.TestCase):
    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1}
        InfoConfReader()._sample_json = {"qosEvents": "SDMA_QOS,DVPP_VENC_QOS"}
        configs = {"table": "QosOriginalData"}
        param = {"data_type": "qos"}
        datas = [
            [1713923176576148000.0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0],
            [1713923176576348000.0, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0]
        ]
        with mock.patch(NAMESPACE + '.QosViewModel.check_table', return_value=True):
            check = QosViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(5, len(ret))


class TestStarsQosViewer(unittest.TestCase):
    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1}
        InfoConfReader()._sample_json = {"qosEvents": "SDMA_QOS,DVPP_VENC_QOS"}
        configs = {"table": "QosOriginalData"}
        param = {"data_type": "qos"}
        datas = [
            [1713923176576148000.0, 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0],
            [1713923176576348000.0, 1, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0]
        ]
        with mock.patch(NAMESPACE + '.QosViewModel.check_table', return_value=True):
            check = StarsQosViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(5, len(ret))
