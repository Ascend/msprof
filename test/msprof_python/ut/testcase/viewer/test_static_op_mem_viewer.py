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

from common_func.msvp_constant import MsvpConstant
from viewer.static_op_mem_viewer import StaticOpMemViewer


class TestStaticOpMemViewer(unittest.TestCase):
    def test_get_summary_data_return_empty_data_with_get_none_summary_from_model(self):
        configs = {
            "headers": ['Op Name', 'Model Name', 'Graph ID', 'Node Index Start', 'Node Index End', 'Size(KB)']
        }
        param = {"project": "static_op_mem"}
        self.assertEqual(StaticOpMemViewer(configs, param).get_summary_data(), MsvpConstant.MSVP_EMPTY_DATA)
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=True), \
                mock.patch('msmodel.add_info.static_op_mem_viewer_model.StaticOpMemViewModel.get_summary_data',
                           return_value=[]):
            result = StaticOpMemViewer(configs, param).get_summary_data()
            self.assertEqual(result, MsvpConstant.MSVP_EMPTY_DATA)
        with mock.patch('msmodel.interface.base_model.BaseModel.check_table', return_value=True), \
                mock.patch('msmodel.add_info.static_op_mem_viewer_model.StaticOpMemViewModel.get_summary_data',
                           return_value=[(7104496996232435956, 0, 0, 1, 6, 1280032)]):
            StaticOpMemViewer(configs, param).get_summary_data()

        if __name__ == '__main__':
            unittest.main()
