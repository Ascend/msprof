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
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel


class TestRuntimeHostTaskModel(TestDirCRBaseModel):
    def test__get_host_tasks_should_return_empty_when_no_db(self):
        model = RuntimeHostTaskModel(self.PROF_DIR)
        tasks = model.get_host_tasks(True, 1, 0, 0)
        self.assertEqual(tasks, [])

    def test__get_host_tasks_should_return_no_empty_when_not_all_in_different_scene(self):
        host_data = [
            [2, 0, 2, 15, "1", 0, "AI_CORE", 'aclnn', 0, 1000, 0],
            [2, 0, 2, 16, "1", 0, "AI_CORE", 'aclnn', 0, 1001, 1],
            [2, 0, 2, 15, "1", 1, "AI_CORE", 'aclnn', 0, 1002, 2],
            # dynamic and static mix 'aclnn',
            [2, 1, 2, 25, "1", 1, "AI_CORE", 'aclnn', 0, 1002.5, 3],
            [3, 1, 3, 22, "1", 0, "AI_CORE", 'aclnn', 0, 1003, 4],
            [3, 1, 3, 23, "1", 0, "AI_CORE", 'aclnn', 0, 1004, 5],
            [3, 1, 3, 22, "1", 1, "AI_CORE", 'aclnn', 0, 1005, 6],
            # model unload 'aclnn',
            [5, 0, 2, 15, "1", 0, "AI_CORE", 'aclnn', 0, 1009, 7],
        ]
        model = RuntimeHostTaskModel(self.PROF_DEVICE_DIR)
        model.init()
        model.create_table()
        model.flush(host_data, DBNameConstant.TABLE_HOST_TASK)

        mix_dynamic_and_static_host_tasks = model.get_host_tasks(False, 2, 1, 0)
        self.assertEqual(len(mix_dynamic_and_static_host_tasks), 4)

        pure_dynamic_host_tasks = model.get_host_tasks(False, 3, 1, 0)
        self.assertEqual(len(pure_dynamic_host_tasks), 3)

        host_tasks = model.get_host_tasks(False, 3, 2, 0)
        self.assertEqual(len(host_tasks), 0)

        host_tasks_after_unload = model.get_host_tasks(False, 5, 2, 0)
        self.assertEqual(len(host_tasks_after_unload), 1)
        model.finalize()

    def test__get_host_tasks_should_return_no_empty_when_all_in_different_scene(self):
        host_data = [
            [4294967295, -1, 4, 22, "1", 0, "AI_CORE", 'aclnn', 0, 1006, 0],
            [4294967295, -1, 4, 23, "1", 0, "AI_CORE", 'aclnn', 0, 1007, 1],
            [4294967295, -1, 4, 22, "1", 1, "AI_CORE", 'aclnn', 0, 1008, 2],
        ]
        model = RuntimeHostTaskModel(self.PROF_DEVICE_DIR)
        model.init()
        model.create_table()
        model.flush(host_data, DBNameConstant.TABLE_HOST_TASK)

        host_tasks = model.get_host_tasks(True, -1, -1, 0)
        self.assertEqual(len(host_tasks), 3)

        model.finalize()
