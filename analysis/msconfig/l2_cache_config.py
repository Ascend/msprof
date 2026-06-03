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

from msconfig.meta_config import MetaConfig


class L2CacheConfig(MetaConfig):
    DATA = {
        '1': [('request_events', '0x59'), ('hit_events', '0x5b'), ('victim_events', '0x5c')],
        '2': [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        '3': [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        '4': [('request_events', '0x78,0x79'), ('hit_events', '0x6a'), ('victim_events', '0x71')],
        '5': [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        '7': [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        '8': [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        '11': [('request_events', '0xfb,0xfc'), ('hit_events', '0x90,0x91'), ('victim_events', '0x9c')],
        '15': [
            ('request_events', '0x00'),
            ('hit_events', '0x00,-0x81,-0x82,-0x83,-0x74,-0x75'),  # 寄存器带负号表示在计算中做减法
            ('victim_events', '0x74,0x75'),
        ],
        '16': [
            ('request_events', '0x00'),
            ('hit_events', '0x00,-0x81,-0x82,-0x83,-0x74,-0x75'),  # 寄存器带负号表示在计算中做减法
            ('victim_events', '0x74,0x75'),
        ],
    }
