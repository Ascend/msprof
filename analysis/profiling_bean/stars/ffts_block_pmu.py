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

from common_func.utils import Utils
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FftsBlockPmuBean(StructDecoder):
    """
    class used to decode ffts pmu data
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self.func_type = Utils.get_func_type(filed[0])
        self._stream_id = StarsCommon.set_stream_id(filed[2], filed[3])
        self._task_id = StarsCommon.set_task_id(filed[2], filed[3])
        self._subtask_type = filed[5] & int(b'11111111')
        self._subtask_id = filed[6]
        self._core_type = filed[7] & 1
        self._core_id = filed[7] >> 1
        self._ffts_type = filed[8] >> 5
        self._sub_block_id = filed[11]
        self._block_id = filed[12]
        self._total_cycle = filed[13]
        self._pmu_list = filed[15:23]
        self._time_list = filed[23:]

    @property
    def stream_id(self: any) -> int:
        """
        get stream_id
        :return: stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        get task_id
        :return: task_id
        """
        return self._task_id

    @property
    def subtask_id(self: any) -> int:
        """
        get subtask_id
        :return: subtask_id
        """
        return self._subtask_id

    @property
    def ffts_type(self: any) -> int:
        """
        get task_type
        :return: task_type
        """
        return self._ffts_type

    @property
    def subtask_type(self: any) -> int:
        """
        get task_type
        :return: task_type
        """
        return self._subtask_type

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
    def time_list(self: any) -> list:
        """
        get time_list
        :return: time_list
        """
        return self._time_list

    def is_aic_data(self):
        """
        get if aic data
        """
        return self.is_ffts_aic() or self.is_ffts_mix_aic_data() or self.is_tradition_aic()

    def is_ffts_aic(self):
        """
        get if ffts aic data
        """
        return self.is_ffts_plus_type() and self.subtask_type == 0

    def is_ffts_mix_aic_data(self):
        """
        get if ffts mix aic
        """
        return (self.is_ffts_plus_type() and self.subtask_type == 6) or self._ffts_type == 5

    def is_tradition_aic(self):
        """
        get if tradition aic
        """
        return self._ffts_type == 0

    def is_ffts_plus_type(self):
        """
        get if ffts plus type
        """
        return self._ffts_type == 4
