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

from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from msparser.iter_rec.iter_info_updater.iter_info_manager import IterInfoManager


class IterInfoUpdater:
    HWTS_TASK_END = 1

    def __init__(self: any, project_path) -> None:
        self.current_iter = -1
        self.iteration_manager = IterInfoManager(project_path)
        self.active_parallel_iter_id = set([])
        self.active_parallel_iter_info = set([])

    @staticmethod
    def update_hwts(iter_info_list: list) -> None:
        """
        update hwts
        """
        for iter_info_bean in iter_info_list:
            iter_info_bean.hwts_count += 1

    @staticmethod
    def update_aicore(iter_info_list: list) -> None:
        """
        update aicore
        """
        for iter_info_bean in iter_info_list:
            iter_info_bean.aic_count += 1

    def update_parallel_iter_info_pool(self: any, iter_id: int) -> None:
        """
        update parallel iter info pool
        """
        # when iter id is increased, iter info should be updated
        while self.current_iter < iter_id:
            next_iter = self.current_iter + 1
            # calculate iter info should be add
            new_iter_info = self.iteration_manager.iter_to_iter_info.get(next_iter, IterInfo())

            new_add_parallel_id = []
            for it_id in new_iter_info.behind_parallel_iter - self.active_parallel_iter_id:
                new_add_parallel_id.append(self.iteration_manager.iter_to_iter_info.get(it_id))

            self.update_new_add_iter_info(new_add_parallel_id)

            self.current_iter = next_iter
            # update current iter info
            self.active_parallel_iter_id = new_iter_info.behind_parallel_iter
            self.active_parallel_iter_info = {
                self.iteration_manager.iter_to_iter_info.get(parallel_iter_id, IterInfo())
                for parallel_iter_id in self.active_parallel_iter_id
            }

    def update_new_add_iter_info(self: any, new_add_parallel_id: any) -> None:
        """
        update new add iter info
        """
        current_iter_info = self.iteration_manager.iter_to_iter_info.get(self.current_iter, IterInfo())

        for new_add_parallel_iter_info in new_add_parallel_id:
            new_add_parallel_iter_info.hwts_offset = current_iter_info.hwts_offset + current_iter_info.hwts_count
            new_add_parallel_iter_info.aic_offset = current_iter_info.aic_offset + current_iter_info.aic_count

    def update_count_and_offset(self: any, task: any) -> None:
        """
        update count and offset
        """
        self.update_hwts(self.active_parallel_iter_info)

        if task.sys_tag == self.HWTS_TASK_END and task.is_ai_core:
            self.update_aicore(self.active_parallel_iter_info)

    def judge_ai_core(self: any, task: any, ai_core_task: set) -> bool:
        """
        judge ai core
        """
        # if there are ge data, ai_core_task is empty
        if not ai_core_task:
            return any(iter_info_bean.is_aicore(task) for iter_info_bean in self.active_parallel_iter_info)
        return GeInfoModel.STREAM_TASK_KEY_FMT.format(task.stream_id, task.task_id) in ai_core_task

    def update_iter_without_hwts(self: any) -> None:
        """
        update iter without hwts
        """
        if not self.iteration_manager.iter_to_iter_info:
            return
        max_iter_id = max(self.iteration_manager.iter_to_iter_info.keys())
        self.update_parallel_iter_info_pool(max_iter_id)

    def calibrate_iter_info_offset(self: any, task_offset: int, iter_offset: int):
        for iter_id, iter_info in self.iteration_manager.iter_to_iter_info.items():
            if iter_id >= iter_offset:
                iter_info.hwts_offset += task_offset

    def calibrate_aic_offset(self: any, pmu_cnt_not_in_iter: dict, remain_aic_count: int) -> None:
        # Reverse traverse to calculate aic offset considering data aging
        #   if the last aic data is consistent with hwts.data.
        max_iter = max(pmu_cnt_not_in_iter.keys()) - 1
        for iter_id in range(max_iter, 0, -1):
            aic_count_after_iter = pmu_cnt_not_in_iter.get(iter_id + 1, 0)
            iter_info: IterInfo = self.iteration_manager.iter_to_iter_info[iter_id]  # current iter info
            # aic offset before current iter
            aic_offset = remain_aic_count - aic_count_after_iter - iter_info.aic_count
            if aic_offset < 0:
                # 处理老化场景，分两种情况：
                #   1. 迭代内所有ai_core数据都被老化，iter_info.aic_count = 0
                #   2. 迭代内部分ai_core数据都被老化，iter_info.aic_count = iter_info.aic_count + aic_offset
                iter_info.aic_offset = 0
                iter_info.aic_count = iter_info.aic_count + aic_offset if iter_info.aic_count + aic_offset > 0 else 0
                remain_aic_count = aic_offset
                continue
            iter_info.aic_offset = aic_offset
            remain_aic_count = aic_offset
