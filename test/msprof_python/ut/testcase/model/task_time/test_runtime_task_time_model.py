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
from msmodel.task_time.runtime_task_time_model import RuntimeTaskTimeModel

NAMESPACE = 'msmodel.stars.acsq_task_model'


class TestRuntimeTaskTimeModel(TestDirCRBaseModel):
    def test_get_runtime_task_data_within_time_range_by_different_time_staggered_situation(self):
        ts_data = [
            [0, 0, "api", 0, 0, 1, 23, 0, 0, 1000, 2000, 0, 0, 0],
            [0, 0, "api", 0, 0, 2, 23, 0, 0, 3000, 4000, 0, 0, 0],
            [0, 0, "api", 0, 0, 3, 23, 0, 0, 5000, 6000, 0, 0, 0],
            [0, 0, "api", 0, 0, 4, 24, 0, 0, 7000, 8000, 0, 0, 0],
        ]
        model = RuntimeTaskTimeModel(self.PROF_HOST_DIR)
        model.init()
        model.create_table()
        model.insert_data_to_db(DBNameConstant.TABLE_RUNTIME_TASK_TIME, ts_data)

        device_tasks = model.get_runtime_task_data_within_time_range(3500, 4500)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_runtime_task_data_within_time_range(0, 6000)
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_runtime_task_data_within_time_range(0, 4999)
        self.assertEqual(len(device_tasks), 2)

        device_tasks = model.get_runtime_task_data_within_time_range(7800, 8000)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_runtime_task_data_within_time_range(200, 500)
        self.assertEqual(len(device_tasks), 0)

        model.finalize()
