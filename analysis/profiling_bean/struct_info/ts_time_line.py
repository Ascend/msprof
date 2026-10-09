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


class TimeLineData(StructDecoder):
    """
    class for decode timeline data
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self._task_type = filed[4]
        self._task_state = filed[5]
        self._stream_id = Utils.get_stream_id(filed[6])
        self._task_id = filed[7]
        self._time_stamp = filed[8]
        self._thread = filed[9]
        self._device_id = filed[10]

    @property
    def task_type(self: any) -> any:
        """
        task type
        :return: task type
        """
        return self._task_type

    @property
    def task_state(self: any) -> any:
        """
        task state
        :return: task state
        """
        return self._task_state

    @property
    def stream_id(self: any) -> any:
        """
        stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        """
        task id
        :return: task id
        """
        return self._task_id

    @property
    def time_stamp(self: any) -> any:
        """
        time stamp
        :return: time stamp
        """
        return self._time_stamp

    @property
    def thread(self: any) -> any:
        """
        thread
        :return: thread
        """
        return self._thread

    @property
    def device_id(self: any) -> any:
        """
        device id
        :return: device id
        """
        return self._device_id
