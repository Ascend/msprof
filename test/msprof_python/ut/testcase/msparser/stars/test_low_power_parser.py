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
import struct
import unittest

from msparser.stars.low_power_parser import LowPowerParser
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'msparser.stars.low_power_parser'


class TestInterSocParser(unittest.TestCase):

    def test_preprocess_data(self):
        data = struct.pack("=HHLQ12LHHLQ12LHHLQ12L", 29, 27603, 0, 100, 838631199, 1139847156, 1204041200, 1208639554,
                           1275068416, 1342177280, 1409286144, 1476395008, 2147483648,
                           2214592512, 2281701376, 2348810240, 29, 27603, 0, 100, 2415919104, 3255989866, 3288334336,
                           3355443200, 3422552064, 3489660928,
                           3556769792, 3623878656, 3690987520, 3758096384, 3825205248, 3892314112, 29, 27603, 0, 100,
                           3959422976, 4026531840, 4093640704, 4227858433, 0, 0, 0, 0, 0, 0, 0, 0)
        check = LowPowerParser('test', 'lowpower.db', ['LowPower'])
        InfoJson(pid=1, DeviceInfo=[DeviceInfo(hwts_frequency=1150)]).apply()
        check.handle('', data[0:64])
        check.handle('', data[64:128])
        check.handle('', data[128:])
        check.preprocess_data()
