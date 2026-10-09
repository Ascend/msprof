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

from constant.constant import ITER_RANGE
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.iter_rec.iter_rec_model'


class TestHwtsIterModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.HwtsIterModel.insert_data_to_db'):
            check = HwtsIterModel('test')
            check.flush([], "HwtsIter")

    def test_check_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('HwtsIter.db')
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = HwtsIterModel('test')
            result = check.check_table()
            self.assertFalse(result)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HwtsIterModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.check_table()
            self.assertTrue(result)
        db_manager.destroy(res)

    def test_get_task_offset_and_sum(self):
        db_manager = DBManager()
        create_sql = "CREATE TABLE if not exists HwtsIter(ai_core_num INTEGER, " \
                     "ai_core_offset INTEGER, " \
                     "task_count INTEGER, " \
                     "task_offset INTEGER, " \
                     "iter_id INTEGER," \
                     "model_id INTEGER," \
                     "index_id INTEGER)"
        data = [(1, 0, 3, 0, 1, 1, 1), (1, 1, 4, 3, 2, 1, 2)]
        insert_sql = "insert into HwtsIter values({value})".format(value="?," * (len(data[0]) - 1) + "?")
        res = db_manager.create_table('hwts.db', create_sql, insert_sql, data)
        check = HwtsIterModel('test')
        check.conn, check.cur = res[0], res[1]
        result = check.get_task_offset_and_sum(IterationRange(1, 1, 1), 'ai_core')
        self.assertEqual(result, (0, 1))
        res[1].execute('drop table if exists HwtsIter')
        check = HwtsIterModel('test')
        check.conn, check.cur = res[0], res[1]
        result = check.get_task_offset_and_sum(ITER_RANGE, 'ai_core')
        self.assertEqual(result, (0, 0))
        db_manager.destroy(res)

    def test_get_batch_list(self: any):
        table_name = "HwtsBatch"
        check = HwtsIterModel('test')
        check.init()

        with mock.patch("common_func.db_manager.DBManager.fetch_all_data", return_value=[0, 1]):
            res = check.get_batch_list(table_name, [1, 2])
        self.assertEqual(res, [0, 1])
