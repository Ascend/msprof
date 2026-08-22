#!/usr/bin/python3
# -*- coding: utf-8 -*-
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

import unittest
from unittest import mock

from common_func.db_manager import DBManager
from mscalculate.hccl.hccl_task import HcclTask
from msmodel.cluster_info.communication_model import CommunicationModel


class TestCommunicationModel(unittest.TestCase):
    """看护 get_all_events_from_db 的核心逻辑：按 (op_id, iter_id) 把 task 归属到 op"""

    @staticmethod
    def _make_model():
        model = CommunicationModel.__new__(CommunicationModel)
        model.cur = None
        return model

    def test_build_task_sql_only_filters_by_timestamp(self):
        sql = CommunicationModel._build_task_sql('HCCLTaskSingleDevice')
        self.assertEqual(sql, "select * from HCCLTaskSingleDevice where timestamp < ? and timestamp >= ?")

    def test_group_tasks_by_op_groups_by_op_id_iter_id(self):
        tasks = [
            HcclTask(op_id=1, iter_id=0),
            HcclTask(op_id=1, iter_id=0),
            HcclTask(op_id=2, iter_id=0),
            HcclTask(op_id=1, iter_id=1),
        ]
        hccl_op_rows = [
            (1, 0, 'op_a', 'g1', 100, 200),
            (2, 0, 'op_b', 'g2', 300, 400),
            (1, 1, 'op_a_1', 'g1', 500, 600),
        ]
        model = self._make_model()
        with mock.patch.object(DBManager, 'judge_table_exist', return_value=True), \
                mock.patch.object(DBManager, 'fetch_all_data', side_effect=[hccl_op_rows, []]):
            result = model._group_tasks_by_op(tasks)

        self.assertEqual(set(result.keys()), {(1, 0), (2, 0), (1, 1)})
        self.assertEqual(result[(1, 0)].op_name, 'op_a')
        self.assertEqual(result[(1, 0)].group_name, 'g1')
        self.assertEqual(result[(1, 0)].start, 100)
        self.assertEqual(result[(1, 0)].end, 200)
        self.assertEqual(len(result[(1, 0)].tasks), 2)
        self.assertEqual(len(result[(2, 0)].tasks), 1)
        self.assertEqual(len(result[(1, 1)].tasks), 1)

    def test_group_tasks_by_op_skips_tasks_without_op(self):
        tasks = [
            HcclTask(op_id=1, iter_id=0),
            HcclTask(op_id=99, iter_id=0),  # 无对应 op 的 task 不合法，跳过
        ]
        model = self._make_model()
        with mock.patch.object(DBManager, 'judge_table_exist', return_value=True), \
                mock.patch.object(DBManager, 'fetch_all_data', side_effect=[[(1, 0, 'op_a', 'g1', 100, 200)], []]):
            result = model._group_tasks_by_op(tasks)

        self.assertEqual(list(result.keys()), [(1, 0)])
        self.assertEqual(len(result[(1, 0)].tasks), 1)

    def test_group_tasks_by_op_filters_by_top_hccl_ops(self):
        tasks = [
            HcclTask(op_id=1, iter_id=0),
            HcclTask(op_id=2, iter_id=0),
        ]
        hccl_op_rows = [
            (1, 0, 'op_a', 'g1', 100, 200),
            (2, 0, 'op_b', 'g2', 300, 400),
        ]
        model = self._make_model()
        with mock.patch.object(DBManager, 'judge_table_exist', return_value=True), \
                mock.patch.object(DBManager, 'fetch_all_data', side_effect=[hccl_op_rows, []]):
            result = model._group_tasks_by_op(tasks, top_hccl_ops=('op_a',))

        self.assertEqual(list(result.keys()), [(1, 0)])
        self.assertEqual(result[(1, 0)].op_name, 'op_a')


if __name__ == '__main__':
    unittest.main()
