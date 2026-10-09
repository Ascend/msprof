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
import os
import unittest
from unittest import mock

from msmodel.ai_cpu.data_preparation_view_model import DataPreparationViewModel

NAMESPACE = 'msmodel.ai_cpu.data_preparation_view_model'


class TestDataPreparationViewModel(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_DataPreparationViewModel')

    def test_get_host_queue(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            DataPreparationViewModel(self.DIR_PATH).get_host_queue()

    def test_get_host_queue_mode(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True),\
                mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]):
            DataPreparationViewModel(self.DIR_PATH).get_host_queue_mode()

    def test_get_data_queue(self):
        with mock.patch(NAMESPACE + '.DataPreparationViewModel.get_all_data', return_value=True):
            DataPreparationViewModel(self.DIR_PATH).get_data_queue()


if __name__ == '__main__':
    unittest.main()