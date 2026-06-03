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


class StarsConstant:
    """
    class used to record constant about stars
    """

    ACSQ_START_FUNCTYPE = '000000'
    ACSQ_END_FUNCTYPE = '000001'
    FUSION_TASK_START_FUNCTYPE = '010110'
    FUSION_TASK_END_FUNCTYPE = '010111'
    FFTS_LOG_START_TAG = '100010'
    FFTS_LOG_END_TAG = '100011'
    FFTS_PMU_TAG = '101000'
    FFTS_BLOCK_PMU_TAG = '101001'
    BLOCK_LOG_START_FUNCTYPE = '100100'
    BLOCK_LOG_END_FUNCTYPE = '100101'
    PMU_TAG_V6 = '101010'

    # chip trans type
    TYPE_STAR_CHIP_TRANS = "011011"
    TYPE_STARS_PA = "111111"
    TYPE_STARS_PCIE = "100010"

    FFTS_TYPE = {0: "AIC only", 1: "AIV only", 2: "Automatic Threading Mode", 3: "Manual Threading Mode", 4: "FFTS+"}
    SUBTASK_TYPE = {
        0: "AIC",
        1: "AIV",
        3: "Notify Wait",
        4: "Notify Record",
        5: "Write Value",
        6: "MIX_AIC",
        7: "MIX_AIV",
        8: "SDMA",
        9: "Data Context",
        # Schedule the DMU descriptor to SDMA, and SDMA informs the L2 cache to
        #   invalidate the corresponding cache line directly
        10: "Invalidate Data Context",
        # FFTS schedules the DMU descriptor to SDMA,
        # and SDMA informs L2 cache to write back the corresponding cache line
        11: "Writeback Data Context",
        12: "AI_CPU",
        13: "Load Context",
        15: "DSA",
    }

    def get_subtask_type(self: any) -> dict:
        """
        get subtask type
        :return: dict
        """
        return self.SUBTASK_TYPE

    def get_ffts_type(self: any) -> dict:
        """
        get ffts type
        :return: dict
        """
        return self.FFTS_TYPE

    def find_key_by_value(self: any, value: str) -> int:
        for key, val in self.SUBTASK_TYPE.items():
            if val == value:
                return key
        return max(self.SUBTASK_TYPE) + 1
