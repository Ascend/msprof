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

from common_func.db_name_constant import DBNameConstant
from msconfig.meta_config import MetaConfig


class StarsConfig(MetaConfig):
    DATA = {
        'AcsqTaskParser': [
            ('fmt', '000000, 000001'),
            ('db', DBNameConstant.DB_SOC_LOG),
            ('table', DBNameConstant.TABLE_ACSQ_TASK),
        ],
        'FftsLogParser': [
            ('fmt', '100010, 100011'),
            ('db', DBNameConstant.DB_SOC_LOG),
            ('table', DBNameConstant.TABLE_FFTS_LOG),
        ],
        'SioParser': [('fmt', '011001'), ('db', DBNameConstant.DB_SIO), ('table', DBNameConstant.TABLE_SIO)],
        'AccPmuParser': [
            ('fmt', '011010'),
            ('db', DBNameConstant.DB_ACC_PMU),
            ('table', DBNameConstant.TABLE_ACC_PMU_DATA),
        ],
        'InterSocParser': [
            ('fmt', '011100'),
            ('db', DBNameConstant.DB_STARS_SOC),
            ('table', DBNameConstant.TABLE_SOC_DATA),
        ],
        'StarsChipTransParser': [
            ('fmt', '011011'),
            ('db', DBNameConstant.DB_STARS_CHIP_TRANS),
            ('table', DBNameConstant.TABLE_STARS_PA_LINK + ',' + DBNameConstant.TABLE_STARS_PCIE),
        ],
        'LowPowerParser': [
            ('fmt', '011101'),
            ('db', DBNameConstant.DB_LOW_POWER),
            ('table', DBNameConstant.TABLE_LOWPOWER),
        ],
        'BlockLogParser': [
            ('fmt', '100100,100101'),
            ('db', DBNameConstant.DB_SOC_LOG),
            ('table', DBNameConstant.TABLE_BLOCK_LOG),
        ],
        'StarsQosParser': [('fmt', '011000'), ('db', DBNameConstant.DB_QOS), ('table', DBNameConstant.TABLE_QOS_BW)],
        'FusionTaskParser': [
            ('fmt', '010111,010110'),
            ('db', DBNameConstant.DB_FUSION_TASK),
            ('table', DBNameConstant.TABLE_FUSION_TASK),
        ],
    }


class FuncTypeConfig(MetaConfig):
    KNOWN_FUNC_TYPE_LIST = [
        # task based func type
        '000000',  # ACSQ
        '000001',
        '000010',  # Topic blk
        '000011',
        '000100',
        '000101',
        '000110',
        '001000',
        '001001',
        '001010',
        '001011',
        '001100',
        '001101',
        '001110',
        '001111',
        '010000',
        '010001',
        '010010',
        '010011',
        '010100',
        '010101',
        '010110',
        '010111',
        '100000',
        '100001',
        '100010',
        '100011',
        '100100',
        '100101',
        '100110',
        '100111',
        '101000',
        '101001',
        '101010',
        # sample based func type
        '011010',
        '011011',
        '011001',
        '011100',
        '011000',
        '011101',
        '011110',
        '011111',
    ]

    KNOWN_FUNC_TYPE_LIST_V6 = [
        # task based func type
        '000000',  # ACSQ
        '000001',
        '000010',  # Topic blk
        '000011',
        '001000',
        '001001',
        '001010',
        '001011',
        '001100',
        '001101',
        '001110',
        '001111',
        '010010',
        '010011',
        '010110',
        '010111',
        '100100',
        '100101',
        '101000',
        '101001',
        '101010',
        # sample based func type
        '011010',
        '011001',
        '011101',
        '011111',
    ]
