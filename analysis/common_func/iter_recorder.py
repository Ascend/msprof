# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.utils import Utils
from common_func.profiling_scene import ProfilingScene
from msmodel.step_trace.ts_track_model import TsTrackModel


class IterRecorder:
    """
    common function of calculating iter id
    """

    STREAM_TASK_KEY_FMT = "{0}-{1}"
    DEFAULT_ITER_ID = -1
    DEFAULT_ITER_TIME = -1

    def __init__(self: any, project_path) -> None:
        self._project_path = project_path
        self._iter_end_dict = dict()
        self._iter_time = []
        self.init_iter_time()
        self._max_iter_time = self._get_max_iter_time()
        self._current_iter_id = self.DEFAULT_ITER_ID

    @property
    def iter_end_dict(self: any) -> dict:
        """
        get iter end dict
        :return: iter end dict
        """
        return self._iter_end_dict

    @property
    def current_iter_id(self: any) -> int:
        """
        get iter id
        :return: iter id
        """
        return self._current_iter_id

    def init_iter_time(self: any) -> None:
        """
        init self._iter_start_dict and self._iter_end_dict
        :return: tuple(iter_start_dict, iter_end_dict)
        """
        if not Utils.is_step_scene(self._project_path):
            return
        with TsTrackModel(
            self._project_path, DBNameConstant.DB_STEP_TRACE, [ProfilingScene().get_step_table_name()]
        ) as ts_track_model:
            step_trace_data = ts_track_model.get_step_trace_data(ProfilingScene().get_step_table_name())
            for step_trace in step_trace_data:
                self._iter_end_dict[step_trace.iter_id] = step_trace.step_end
                self._iter_time.append([step_trace.step_start, step_trace.step_end])

    def check_task_before_max_iter(self: any, sys_cnt: int) -> bool:
        if self._max_iter_time == self.DEFAULT_ITER_TIME:
            return True
        return self._max_iter_time >= sys_cnt

    def check_task_in_iter(self: any, sys_cnt: int, iters: list = None) -> bool:
        if iters is None:
            iters = [self._current_iter_id if self._current_iter_id != self.DEFAULT_ITER_ID else 1]
        for curr_iter in iters:
            for iter_start_time, iter_end_time in self._iter_time[curr_iter - 1 :]:
                if sys_cnt < iter_start_time:
                    break
                if sys_cnt <= iter_end_time:
                    return True
        return False

    def set_current_iter_id(self: any, sys_cnt: int) -> None:
        """
        set current iter id
        :params: sys cnt
        :return: int
        """
        if self._current_iter_id == self.DEFAULT_ITER_ID:
            for iter_id, end_sys_cnt in self._iter_end_dict.items():
                if sys_cnt <= end_sys_cnt:
                    self._current_iter_id = iter_id
                    return
            logging.error("Data cannot be found in any iteration.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        while self._check_current_iter_id(sys_cnt):
            self._current_iter_id += 1

    def reset_current_iter_id(self: any) -> None:
        self._current_iter_id = self.DEFAULT_ITER_ID

    def _check_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end

    def _get_max_iter_time(self: any) -> int:
        if self._iter_end_dict.values():
            return max(self._iter_end_dict.values())
        return self.DEFAULT_ITER_TIME
