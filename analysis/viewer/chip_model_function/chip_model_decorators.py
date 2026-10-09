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
from functools import wraps
from common_func.constant import Constant
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

# 部分表头带单位，是因为sample-based和task-based的表头不一致
NOT_SUPPORT_PMU_FOR_CHIP_V1_1 = [
    "vec_ratio",
    "vec_time",
    "ub_read_bw",
    "ub_read_bw(GB/s)",
    "ub_write_bw",
    "ub_write_bw(GB/s)",
    "l0c_write_bw",
    "l0c_write_bw(GB/s)",
    "l2_write_bw",
    "l2_write_bw(GB/s)",
    "vec_fp32_ratio",
    "vec_fp16_ratio",
    "vec_int32_ratio",
    "vec_misc_ratio",
    "vec_bankgroup_cflt_ratio",
    "vec_bank_cflt_ratio",
    "vec_resc_cflt_ratio",
]

Chip_Model_Not_Support_PMU_Dict = {
    ChipModel.CHIP_V1_1_1: NOT_SUPPORT_PMU_FOR_CHIP_V1_1,
    ChipModel.CHIP_V1_1_2: NOT_SUPPORT_PMU_FOR_CHIP_V1_1,
    ChipModel.CHIP_V1_1_3: NOT_SUPPORT_PMU_FOR_CHIP_V1_1,
}


def get_disable_support_pmu_set() -> set:
    return set(Chip_Model_Not_Support_PMU_Dict.get(ChipManager().get_chip_id(), []))


class ChipModeDecorators:
    @staticmethod
    def pmu_format_for_chip_model(func):
        @wraps(func)
        def wrapper(headers: list, pmu_data: list) -> list:
            disable_support_pmu_list = get_disable_support_pmu_set() & set(headers)
            if not disable_support_pmu_list:
                return func(headers, pmu_data)
            for name in disable_support_pmu_list:
                idx = headers.index(name)
                for item in pmu_data:
                    item[idx] = Constant.NA
            return func(headers, pmu_data)

        return wrapper


@ChipModeDecorators.pmu_format_for_chip_model
def format_pmu_data_by_headers(headers: list, pmu_data: list) -> tuple:
    return headers, pmu_data
