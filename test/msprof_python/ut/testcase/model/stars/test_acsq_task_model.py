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
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.acsq_task_model import AcsqTaskModel

NAMESPACE = 'msmodel.stars.acsq_task_model'


class TestAcsqTaskModel(TestDirCRBaseModel):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.AcsqTaskModel.insert_data_to_db'):
            check = AcsqTaskModel('test', 'test', [])
            check.flush([])

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(1, 2)]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=1):
            check = AcsqTaskModel('test', 'test', [])
            ret = check.get_summary_data()
        self.assertEqual(ret, [(1, 2)])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.AcsqTaskModel.get_all_data', return_value=1):
            check = AcsqTaskModel('test', 'test', [])
            ret = check.get_timeline_data()
        self.assertEqual(ret, 1)

    def test_get_ffts_type_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=True), \
                mock.patch(NAMESPACE + '.AcsqTaskModel.get_all_data', return_value=1):
            check = AcsqTaskModel('test', 'test', [])
            ret = check.get_ffts_type_data()
        self.assertEqual(ret, 1)

    def test_get_acsq_data_within_time_range_by_different_time_staggered_situation(self):
        acsq_data = [
            [2, 23, 0, "AI_CORE", 1000, 2000, 1000],
            [2, 24, 0, "AI_CORE", 3000, 4000, 1000],
            [2, 25, 0, "AI_CORE", 5000, 6000, 1000],
        ]
        model = AcsqTaskModel(self.PROF_DEVICE_DIR, DBNameConstant.DB_SOC_LOG, [DBNameConstant.TABLE_ACSQ_TASK])
        model.init()
        model.flush(acsq_data)

        device_tasks = model.get_acsq_data_within_time_range(float("-inf"), float("inf"))
        self.assertEqual(len(device_tasks), 3)

        device_tasks = model.get_acsq_data_within_time_range(0, 500)
        self.assertEqual(len(device_tasks), 0)

        device_tasks = model.get_acsq_data_within_time_range(3500, 4500)
        self.assertEqual(len(device_tasks), 1)
        model.finalize()
