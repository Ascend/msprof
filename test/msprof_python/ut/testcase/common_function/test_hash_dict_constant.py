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

from common_func.hash_dict_constant import HashDictData

NAMESPACE = 'common_func.file_manager'


class TestFileManager(unittest.TestCase):

    def test_load_hash_data(self):
        with mock.patch('common_func.singleton.singleton'), \
                mock.patch('os.path.exists',
                           return_value=True), \
                mock.patch('msmodel.ge.ge_hash_model.GeHashViewModel.get_type_hash_data',
                           return_value={'acl': {'1': 'test'}}), \
                mock.patch('msmodel.ge.ge_hash_model.GeHashViewModel.get_ge_hash_data',
                           return_value={'123': 'test_ge_hash'}):
            check = HashDictData('test')
            self.assertEqual(check.get_type_hash_dict(), {'acl': {'1': 'test'}})
            self.assertEqual(check.get_ge_hash_dict(), {'123': 'test_ge_hash'})
