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

from enum import Enum
from enum import unique
from common_func.platform.chip_manager import ChipManager


class SqeType:
    @unique
    class StarsSqeType(Enum):
        """
        Chip's sqetype whose task schedule is stars type
        """

        AI_CORE = 0
        AI_CPU = 1
        AIV_SQE = 2
        PLACE_HOLDER_SQE = 3
        EVENT_RECORD_SQE = 4
        EVENT_WAIT_SQE = 5
        NOTIFY_RECORD_SQE = 6
        NOTIFY_WAIT_SQE = 7
        WRITE_VALUE_SQE = 8
        VQ6_SQE = 9
        TOF_SQE = 10
        SDMA_SQE = 11
        VPC_SQE = 12
        JPEGE_SQE = 13
        JPEGD_SQE = 14
        DSA_SQE = 15
        ROCCE_SQE = 16
        PCIE_DMA_SQE = 17
        HOST_CPU_SQE = 18
        CDQM_SQE = 19
        C_CORE_SQE = 20

    @unique
    class HwSqeType(Enum):
        """
        Chip's sqetype whose task schedule is hwts type
        """

        AI_CORE = 0
        AI_CPU = 1
        AIV_SQE = 2
        PLACE_HOLDER_SQE = 3
        EVENT_RECORD_SQE = 4
        EVENT_WAIT_SQE = 5
        NOTIFY_RECORD_SQE = 6
        NOTIFY_WAIT_SQE = 7
        WRITE_VALUE_SQE = 8
        SDMA_SQE = 9
        MAX_SQE = 10

    @unique
    class ChipV6SqeType(Enum):
        """
        Chip's sqetype whose task schedule is ChipV6 type
        """

        AI_CORE = 0
        AIV_SQE = 1
        FUSION = 2
        PLACE_HOLDER_SQE = 3
        AICPU_H = 4
        AICPU_D = 5
        NOTIFY_RECORD_SQE = 6
        NOTIFY_WAIT_SQE = 7
        WRITE_VALUE_SQE = 8
        UBDMA = 9
        ASYNCDMA = 10
        SDMA_SQE = 11
        VPC_SQE = 12
        JPEGE_SQE = 13
        JPEGD_SQE = 14
        CMO_SQE = 15
        CCU_SQE = 16
        CONDITION_SQE = 20
        END_SQE = 21

    def __init__(self):
        if ChipManager().is_chip_v6():
            self.instance = SqeType.ChipV6SqeType
        elif ChipManager().is_stars_chip():
            self.instance = SqeType.StarsSqeType
        else:
            self.instance = SqeType.HwSqeType
