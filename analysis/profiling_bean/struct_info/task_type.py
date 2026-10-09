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


class TaskTypeBean(StructDecoder):
    """
    step trace
    """

    def __init__(self: any, *args: any) -> None:
        task_type_data = args[0]
        self._timestamp = task_type_data[4]
        self._stream_id = Utils.get_stream_id(task_type_data[5])
        self._task_id = task_type_data[6]
        self._task_type = task_type_data[7]
        self._task_state = task_type_data[8]

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
    def task_id(self: any) -> int:
        """
        get task_id
        :return: task_id
        """
        return self._task_id

    @property
    def task_type(self: any) -> int:
        """
        get task_type
        :return: task_type
        """
        return self._task_type

    @property
    def task_state(self: any) -> int:
        """
        get task state
        :return: task state
        """
        return self._task_state
