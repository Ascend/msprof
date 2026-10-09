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

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.acc_pmu_viewer import AccPmuViewer


class AccPmuTestData:

    def __init__(self, *args):
        self.timestamp = args[0]
        self.read_bandwidth = args[1]
        self.acc_id = args[2]
        self.write_bandwidth = args[3]
        self.read_ost = args[4]
        self.write_ost = args[5]


class TestAccPmuViewer(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        InfoConfReader()._info_json = {"pid": '0'}

    def test_get_timeline_header(self):
        check = AccPmuViewer({}, {})
        ret = check.get_timeline_header()
        self.assertEqual([["process_name", 0, 0, "Acc PMU"]], ret)

    def test_get_trace_timeline(self):
        check = AccPmuViewer({}, {})
        ret = check.get_trace_timeline([])
        self.assertEqual([], ret)

        datas = [AccPmuTestData(1, 2, 3, 4, 5, 6, 7, 8, 9, 123),
                 AccPmuTestData(2, 2, 3, 4, 5, 6, 7, 8, 9, 123),
                 AccPmuTestData(2, 3, 4, 5, 6, 7, 8, 9, 10, 123)]
        check = AccPmuViewer({}, {})
        ret = check.get_trace_timeline(datas)
        self.assertEqual(9, len(ret))
