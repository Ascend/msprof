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

from common_func.msprof_exception import ProfException


class TaskTime:
    """
    class used to present task time
    """

    def __init__(self: any) -> None:
        self._task_id = None
        self._stream_id = None
        self._start_time = 0
        self._duration_time = 0
        self._wait_time = 0
        self._index_id = 0
        self._model_id = 0

    @property
    def task_id(self):
        """
        get task id
        :return: task_id
        """
        return self._task_id

    @property
    def stream_id(self):
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def wait_time(self):
        """
        get wait time
        :return: wait time
        """
        return self._wait_time

    @property
    def model_id(self):
        """
        get model id
        :return: model id
        """
        return self._model_id

    @staticmethod
    def _pre_check(*args: list) -> None:
        if len(args) != 7:
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, "Invalid task time data")

    @model_id.setter
    def model_id(self, model_id: int) -> None:
        """
        set model id
        :param model_id:
        :return:
        """
        self._model_id = model_id

    @wait_time.setter
    def wait_time(self, last_complete_time: int) -> None:
        """
        set wait time
        :param last_complete_time:
        :param is_first_task:
        :return: None
        """
        if last_complete_time == 0 or self._start_time - last_complete_time < 0:
            self._wait_time = 0
        else:
            self._wait_time = self._start_time - last_complete_time

    def construct(self, *args: list) -> object:
        """
        construct task time instance
        :param args:
        :return: Task time instance
        """
        self._pre_check(args)
        self._task_id = args[0]
        self._stream_id = args[1]
        self._start_time = int(float(args[2]))
        self._duration_time = int(float(args[3])) - int(float(args[2]))
        self._index_id = args[5]
