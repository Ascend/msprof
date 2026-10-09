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

# pylint: disable=duplicate-code

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class PmuBeanV6(StructDecoder):
    """
    class used to decode pmu data for chip v6
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self._stream_id = filed[2]
        self._task_id = StarsCommon.set_unique_task_id(filed[2], filed[3])
        self._total_cycle = filed[4]
        self._ctx_type = filed[5]
        self._mix = int(bin(filed[6] & 2)[2:3])
        self._mst = int(bin(filed[6] & 1)[2:3])
        self._ov_flag = bin(filed[6] & 4)[2:3]
        self._core_type = filed[8] & 1
        self._core_id = filed[9]
        self._sub_block_id = filed[11]
        self._block_id = filed[12]
        self._pmu_list = filed[14:24]
        self._start_time = filed[24]
        self._end_time = filed[25]

    @property
    def stream_id(self: any) -> int:
        """
        get stream_id
        :return: stream_id
        """
        return self._stream_id

    @stream_id.setter
    def stream_id(self: any, stream_id: int):
        """
        set stream id
        """
        self._stream_id = stream_id

    @property
    def task_id(self: any) -> int:
        """
        get task_id
        :return: task_id
        """
        return self._task_id

    @property
    def core_type(self: any) -> int:
        """
        get core_type
        :return: core_type
        """
        return self._core_type

    @property
    def core_id(self: any) -> int:
        """
        get core_id
        :return: core_id
        """
        return self._core_id

    @property
    def sub_block_id(self: any) -> int:
        """
        get sub_block_id
        :return: sub_block_id
        """
        return self._sub_block_id

    @property
    def block_id(self: any) -> int:
        """
        get block_id
        :return: block_id
        """
        return self._block_id

    @property
    def total_cycle(self: any) -> int:
        """
        get total_cycle
        :return: total_cycle
        """
        return self._total_cycle

    @property
    def pmu_list(self: any) -> list:
        """
        get pmu_list
        :return: pmu_list
        """
        return self._pmu_list

    @property
    def start_time(self: any) -> int:
        """
        get start_time
        :return: start_time
        """
        return self._start_time

    @property
    def end_time(self: any) -> int:
        """
        get end_time
        :return: end_time
        """
        return self._end_time

    @property
    def subtask_id(self: any) -> int:
        """
        get subtask_id
        :return: subtask_id
        """
        return NumberConstant.DEFAULT_GE_CONTEXT_ID

    @property
    def ffts_type(self: any) -> int:
        return 0

    @property
    def subtask_type(self) -> int:
        return 0

    @property
    def ov_flag(self) -> bool:
        """
        get ov_flag
        :return: ov_flag status
        """
        return self._ov_flag == 1

    def is_aic_data(self):
        """
        get if aic data
        """
        return self._core_type == 0

    def is_mix_data(self):
        """
        get if mix data
        """
        return self._mix == 1

    def is_mst_data(self):
        """
        get if mst data
        """
        return self._mst == 1
