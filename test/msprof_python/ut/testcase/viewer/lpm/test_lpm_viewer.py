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

# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025. All rights reserved.
"""

import unittest
from collections import OrderedDict

from viewer.lpm.lpm_viewer import LpmConvInfoViewer


class TestLpmConvInfoViewer(unittest.TestCase):
    """
    test lpm conv info viewer
    """

    def test_lpm_conv_info_viewer_split_and_fill_data_will_return_correct_filled_data(self):
        lpm_conv_data = [['Aicore Voltage', '0.200', 1, 0, OrderedDict([('mV', 850)])],
                         ['Aicore Voltage', '2000.000', 1, 0, OrderedDict([('mV', 600)])],
                         ['Aicore Voltage', '3000000.000', 1, 0, OrderedDict([('mV', 950)])],
                         ['Aicore Voltage', '3000001.000', 1, 0, OrderedDict([('mV', 950)])]]
        filled_data = LpmConvInfoViewer._split_and_fill_data(lpm_conv_data)
        self.assertEqual(len(filled_data), 3002)
        self.assertEqual(filled_data[0][0], "Aicore Voltage")
        self.assertEqual(filled_data[0][1], "0.200")
        self.assertEqual(filled_data[0][2], 1)
        self.assertEqual(filled_data[0][3], 0)
        self.assertEqual(filled_data[0][4], OrderedDict([('mV', 850)]))
        # filled data
        self.assertEqual(filled_data[1][0], "Aicore Voltage")
        self.assertEqual(filled_data[1][1], "1000.2")
        self.assertEqual(filled_data[1][2], 1)
        self.assertEqual(filled_data[1][3], 0)
        self.assertEqual(filled_data[1][4], OrderedDict([('mV', 850)]))
