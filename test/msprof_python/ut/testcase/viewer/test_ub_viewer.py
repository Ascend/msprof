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
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ub_dto import UBDto
from viewer.ub_viewer import UBViewer

NAMESPACE = 'msmodel.hardware.ub_model'


class TestUBViewer(unittest.TestCase):
    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "50"}]}
        configs = {}
        param = {}
        datas = [
            UBDto(7, 0, 508694802700, 800, 1000, 780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3),
            UBDto(6, 0, 508965580266, 620, 1100, 560, 550, 999, 880, 8, 2, 888, 200, 1211, 85, 9, 1)
        ]
        with mock.patch(NAMESPACE + '.UBViewModel.check_table', return_value=True):
            check = UBViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(3, len(ret))
            self.assertEqual("UB Port000", ret[1]["name"])
            self.assertEqual("UB Port000", ret[2]["name"])

    def test_format_ub_summary_data_should_return_formatted_summary_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "50"}]}
        configs = {}
        param = {}
        datas = [
            UBDto(7, 0, 508694802700, 800, 1000, 780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3),
            UBDto(6, 0, 508965580266, 620, 1100, 560, 550, 999, 880, 8, 2, 888, 200, 1211, 85, 9, 1)
        ]
        with mock.patch(NAMESPACE + '.UBViewModel.check_table', return_value=True):
            check = UBViewer(configs, param)
            ret = check.format_ub_summary_data(datas)
            self.assertEqual(2, len(ret))
            self.assertEqual(4, len(ret[0]))
            self.assertEqual((0, ret[0][1], 800, 1000), ret[0])
