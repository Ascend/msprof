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

from common_func.platform.chip_manager import ChipManager
from msmodel.stars.low_power_model import LowPowerModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.stars.low_power_bean import LowPowerBean


class LowPowerParser(IStarsParser):
    """
    stars lower power sample data parser
    now only support chip 14
    """

    LOW_POWER_DATA0_SOFT_INDEX = 10
    LOW_POWER_HIGH_16_SHIFT = 16
    LOW_POWER_DATA0_SOFT_SCALE = 100

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = LowPowerModel(result_dir, db, table_list)
        self._decoder = LowPowerBean
        self._data_list = []

    @classmethod
    def _adjust_lp_sample_info(cls: any, lp_sample_info: list) -> list:
        """
        Convert V6.2.0 data0_soft from high/low 16-bit samples to a scaled 32-bit value.
        """
        result = list(lp_sample_info)
        if ChipManager().chip_id != ChipModel.CHIP_V6_2_0:
            return result
        if len(result) <= cls.LOW_POWER_DATA0_SOFT_INDEX + 1:
            return result

        high_16_value = result[cls.LOW_POWER_DATA0_SOFT_INDEX]
        low_16_value = result[cls.LOW_POWER_DATA0_SOFT_INDEX + 1]
        combined_value = (high_16_value << cls.LOW_POWER_HIGH_16_SHIFT) | low_16_value
        result[cls.LOW_POWER_DATA0_SOFT_INDEX] = combined_value / cls.LOW_POWER_DATA0_SOFT_SCALE
        return result

    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
        self._data_list = [
            [data.timestamp, data.die_id] + self._adjust_lp_sample_info(data.lp_sample_info) for data in self._data_list
        ]
