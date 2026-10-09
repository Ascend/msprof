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
from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.qos_model import QosModel

NAMESPACE = 'msmodel.hardware.qos_model'


class TestQosModel(unittest.TestCase):
    TABLE_LIST = [
        DBNameConstant.TABLE_QOS_BW
    ]
    TEMP_DIR = 'test'

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.QosModel.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = QosModel(TestQosModel.TEMP_DIR, DBNameConstant.DB_QOS, TestQosModel.TABLE_LIST)
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.QosModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = QosModel(TestQosModel.TEMP_DIR, DBNameConstant.DB_QOS, TestQosModel.TABLE_LIST)
            check.flush([])
