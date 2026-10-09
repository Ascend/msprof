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

from msmodel.interface.parser_model import ParserModel

NAMESPACE = 'msmodel.interface.parser_model'


class DemoClass(ParserModel):
    def flush(self: any, data_list: list):
        pass


class TestBaseModel(unittest.TestCase):

    def test_init0(self):
        expect_res = True
        check = DemoClass('test', 'test', 'test')
        with mock.patch(NAMESPACE + '.ParserModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ParserModel.create_table'):
            res = check.init()
        self.assertEqual(expect_res, res)

    def test_init1(self):
        expect_res = False
        check = DemoClass('test', 'test', 'test')
        with mock.patch(NAMESPACE + '.ParserModel.init', return_value=False):
            res = check.init()
        self.assertEqual(expect_res, res)
