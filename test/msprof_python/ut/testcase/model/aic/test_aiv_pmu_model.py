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

from msmodel.aic.aiv_pmu_model import AivPmuModel
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson

NAMESPACE = 'msmodel.aic.aiv_pmu_model'


class TestAivPmuModel(unittest.TestCase):

    @staticmethod
    def setup_class():
        InfoJson(devices='0').apply()

    def test_init(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.AivPmuModel.create_table'):
            check = AivPmuModel('test')
            check.init()

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.AivPmuModel.clear'), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.create_metric_table'):
            check = AivPmuModel('test')
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AivPmuModel.insert_data_to_db'):
            check = AivPmuModel('test')
            check.flush([])

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            check = AivPmuModel('test')
            check.clear()
