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

from msmodel.add_info.kfc_info_model import KfcInfoViewModel

NAMESPACE = 'msmodel.add_info.kfc_info_model'


class TestKfcInfoViewModel(unittest.TestCase):

    def test_get_aicpu_master_stream_hccl_task_should_return_empty_when_table_missing(self: any) -> None:
        vm = KfcInfoViewModel('result_dir', [])
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            self.assertEqual([], vm.get_aicpu_master_stream_hccl_task())

    def test_get_aicpu_master_stream_hccl_task_should_compute_aicpu_batch_id_and_keep_db_batch_id(self: any) -> None:
        # DB 行：timestamp, aicpu_stream_id, aicpu_task_id, stream_id, task_id,
        #        aicpu_batch_id(0 占位), batch_id(展开算子 batchId), type
        db_rows = [
            (100, 19, 0, 52, 7, 0, 5, 0),
            (200, 19, 1, 52, 8, 0, 5, 1),
        ]
        captured = {}

        def fake_set_device_batch_id(data, result_dir):
            captured['intermediate'] = data
            captured['result_dir'] = result_dir
            # 模拟 flip 计算：把 HCCL_OP_MASTER_STREAM_TYPE 的 batch_id 槽位算成 aicpu_batch_id
            return [d.replace(batch_id=42) for d in data]

        vm = KfcInfoViewModel('result_dir', [])
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch.object(vm, 'get_sql_data', return_value=db_rows) as mock_get_sql, \
                mock.patch(NAMESPACE + '.FlipCalculator.set_device_batch_id',
                           side_effect=fake_set_device_batch_id):
            result = vm.get_aicpu_master_stream_hccl_task()

        # SQL 需包含 aicpu_batch_id 占位列，batch_id 从 DB 读取
        sql = mock_get_sql.call_args[0][0]
        self.assertIn('0 as aicpu_batch_id', sql)
        self.assertIn('batch_id', sql)

        # 中间类型：stream_id 槽位承载 aicpu_stream_id（flip 按 aicpu stream 计算）
        mid = captured['intermediate']
        self.assertEqual(19, mid[0].stream_id)
        self.assertEqual(0, mid[0].task_id)
        self.assertEqual(52, mid[0].hccl_stream_id)
        self.assertEqual(7, mid[0].hccl_task_id)
        self.assertEqual(5, mid[0].hccl_batch_id)
        self.assertEqual('result_dir', captured['result_dir'])

        # 最终类型：aicpu_batch_id 来自 set_device_batch_id，batch_id 保留 DB 值
        self.assertEqual(2, len(result))
        self.assertEqual(42, result[0].aicpu_batch_id)
        self.assertEqual(5, result[0].batch_id)
        self.assertEqual(0, result[0].task_type)
        self.assertEqual(42, result[1].aicpu_batch_id)
        self.assertEqual(5, result[1].batch_id)
        self.assertEqual(1, result[1].task_type)


if __name__ == '__main__':
    unittest.main()
