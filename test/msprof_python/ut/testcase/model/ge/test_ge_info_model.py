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

from msmodel.ge.ge_info_model import GeModel

NAMESPACE = 'msmodel.ge.ge_info_model'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeModel.insert_data_to_db'):
            GeModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeModel('test', ['test']).get_ge_model_name()

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.GeModel.flush'):
            GeModel('test', ['test']).flush_all({'test': 'test'})
