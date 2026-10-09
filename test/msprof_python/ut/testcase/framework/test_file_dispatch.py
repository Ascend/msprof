#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from framework.file_dispatch import FileDispatch

NAMESPACE = 'framework.file_dispatch'


class TestFileDispatch(unittest.TestCase):
    def test_dispatch(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
             mock.patch('os.listdir', return_value=['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0']), \
             mock.patch(NAMESPACE + '.ProfFactoryMaker.create_parser_factory'), \
             mock.patch(NAMESPACE + '.ProfFactoryMaker.create_calculator_factory'):
            check = FileDispatch(CONFIG)
            check.pick_up_files()

    def test_dispatch_parser(self):
        with mock.patch(NAMESPACE + '.FileDispatch.pick_up_files'), \
                mock.patch('framework.prof_factory_maker.ProfFactoryMaker.create_parser_factory'):
            check = FileDispatch(CONFIG)
            check._file_list = {'a': 'b'}
            check.dispatch_parser()

    def test_dispatch_calculator(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.FileDispatch.pick_up_files'), \
                mock.patch('framework.prof_factory_maker.ProfFactoryMaker.create_parser_factory'):
            check = FileDispatch(CONFIG)
            check._file_list = {'a': 'b'}
            check.dispatch_calculator()
