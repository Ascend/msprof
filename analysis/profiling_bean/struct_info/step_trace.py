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

from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StepTrace(StructDecoder):
    """
    step trace
    """

    def __init__(self: any, *args: any) -> None:
        step_trace = args[0]
        self._timestamp = step_trace[4]
        self._index_id = step_trace[5]
        self._model_id = step_trace[6]
        self._stream_id = Utils.get_stream_id(step_trace[7])
        self._task_id = step_trace[8]
        self._tag_id = step_trace[9]

    @property
    def index_id(self: any) -> str:
        """
        get mode
        :return: mode
        """
        return self._index_id

    @property
    def model_id(self: any) -> int:
        """
        get task cyc
        :return: task cyc
        """
        return self._model_id

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def stream_id(self: any) -> int:
        """
        get count_num
        :return: count_num
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._task_id

    @property
    def tag_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._tag_id


class StepTraceChipV6(StepTrace):
    """
    step trace for chip v6
    """

    def __init__(self: any, *args: any) -> None:
        super().__init__(*args)
        step_trace = args[0]
        self._timestamp = step_trace[4]
        self._index_id = step_trace[5]
        self._model_id = step_trace[6]
        self._tag_id = step_trace[7]
        self._stream_id = step_trace[8]
        self._task_id = step_trace[9]
