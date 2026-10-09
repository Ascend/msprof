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

import os

from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class CounterInfo:
    """
    counter info
    """

    def __init__(self: any, *args: any) -> None:
        self._overflow = args[0]
        self._overflow_cycle = args[1]
        self._event_counter = args[2]
        self._task_cyc = args[3]
        self._time_stamp = args[4]
        self._block = args[5]

    @property
    def overflow(self: any) -> any:
        """
        get overflow
        :return: get overflow
        """
        return self._overflow

    @property
    def overflow_cycle(self: any) -> any:
        """
        get overflow_cycle
        :return: get overflow_cycle
        """
        return self._overflow_cycle

    @property
    def event_counter(self: any) -> any:
        """
        get event_counter
        :return: get event_counter
        """
        return self._event_counter

    @property
    def task_cyc(self: any) -> any:
        """
        get task_cyc
        :return: get task_cyc
        """
        return self._task_cyc

    @property
    def time_stamp(self: any) -> any:
        """
        get time_stamp
        :return: get time_stamp
        """
        return self._time_stamp

    @property
    def block(self: any) -> any:
        """
        get block
        :return: get block
        """
        return self._block

    @overflow_cycle.setter
    def overflow_cycle(self: any, value: any) -> None:
        """
        set overflow cycle
        :param value: overflow cycle
        :return: None
        """
        self._overflow_cycle = value


class AiCoreTaskInfo(StructDecoder):
    """
    class for decode aicore data
    """

    def __init__(self: any, *args: any) -> None:
        filed = args[0]
        self.task_type = filed[4]
        self.stream_id = Utils.get_stream_id(filed[5])
        self.task_id = filed[6]
        self.counter_info = CounterInfo(filed[7], filed[8], filed[9:17], filed[17], filed[18], filed[19])

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return AiCoreTaskInfo.__name__

    @staticmethod
    def file_name() -> str:
        """
        file name
        """
        return os.path.basename(__file__)
