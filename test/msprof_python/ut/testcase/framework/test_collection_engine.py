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
import multiprocessing
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG, INFO_JSON
from framework.collection_engine import AI

NAMESPACE = 'framework.collection_engine'


class TestCollectionEngine(unittest.TestCase):

    def test_import_control_flow(self):
        with mock.patch(NAMESPACE + '.AI.data_analysis', side_effect=multiprocessing.ProcessError),\
                mock.patch(NAMESPACE + '.logging.error'),\
                mock.patch(NAMESPACE + '.logging.info'):
            check = AI(CONFIG)
            check.import_control_flow()

    def test_data_analysis(self):
        InfoConfReader()._info_json = {}
        with mock.patch(NAMESPACE + '.AI.formula_list'):
            check = AI(CONFIG)
            check.data_analysis()

    def test_data_analysis_2(self):
        InfoConfReader()._info_json = INFO_JSON
        with mock.patch(NAMESPACE + '.AI.formula_list'):
            check = AI(CONFIG)
            check.data_analysis()

    def test_project_preparation(self):
        project_dir = "test100"
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
             mock.patch(NAMESPACE + '.PathManager.get_sql_dir'), \
             mock.patch(NAMESPACE + '.check_dir_writable'), \
             mock.patch(NAMESPACE + '.AI._create_collection_log'), \
             mock.patch('os.path.exists', return_value=False), \
             mock.patch('os.makedirs'), \
             mock.patch('os.remove'), \
             mock.patch('os.path.join', return_value="test"), \
             mock.patch('os.listdir', return_value=['test.complete']):
            AI.project_preparation(project_dir)

    def test_create_collection_log(self):
        project_dir = "test100"
        with mock.patch(NAMESPACE + '.check_dir_writable'), \
             mock.patch(NAMESPACE + '.PathManager.get_log_dir'), \
             mock.patch('builtins.open', side_effect=OSError), \
             mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
             mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
             mock.patch(NAMESPACE + '.error'):
            AI._create_collection_log(project_dir)
