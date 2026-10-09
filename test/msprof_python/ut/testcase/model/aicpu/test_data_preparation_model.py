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

from msmodel.ai_cpu.data_preparation_model import DataPreparationModel

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msmodel.ai_cpu.data_preparation_model'


class TestDataPreparationModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            DataPreparationModel('test', ['test']).create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.DataPreparationModel.insert_data_to_db'):
            self.assertEqual(DataPreparationModel('test', ['test']).flush([]), None)

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.DataPreparationModel.insert_data_to_db'):
            DataPreparationModel('test', ['test']).flush_all({'test': []})


if __name__ == '__main__':
    unittest.main()
