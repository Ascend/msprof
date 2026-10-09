# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
import unittest
from unittest import mock

from msmodel.add_info.runtime_op_info_model import RuntimeOpInfoModel, RuntimeOpInfoViewModel
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto

NAMESPACE = 'msmodel.add_info.runtime_op_info_model'


class TestRuntimeOpInfoModel(unittest.TestCase):

    def test_flush_for_parse_model(self):
        with mock.patch(NAMESPACE + '.RuntimeOpInfoModel.insert_data_to_db'):
            self.assertEqual(RuntimeOpInfoModel('test').flush([]), None)

    def test_get_runtime_op_info_data_for_view_model(self):
        with mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=False):
            self.assertEqual(RuntimeOpInfoViewModel('test').get_runtime_op_info_data(), {})

        with mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
            mock.patch('common_func.db_manager.DBManager.fetch_all_data', return_value=[RuntimeOpInfoDto()]):
            self.assertEqual(RuntimeOpInfoViewModel('test').get_runtime_op_info_data(), {(0, 0, 0): RuntimeOpInfoDto()})


if __name__ == '__main__':
    unittest.main()
