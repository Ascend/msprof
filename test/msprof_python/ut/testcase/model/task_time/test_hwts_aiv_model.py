#!/usr/bin/python3
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
from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.task_time.hwts_aiv_model import HwtsAivModel


class TestHwtsAivModel(TestDirCRBaseModel):
    def test_get_hwts_aiv_data_within_time_range_by_different_time_staggered_situation(self):
        hwts_data = [
            [2, 23, 1000, 2000, "AI_CORE", 1, 1],
            [2, 24, 3000, 4000, "AI_CORE", 1, 1],
            [2, 25, 5000, 6000, "AI_CORE", 1, 1],
        ]
        model = HwtsAivModel(self.PROF_DEVICE_DIR, [DBNameConstant.TABLE_HWTS_TASK_TIME])
        model.init()
        model.flush_data(hwts_data, DBNameConstant.TABLE_HWTS_TASK_TIME)

        device_tasks = model.get_hwts_aiv_data_within_time_range(float("-inf"), float("inf"))
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_hwts_aiv_data_within_time_range(1500, 4000)
        self.assertEqual(len(device_tasks), 2)

        device_tasks = model.get_hwts_aiv_data_within_time_range(9000, 10000)
        self.assertEqual(len(device_tasks), 0)

        model.finalize()
