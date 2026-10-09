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

import os
import shutil
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from msparser.hardware.ub_parser import ParsingUBData
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.ub_parser'


class TestParsingUBData(unittest.TestCase):
    file_list = {DataTag.UB: ['ub.data.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ub")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def setUpClass(cls):
        InfoConfReader()._info_json = {"DeviceInfo": [{"hwts_frequency": "50.000000"}]}
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_ub_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ub_data(cls):
        data = struct.pack("=4H15Q", 23130, 7, 0, 111, 508694802700, 800, 1000,
                           780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3)
        with FdOpen(os.path.join(cls.DATA_PATH, "ub.data.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data(self):
        check = ParsingUBData(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "ub.data.0.slice_0"))
        self.assertEqual(result, 0)

    def test_parse(self):
        check = ParsingUBData(self.file_list, self.CONFIG)
        check.parse()

    def test_save(self):
        check = ParsingUBData(self.file_list, self.CONFIG)
        check._data_list = [[7, 0, 508694802700, 800, 1000, 780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3]]
        check.save()

    def test_ms_run(self):
        ParsingUBData(self.file_list, self.CONFIG).ms_run()
