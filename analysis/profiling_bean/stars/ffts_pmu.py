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


from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FftsPmuBean(StructDecoder):
    """
    class used to decode ffts pmu data
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self._stream_id = StarsCommon.set_stream_id(filed[2], filed[3])
        self._task_id = StarsCommon.set_task_id(filed[2], filed[3])
        self._ov_flag = (filed[5] >> 10) & 1  # 0 / 1
        self._subtask_id = filed[6]
        self._ffts_type = filed[7] >> 13
        self._subtask_type = filed[5] & int(b'11111111')
        self._total_cycle = filed[10]
        self._pmu_list = filed[12:20]
        self._time_list = filed[20:]

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
        if self.is_ffts_plus_type():
            return self._subtask_id
        return NumberConstant.DEFAULT_GE_CONTEXT_ID  # Traditional mode set _subtask_id = 4294967295

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

    @property
    def ov_flag(self: any) -> bool:
        """
        get ov_flag
        :return: ov_flag status
        """
        return self._ov_flag == 1

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
        return self.is_ffts_plus_type() and self.subtask_type == 6 or self._ffts_type == 5

    def is_tradition_aic(self):
        """
        get if tradition aic
        """
        return self._ffts_type == 0

    def is_ffts_mix_aiv_data(self):
        """
        get if ffts mix aic
        """
        return self.is_ffts_plus_type() and self.subtask_type == 7

    def is_ffts_plus_type(self):
        """
        get if ffts plus type
        """
        return self._ffts_type == 4
