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

from msmodel.ge.ge_model_load_model import GeFusionModel

NAMESPACE = 'msmodel.ge.ge_model_load_model'


class TestGeHashModel(unittest.TestCase):
    def test_flush(self):
        with mock.patch(NAMESPACE + '.GeFusionModel.insert_data_to_db'):
            GeFusionModel('test', ['test']).flush([])

    def test_get_ge_hash_model_name(self):
        GeFusionModel('test', ['test']).get_ge_fusion_model_name()

    def test_flush_all(self):
        with mock.patch(NAMESPACE + '.GeFusionModel.flush'):
            GeFusionModel('test', ['test']).flush_all({'test': 'test'})
