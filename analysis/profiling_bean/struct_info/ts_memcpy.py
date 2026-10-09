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


class TsMemcpy(StructDecoder):
    """
    ts memcpy
    """

    def __init__(self: any, *args: any) -> None:
        ts_memcpy = args[0]
        self._timestamp = ts_memcpy[4]
        self._stream_id = Utils.get_stream_id(ts_memcpy[5])
        self._task_id = ts_memcpy[6]
        self._task_state = ts_memcpy[7]

    @property
    def stream_id(self: any) -> int:
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def task_id(self: any) -> int:
        """
        get task id
        :return: task id
        """
        return self._task_id

    @property
    def task_state(self: any) -> int:
        """
        get task state
        :return: task state
        """
        return self._task_state
