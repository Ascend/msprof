#!/usr/bin/python3
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

from constant.constant import CONFIG
from msparser.compact_info.memcpy_info_bean import MemcpyInfoBean
from msparser.compact_info.memcpy_info_parser import MemcpyInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.compact_info.memcpy_info_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.memcpy_info_model'
GE_HASH_MODEL_NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestMemcpyInfoParser(unittest.TestCase):
    file_list = {DataTag.MEMCPY_INFO: ['aging.compact.memcpy_info.slice_0', 'unaging.compact.memcpy_info.slice_0']}

    def test_reformat_data(self):
        hash_key = {
            'runtime': {
                '999': 'memcpy_info',
            }
        }
        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data', return_value=hash_key), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = MemcpyInfoParser(self.file_list, CONFIG)
            bean_data = MemcpyInfoBean([23130, 5000, 999, 270722, 16, 75838889645892, 12, 0])
            data = check.reformat_data([bean_data])
            self.assertEqual(len(data), 1)
            self.assertEqual(len(data[0]), 7)
            self.assertEqual(data[0][0], '999')

    def test_parse(self):
        with mock.patch(NAMESPACE + '.MemcpyInfoParser.parse_bean_data'), \
                mock.patch(NAMESPACE + '.MemcpyInfoParser.reformat_data'):
            check = MemcpyInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.MemcpyInfoModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.MemcpyInfoModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.MemcpyInfoModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.MemcpyInfoModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.MemcpyInfoModel.finalize'):
            check = MemcpyInfoParser(self.file_list, CONFIG)
            check._task_track_data = ['memcpy_info', 'runtime', 270722, 16, 75838931021372, 125544, 1]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MemcpyInfoParser.parse'), \
                mock.patch(NAMESPACE + '.MemcpyInfoParser.save'):
            MemcpyInfoParser(self.file_list, CONFIG).ms_run()
        MemcpyInfoParser({DataTag.MEMCPY_INFO: []}, CONFIG).ms_run()
