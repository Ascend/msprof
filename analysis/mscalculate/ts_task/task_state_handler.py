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


class TaskTimeline:
    """
    contain start and end time of task
    """

    def __init__(self: any, stream_id: int, task_id: int) -> None:
        self._stream_id = stream_id
        self._task_id = task_id
        self.start_time = None
        self.end_time = None

    @property
    def stream_id(self: any) -> int:
        """
        stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        task id
        :return: task id
        """
        return self._task_id


class TaskStateHandler:
    """
    handle task state
    """

    RECEIVE_TAG = 0
    START_TAG = 1
    END_TAG = 2

    def __init__(self: any, stream_id: int, task_id: int) -> None:
        self._stream_id = stream_id
        self._task_id = task_id
        self.new_task = None
        self.task_timeline_list = []

    @property
    def stream_id(self: any) -> int:
        """
        stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        task id
        :return: task id
        """
        return self._task_id

    def process_record(self: any, timestamp: int, task_state: int) -> None:
        """
        process record
        :param timestamp: timestamp
        :param task_state: task state
        """
        if task_state == self.START_TAG:
            self.new_task = TaskTimeline(self.stream_id, self.task_id)
            self.new_task.start_time = timestamp

        if task_state == self.END_TAG and self.new_task:
            self.new_task.end_time = timestamp
            self.task_timeline_list.append(self.new_task)
