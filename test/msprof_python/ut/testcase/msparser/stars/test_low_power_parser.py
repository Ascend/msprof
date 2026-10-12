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
import struct
import unittest

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from msparser.stars.low_power_parser import LowPowerParser
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'msparser.stars.low_power_parser'


class TestInterSocParser(unittest.TestCase):

    def setUp(self):
        self._chip_id = ChipManager().chip_id

    def tearDown(self):
        ChipManager().chip_id = self._chip_id

    def test_preprocess_data(self):
        data = struct.pack("=HHLQQ20H", 29, 27603, 0, 100, 200, *range(20))
        check = LowPowerParser('test', 'lowpower.db', ['LowPower'])
        InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1150)]).apply()
        check.handle('', data)
        check.preprocess_data()

        self.assertEqual(len(check._data_list), 1)
        self.assertEqual(check._data_list[0][2:], [1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14, 17, 16, 19, 18])

    def test_preprocess_data_should_combine_data0_soft_on_chip_v620(self):
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        data = struct.pack("=HHLQQ20H", 29, 27603, 0, 100, 200, *range(20))
        check = LowPowerParser('test', 'lowpower.db', ['LowPower'])
        InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1150)]).apply()
        check.handle('', data)
        check.preprocess_data()

        data0_soft_column_index = 12
        expected_data0_soft = ((11 << 16) | 10) / 100
        self.assertAlmostEqual(check._data_list[0][data0_soft_column_index], expected_data0_soft)
        self.assertEqual(check._data_list[0][data0_soft_column_index + 1], 10)
