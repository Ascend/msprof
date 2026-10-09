#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from msparser.iter_rec.iter_info_updater.iter_info_updater import IterInfoUpdater


class TestIterInfoUpdater(unittest.TestCase):

    def test_update_parallel_iter_info_pool_1(self: any) -> None:
        instance = mock.Mock()
        instance.current_iter = 1
        IterInfoUpdater.update_parallel_iter_info_pool(instance, 1)

    def test_update_parallel_iter_info_pool_2(self: any) -> None:
        self_instance = mock.Mock()
        self_instance.current_iter = 10

        new_iter_info = mock.Mock()
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = new_iter_info
        new_iter_info.behind_parallel_iter = {2, 3, 4}

        self_instance.active_parallel_iter_id = {1, 2, 3}
        IterInfoUpdater.update_parallel_iter_info_pool(self_instance, 11)

        self.assertEqual(self_instance.current_iter, 11)
        self.assertEqual(self_instance.active_parallel_iter_id, {2, 3, 4})

    def test_update_new_add_iter_info(self: any) -> None:
        self_instance = mock.Mock()

        current_iter_info = mock.Mock()
        current_iter_info.hwts_offset = 1
        current_iter_info.aic_offset = 2
        current_iter_info.hwts_count = 1
        current_iter_info.aic_count = 2
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = current_iter_info

        IterInfoUpdater.update_new_add_iter_info(self_instance, [mock.Mock(), mock.Mock()])

    def test_update_count_and_offset(self: any) -> None:
        self_instance = mock.Mock()
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = mock.Mock()
        self_instance.active_parallel_iter_set = {1, 2, 3}
        self_instance.judge_ai_core.return_value = True

        task = mock.Mock()
        task.sys_tag = IterInfoUpdater.HWTS_TASK_END

        ai_core_task = set([])

        IterInfoUpdater.update_count_and_offset(self_instance, task)

    def test_judge_ai_core_1(self: any) -> None:
        task = mock.Mock()
        task.stream_id = 1
        task.task_id = 1

        ai_core_task = ("1-1", "2-10")

        IterInfoUpdater("test").judge_ai_core(task, ai_core_task)

    def test_judge_ai_core_2(self: any) -> None:
        task = mock.Mock()
        task.stream_id = 1
        task.task_id = 1

        ai_core_task = ()

        IterInfoUpdater("test").judge_ai_core(task, ai_core_task)

    def test_update_hwts(self: any) -> None:
        iter_info_bean = mock.Mock()
        iter_info_bean.hwts_count = 1
        iter_info_list = [iter_info_bean]

        IterInfoUpdater.update_hwts(iter_info_list)

        self.assertEqual(iter_info_bean.hwts_count, 2)

    def test_update_aicore(self: any) -> None:
        iter_info_bean = mock.Mock()
        iter_info_bean.aic_count = 1
        iter_info_list = [iter_info_bean]

        IterInfoUpdater.update_aicore(iter_info_list)

        self.assertEqual(iter_info_bean.aic_count, 2)

    def test_calibrate_iter_info_offset(self: any) -> None:
        iter_info_updater = IterInfoUpdater('./')
        iter_info = IterInfo(1, 1, 1, 2, 3)
        iter_info_updater.iteration_manager.iter_to_iter_info = {
            1: iter_info,
        }
        iter_info_updater.calibrate_iter_info_offset(1, 1)
        self.assertEqual(iter_info.hwts_offset, 1)

    def test_calibrate_aic_offset_should_no_change_when_there_is_zero_task_not_in_iter(self: any):
        iter_info_updater = IterInfoUpdater('./')
        iter_info1 = IterInfo()
        iter_info1.aic_count = 25
        iter_info2 = IterInfo()
        iter_info2.aic_offset = 25
        iter_info2.aic_count = 35
        iter_info_updater.iteration_manager.iter_to_iter_info = {
            1: iter_info1,
            2: iter_info2,
        }
        pmu_cnt_not_in_iter = {
            3: 0,  # 0 pmu data after iter 2
        }
        remain_aic_count = 60
        iter_info_updater.calibrate_aic_offset(pmu_cnt_not_in_iter, remain_aic_count)
        self.assertEqual(0, iter_info_updater.iteration_manager.iter_to_iter_info[1].aic_offset)
        self.assertEqual(25, iter_info_updater.iteration_manager.iter_to_iter_info[1].aic_count)
        self.assertEqual(25, iter_info_updater.iteration_manager.iter_to_iter_info[2].aic_offset)
        self.assertEqual(35, iter_info_updater.iteration_manager.iter_to_iter_info[2].aic_count)

    def test_calibrate_aic_offset_should_return_aic_offset_11_and_aic_count_35_when_aging_10_pmu_data(self: any):
        # there is 70 ai_core tasks in hwts.data, but just 50 pmu data because of data aging.
        # | aging pmu before iter 1 | aging pmu iter 1 | iter 1 | pmu between iter 1 and 2 | iter 2 | pmu after iter 2 |
        #             1                      19             6                5                 35           4
        iter_info_updater = IterInfoUpdater('./')
        iter_info1 = IterInfo()
        iter_info1.aic_count = 25
        iter_info2 = IterInfo()
        iter_info2.aic_offset = 25
        iter_info2.aic_count = 35
        iter_info_updater.iteration_manager.iter_to_iter_info = {
            1: iter_info1,
            2: iter_info2,
        }
        pmu_cnt_not_in_iter = {
            1: 1,  # 1 pmu before iter 1
            2: 5,  # 5 pmu between iter 1 and 2
            3: 4,  # 4 pmu after iter 2
        }
        remain_aic_count = 50
        iter_info_updater.calibrate_aic_offset(pmu_cnt_not_in_iter, remain_aic_count)
        self.assertEqual(0, iter_info_updater.iteration_manager.iter_to_iter_info[1].aic_offset)
        self.assertEqual(6, iter_info_updater.iteration_manager.iter_to_iter_info[1].aic_count)
        self.assertEqual(11, iter_info_updater.iteration_manager.iter_to_iter_info[2].aic_offset)
        self.assertEqual(35, iter_info_updater.iteration_manager.iter_to_iter_info[2].aic_count)
