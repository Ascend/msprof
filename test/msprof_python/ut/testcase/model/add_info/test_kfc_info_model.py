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

    def test_get_aicpu_master_stream_hccl_task_should_read_aicpu_batch_id_from_db_and_keep_batch_id(self: any) -> None:
        # DB 行：timestamp, aicpu_stream_id, aicpu_task_id, aicpu_batch_id(msparser 预计算落盘值),
        #        stream_id, task_id, batch_id(展开算子 batchId), type
        db_rows = [
            (100, 19, 0, 0, 52, 7, 5, 0),
            (200, 19, 1, 0, 52, 8, 5, 1),
        ]

        vm = KfcInfoViewModel('result_dir', [])
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch.object(vm, 'get_sql_data', return_value=db_rows) as mock_get_sql:
            result = vm.get_aicpu_master_stream_hccl_task()

        # SQL 直读 aicpu_batch_id（msparser 已用 DeviceTaskFlip 预计算并落盘），batch_id 同样从 DB 读取
        sql = mock_get_sql.call_args[0][0]
        self.assertIn('aicpu_batch_id', sql)
        self.assertIn('batch_id', sql)

        # 直读结果：aicpu_batch_id / batch_id / task_type 均保留 DB 值
        self.assertEqual(2, len(result))
        self.assertEqual(100, result[0].timestamp)
        self.assertEqual(19, result[0].aicpu_stream_id)
        self.assertEqual(0, result[0].aicpu_task_id)
        self.assertEqual(52, result[0].stream_id)
        self.assertEqual(7, result[0].task_id)
        self.assertEqual(0, result[0].aicpu_batch_id)
        self.assertEqual(5, result[0].batch_id)
        self.assertEqual(0, result[0].task_type)
        self.assertEqual(200, result[1].timestamp)
        self.assertEqual(19, result[1].aicpu_stream_id)
        self.assertEqual(1, result[1].aicpu_task_id)
        self.assertEqual(52, result[1].stream_id)
        self.assertEqual(8, result[1].task_id)
        self.assertEqual(0, result[1].aicpu_batch_id)
        self.assertEqual(5, result[1].batch_id)
        self.assertEqual(1, result[1].task_type)


if __name__ == '__main__':
    unittest.main()
