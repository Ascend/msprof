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

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from msmodel.compact_info.task_track_model import TaskTrackModel

NAMESPACE = 'msmodel.compact_info.task_track_model'


class TestTaskTrackModel(unittest.TestCase):
    def test_construct(self):
        check = TaskTrackModel('test', [DBNameConstant.TABLE_TASK_TRACK])
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_RTS_TRACK)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_TASK_TRACK])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TaskTrackModel.insert_data_to_db'):
            check = TaskTrackModel('test', [DBNameConstant.TABLE_TASK_TRACK])
            check.flush([0, 75838889645892, 'CREATE_STREAM', 0, 0, 270772, 0, 'task_track', 'runtime', 12])
