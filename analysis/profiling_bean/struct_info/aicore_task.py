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


class TaskExecuteBean:
    def __init__(self: any, *args: any) -> None:
        self._stream_id = args[0]
        self._task_id = args[1]
        self._start_time = args[2]
        self._end_time = args[3]
        self._task_type = args[4]

    @property
    def stream_id(self: any) -> int:
        """
        for stream_id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        for task_id
        """
        return self._task_id

    @property
    def start_time(self: any) -> int:
        """
        for start time
        """
        return self._start_time

    @property
    def end_time(self: any) -> int:
        """
        for end time
        """
        return self._end_time

    @property
    def task_type(self: any) -> int:
        """
        for task type
        """
        return self._task_type
