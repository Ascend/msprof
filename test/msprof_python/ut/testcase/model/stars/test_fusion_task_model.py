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
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.fusion_task_model import FusionTaskModel

NAMESPACE = 'msmodel.stars.fusion_task_model'


class TestFusionTaskModel(TestDirCRBaseModel):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.FusionTaskModel.insert_data_to_db') as mock_insert:
            model = FusionTaskModel('test', 'test', [])
            model.flush([])
            mock_insert.assert_called_once_with(DBNameConstant.TABLE_FUSION_TASK, [])

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[(1, 2)]), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            model = FusionTaskModel('test', 'test', [])
            ret = model.get_summary_data()
        self.assertEqual(ret, [(1, 2)])

    def test_get_summary_data_table_not_exist(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            model = FusionTaskModel('test', 'test', [])
            ret = model.get_summary_data()
        self.assertEqual(ret, [])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + '.FusionTaskModel.get_all_data', return_value=1):
            model = FusionTaskModel('test', 'test', [])
            ret = model.get_timeline_data()
        self.assertEqual(ret, 1)

    def test_flush_and_get_summary_data_integration(self):
        """data_list: 10 fields incl. display fusion_task_type, mission_id, ccu_die_id"""
        fusion_data = [
            [0, 100, 3, 'AI_CORE', 1120.0, 1200.0, 80.0, 'CPU', 0, None],
            [0, 200, 7, 'AI_CORE', 1600.0, 1800.0, 200.0, 'CCU', 1, 0],
        ]
        model = FusionTaskModel(
            self.PROF_DEVICE_DIR, DBNameConstant.DB_FUSION_TASK,
            [DBNameConstant.TABLE_FUSION_TASK],
        )
        model.init()
        try:
            model.flush(fusion_data)
            ret = model.get_summary_data()
            self.assertEqual(len(ret), 2)
            self.assertEqual(ret[0].op_name, 'CPU')
            self.assertEqual(ret[0].mission_id, 0)
            self.assertEqual(ret[0].ccu_die_id, None)
            self.assertEqual(ret[1].op_name, 'CCU')
            self.assertEqual(ret[1].mission_id, 1)
            self.assertEqual(ret[1].ccu_die_id, 0)
        finally:
            model.finalize()
