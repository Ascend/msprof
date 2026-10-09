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

from msmodel.task_time.task_time import TaskTime

NAMESPACE = 'msmodel.task_time.task_time'


class TestTaskTime(unittest.TestCase):
    def test_init(self):
        check = TaskTime()
        result = (check.task_id, check.stream_id)
        self.assertEqual(result, (None, None))
        result = check.wait_time + check.model_id
        self.assertEqual(result, 0)

    def test_construct(self):
        with mock.patch(NAMESPACE + '.TaskTime._pre_check'):
            check = TaskTime()
            check.construct(1, 2, 3, 4, 5, 6, 7)
            self.assertEqual(check._task_id, 1)
