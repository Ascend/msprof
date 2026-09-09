#!/usr/bin/env python
# coding=utf-8
# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
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

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from framework.prof_factory_maker import ProfFactoryMaker
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'framework.prof_factory_maker'


class TestProfFactoryMaker(unittest.TestCase):
    def test_create_parser_factory(self):
        file_list = {DataTag.ACL: ['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0'],
                     DataTag.ACL_HASH: ['AclModule.hash_dic.0.slice_0']}
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model'), \
             mock.patch(NAMESPACE + '.ParserFactory.run'):
            check = ProfFactoryMaker(CONFIG)
            check.create_parser_factory(file_list)
        with mock.patch(NAMESPACE + '.logging.warning'):
            check = ProfFactoryMaker(CONFIG)
            check.create_parser_factory(False)

    def test_create_calculator_factory(self):
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model'), \
             mock.patch(NAMESPACE + '.CalculatorFactory.run'):
            check = ProfFactoryMaker(CONFIG)
            check.create_calculator_factory([])
            check.create_calculator_factory(['a'])

    def test_get_chip_model(self):
        check = ProfFactoryMaker(CONFIG)
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "1"}
        ret = check._get_chip_model()
        self.assertEqual(ChipModel.CHIP_V2_1_0, ret)

    def test_prof_factory_should_inject_chip_model_without_mutating_input(self):
        sample_config = {'result_dir': 'test_result'}
        file_list = {DataTag.ACL: ['AclModule.acl_model.0.slice_0']}
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model',
                        return_value=ChipModel.CHIP_V6_1_0), \
                mock.patch(NAMESPACE + '.ParserFactory') as parser_factory:
            ProfFactoryMaker(sample_config).create_parser_factory(file_list)

        self.assertNotIn('chip_model', sample_config)
        parser_config = parser_factory.call_args[0][1]
        self.assertEqual('15', parser_config['chip_model'])
        parser_factory.return_value.run.assert_called_once_with()

    def test_factory_should_not_special_case_unsupported_ccu(self):
        file_list = {DataTag.CCU_MISSION: ['ccu0.instr.0.slice_0']}
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model', return_value=ChipModel.CHIP_V5_1_0), \
                mock.patch(NAMESPACE + '.ParserFactory.run'), \
                mock.patch(NAMESPACE + '.logging.error') as logging_error:
            ProfFactoryMaker(CONFIG).create_parser_factory(file_list)

        logging_error.assert_not_called()

    def test_host_add_info_should_not_require_a_hardware_profile(self):
        file_list = {DataTag.CCU_TASK: ['unaging.additional.ccu_task_info.slice_0']}
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model', return_value=ChipModel.CHIP_V5_1_0), \
                mock.patch(NAMESPACE + '.ParserFactory.run'), \
                mock.patch(NAMESPACE + '.logging.error') as logging_error:
            ProfFactoryMaker(CONFIG).create_parser_factory(file_list)

        logging_error.assert_not_called()

    def test_factory_should_leave_ccu_selection_to_parser_config(self):
        file_list = {
            DataTag.CCU_MISSION: ['ccu0.instr.0.slice_0'],
            DataTag.CCU_CHANNEL: ['ccu0.stat.0.slice_0'],
            DataTag.ACL: ['AclModule.acl_model.0.slice_0']
        }
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model',
                        return_value=ChipModel.CHIP_V6_1_0), \
                mock.patch(NAMESPACE + '.ParserFactory') as parser_factory, \
                mock.patch('msinterface.msprof_c_interface.dump_device_data') as native_parser:
            self.assertIsNone(ProfFactoryMaker(CONFIG).create_parser_factory(file_list))

        self.assertIs(parser_factory.call_args[0][0], file_list)
        parser_factory.return_value.run.assert_called_once_with()
        native_parser.assert_not_called()
