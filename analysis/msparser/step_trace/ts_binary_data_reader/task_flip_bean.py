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
from dataclasses import dataclass

from profiling_bean.struct_info.struct_decoder import StructDecoder
from common_func.ms_constant.number_constant import NumberConstant


class TaskFlipBean(StructDecoder):
    """
    device task flip bean
    """

    def __init__(self: any, *args: any) -> None:
        task_flip_data = args[0]
        self._timestamp = task_flip_data[4]
        self._stream_id = task_flip_data[5]
        self._flip_num = task_flip_data[6]
        self._task_id = task_flip_data[8]

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
        get stream_id
        :return: stream_id
        """
        return self._stream_id

    @property
    def flip_num(self: any) -> int:
        """
        get flip_num
        :return: flip_num
        """
        return self._flip_num

    @property
    def task_id(self: any) -> int:
        """
        get flip_num
        :return: flip_num
        """
        return self._task_id


@dataclass
class TaskFlip:
    """
    This class represents a task flip dataclass.
    """

    stream_id: int = NumberConstant.DEFAULT_STREAM_ID
    timestamp: int = NumberConstant.DEFAULT_START_TIME
    task_id: int = NumberConstant.DEFAULT_TASK_ID
    flip_num: int = 0
