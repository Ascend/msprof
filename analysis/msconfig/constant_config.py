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


class ConstantConfig(MetaConfig):
    DATA = {
        'events': [
            ('ai_ctrl_cpu_profiling_events', '0x11,0x8'),
            ('ts_cpu_profiling_events', '0x11,0x8'),
            ('hbm_profiling_events', 'read,write'),
            ('ddr_profiling_events', 'read,write'),
        ],
        'GENERAL': [('dangerous_app_list', 'rm,mv,reboot,shutdown,halt')],
    }
