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
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.ai_cpu.ai_cpu_model'


class TestAiCpuModel(TestDirCRBaseModel):
    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            AiCpuModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AiCpuModel.insert_data_to_db'):
            self.assertEqual(AiCpuModel('test', ['test']).flush([]), None)

    def test_get_ai_cpu_data_within_time_range_by_different_time_staggered_situation(self):
        ai_cpu_data = [
            [2, 23, 1000, 2000],
            [2, 24, 3000, 4000],
            [2, 25, 5000, 6000],
        ]
        model = AiCpuModel(self.PROF_DEVICE_DIR, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        model.init()
        model.flush(ai_cpu_data, DBNameConstant.TABLE_AI_CPU_FROM_TS)

        device_tasks = model.get_ai_cpu_data_within_time_range(float("-inf"), float("inf"))
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_ai_cpu_data_within_time_range(1500000000, 2800000000)
        self.assertEqual(len(device_tasks), 1)

        device_tasks = model.get_ai_cpu_data_within_time_range(9000000000, 10000000000)
        self.assertEqual(len(device_tasks), 0)

        model.finalize()


if __name__ == '__main__':
    unittest.main()
