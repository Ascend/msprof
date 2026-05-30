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

from common_func.constant import Constant
from common_func.msprof_iteration import MsprofIteration
from common_func.profiling_scene import ProfilingScene
from profiling_bean.db_dto.step_trace_dto import IterationRange


class TaskDispatchModelIndex:
    """
    Dispatch model id and index id for each task
    """

    def __init__(self: any, iter_range: IterationRange, result_dir: int) -> None:
        self.iter_range = iter_range
        self.result_dir = result_dir
        self.iteration_info_list = self.init_iteration_info_list()
        self.iter_range_syscnt_end = MsprofIteration(result_dir).get_step_end_range_by_iter_range(iter_range)

    def init_iteration_info_list(self: any) -> any:
        """
        init iteration info list
        """
        iteration_info_list = []
        if not (ProfilingScene().is_mix_operator_and_graph() and self.iter_range.model_id == Constant.GE_OP_MODEL_ID):
            return iteration_info_list

        msprof_iteration = MsprofIteration(self.result_dir)
        iter_list = msprof_iteration.get_index_id_list_with_index_and_model(self.iter_range)

        for index_id, model_id in iter_list:
            iteration_info = msprof_iteration.get_iteration_info_by_index_id(index_id, model_id)
            if iteration_info:
                iteration_info_list.append(iteration_info)

        # after sorting, the last element of iteration info list is op model id and its index id
        iteration_info_list.sort(key=lambda iter_info: iter_info.step_end)

        return iteration_info_list

    def dispatch(self: any, end_time: int) -> tuple:
        """
        dispatch model id and index id for each task
        """
        # only mix and model id is op model id scene, model id and index id for each task might be different
        # the last element of iteration info list is op model id and its index id
        for iter_info in self.iteration_info_list:
            if iter_info.step_start < end_time <= iter_info.step_end:
                return iter_info.model_id, iter_info.index_id
        # For parallel scene, can not find accurately model id and index id
        while self.iter_range_syscnt_end:
            syscnt_end = self.iter_range_syscnt_end[0]
            if end_time <= syscnt_end.step_end:
                return self.iter_range.model_id, syscnt_end.index_id
            else:
                self.iter_range_syscnt_end.pop(0)
        return self.iter_range.model_id, self.iter_range.iteration_end
