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
import shutil
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from msparser.hardware.qos_parser import ParsingQosData
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.qos_parser'


class TestParsingQosData(unittest.TestCase):
    file_list = {DataTag.QOS: ['qos.data.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "qos")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setUpClass(cls):
        InfoConfReader()._info_json = {"DeviceInfo": [{"hwts_frequency": "50.000000"}]}
        InfoConfReader()._sample_json = {"qosEvents": "SDMA_QOS,DVPP_VENC_QOS"}
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_qos_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_qos_data(cls):
        data = struct.pack("=2I2Q10I", 66, 0, 0, 0, 66, 0, 0, 1, 66, 0, 0, 2, 66, 0)
        with FdOpen(os.path.join(cls.DATA_PATH, "qos.data.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data(self):
        check = ParsingQosData(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "qos.data.0.slice_0"))
        self.assertEqual(result, 0)

    def test_parse(self):
        check = ParsingQosData(self.file_list, self.CONFIG)
        check.parse()

    def test_save(self):
        check = ParsingQosData(self.file_list, self.CONFIG)
        check.qos_data = [[66, 0, 0, 0], [66, 0, 0, 1]]
        check.save()

    def test_ms_run(self):
        ParsingQosData(self.file_list, self.CONFIG).ms_run()
