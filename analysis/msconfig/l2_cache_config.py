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

from common_func.constant import Constant
from msconfig.meta_config import MetaConfig


class L2CacheConfig(MetaConfig):
    DATA = {
        Constant.CHIP_V2_1_0: [('request_events', '0x59'), ('hit_events', '0x5b'), ('victim_events', '0x5c')],
        Constant.CHIP_V3_1_0: [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        Constant.CHIP_V3_2_0: [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        Constant.CHIP_V3_3_0: [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        Constant.CHIP_V4_1_0: [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        Constant.CHIP_V1_1_1: [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        Constant.CHIP_V1_1_2: [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        Constant.CHIP_V1_1_3: [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        Constant.CHIP_V6_1_0: [
            ('request_events', '0x00'),
            ('hit_events', '0x00,-0x81,-0x82,-0x83,-0x74,-0x75'),  # 寄存器带负号表示在计算中做减法
            ('victim_events', '0x74,0x75'),
        ],
        Constant.CHIP_V6_2_0: [
            ('request_events', '0x00'),
            ('hit_events', '0x00,-0x81,-0x82,-0x83,-0x74,-0x75'),  # 寄存器带负号表示在计算中做减法
            ('victim_events', '0x74,0x75'),
        ],
        Constant.CHIP_V6_1_1: [
            ('request_events', '0x00'),
            ('hit_events', '0x00,-0x81,-0x82,-0x83,-0x74,-0x75'),  # 寄存器带负号表示在计算中做减法
            ('victim_events', '0x74,0x75'),
        ],
    }
