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
from msmodel.hardware.sys_mem_model import SysMemModel

NAMESPACE = 'msmodel.hardware.sys_mem_model'


class TestSysMemModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.SysMemModel.insert_data_to_db'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = SysMemModel('test', 'tscpu_0.db', ['TsOriginalData'])
            check.flush({'sys_mem_data': [1], 'pid_mem_data': [2]})
