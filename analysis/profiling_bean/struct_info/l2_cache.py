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

import logging
import struct

from msparser.data_struct_size_constant import StructFmt
from msparser.interface.idata_bean import IDataBean


class L2CacheDataBean(IDataBean):
    """
    l2 cache bean data for the data parsing by l2 cache parser.
    """

    TASK_TYPE_INDEX = 0
    STREAM_ID_INDEX = 1
    TASK_ID_INDEX = 2
    L2_CACHE_EVENT_START_INDEX = 4
    L2_CACHE_DATA_NUM = 12

    def __init__(self: any) -> None:
        self._task_type = None
        self._stream_id = None
        self._task_id = None
        self._events_list = []

    @property
    def task_type(self: any) -> int:
        """
        l2 cache task type
        """
        return self._task_type

    @property
    def task_id(self: any) -> int:
        """
        l2 cache task id
        """
        return self._task_id

    @property
    def events_list(self: any) -> list:
        """
        l2 cache events list
        """
        return self._events_list

    @property
    def stream_id(self: any) -> int:
        """
        l2 cache stream id
        """
        return self._stream_id

    def decode(self: any, bin_data: bytes) -> None:
        """
        decode the l2 cache bin data
        :param bin_data: l2 cache bin data
        :return: instance of l2 cache
        """
        if not self.construct_bean(struct.unpack(StructFmt.L2_CACHE_STRUCT_FMT, bin_data)):
            logging.error("l2 cache data struct is incomplete, please check the l2 cache file.")

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the l2 cache data
        :param args: l2 cache data
        :return: True or False
        """
        l2_cache_data = args[0]
        if len(l2_cache_data) != self.L2_CACHE_DATA_NUM:
            return False
        self._task_type = l2_cache_data[self.TASK_TYPE_INDEX]
        self._stream_id = l2_cache_data[self.STREAM_ID_INDEX]
        self._task_id = l2_cache_data[self.TASK_ID_INDEX]
        for _event_index in range(self.L2_CACHE_EVENT_START_INDEX, self.L2_CACHE_DATA_NUM):
            self._events_list.append(str(l2_cache_data[_event_index]))
        return True
