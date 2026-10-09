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

from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.sys_usage_model import SysUsageModel

NAMESPACE = 'msmodel.hardware.sys_usage_model'


class TestSysUsageModel(unittest.TestCase):

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = SysUsageModel('test', 'sys_usage_0.db', ['SysCpuUsage'])
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.SysUsageModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = SysUsageModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.flush({'sys_data_list': [1], 'pid_data_list': [2]})
