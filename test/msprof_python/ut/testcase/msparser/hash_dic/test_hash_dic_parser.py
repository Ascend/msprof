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
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from constant.constant import CONFIG
from msparser.hash_dic.hash_dic_parser import HashDicParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hash_dic.hash_dic_parser'


class TestHashDicParser(unittest.TestCase):
    file_list = {
        DataTag.HASH_DICT: ['aging.additional.type_info_dic.data.slice_0', 'unaging.additional.hash_dic.data.slice_0']
    }

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.HashDicParser.parse'), \
                mock.patch(NAMESPACE + '.HashDicParser.save'):
            check = HashDicParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.HashDicParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HashDicParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = HashDicParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        with mock.patch('msmodel.ge.ge_hash_model.GeHashModel.insert_data_to_db'):
            check = HashDicParser(self.file_list, CONFIG)
            check._hash_data = {'ge_hash': [(1, 'test')], 'type_hash': []}
            check.save()

    def test_parse(self):
        data_list = ('aging.additional.type_info_dic.data.slice_0', 'unaging.additional.hash_dic.data.slice_0')
        with mock.patch(NAMESPACE + '.HashDicParser._read_ge_hash_data'), \
                mock.patch(NAMESPACE + '.HashDicParser._read_type_hash_data'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', side_effect=data_list):
            check = HashDicParser(self.file_list, CONFIG)
            check.parse()

    def test_read_ge_hash_data(self):
        data = """7634890245309224580:resnet50\n10401956421971378586:ArgMaxD\n10401956421971378587"""
        with mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            check = HashDicParser(self.file_list, CONFIG)
            check._read_ge_hash_data('test')

    def test_read_type_hash_data(self):
        data = """20000_1:TypeOp\n20000_5:AclmdlExecute\n20000"""
        with mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            check = HashDicParser(self.file_list, CONFIG)
            check._read_type_hash_data('test')


if __name__ == '__main__':
    unittest.main()
