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

from msmodel.interface.sql_helper import SqlWhereCondition


class TestSqlWhereCondition(unittest.TestCase):
    def test__get_interval_intersection_condition_should_return_different_condition_when_start_end_is_inf_or_not(self):
        model = SqlWhereCondition()
        table_name = "A"
        start_col = "start_time"
        end_col = "end_time"
        condition1 = model.get_interval_intersection_condition(float("-inf"), float("inf"),
                                                               table_name, start_col, end_col)
        self.assertEqual(condition1, "")

        condition2 = model.get_interval_intersection_condition(1000, float("inf"), table_name, start_col, end_col)
        self.assertEqual(condition2, "where not (A.end_time < 1000)")

        condition2 = model.get_interval_intersection_condition(float("-inf"), 2000, table_name, start_col, end_col)
        self.assertEqual(condition2, "where not (A.start_time > 2000)")

        condition2 = model.get_interval_intersection_condition(1000, 2000, table_name, start_col, end_col)
        self.assertEqual(condition2, "where not (A.end_time < 1000 or A.start_time > 2000)")

    def test__get_select_condition_return_model_iter_range_when_not_is_all(self):
        model = SqlWhereCondition()
        condition = model.get_host_select_condition(False, 1, 2, 0)
        self.assertEqual(condition, "where model_id=1 and (index_id=2 or index_id=0) and device_id=0")

    def test__get_select_condition_return_empty_when_is_all_and(self):
        model = SqlWhereCondition()
        condition = model.get_host_select_condition(True, 1, 2, 0)
        self.assertEqual(condition, "where device_id=0")
