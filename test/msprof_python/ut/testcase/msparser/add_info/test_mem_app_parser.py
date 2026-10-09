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
import sqlite3
import struct
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from constant.constant import CONFIG
from msparser.add_info.memory_application_bean import MemoryApplicationBean
from msparser.add_info.memory_application_parser import MemoryApplicationParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.memory_application_parser'


class TestMemoryApplicationParser(unittest.TestCase):
    file_list = {
        DataTag.MEMORY_APPLICATION: [
            'aging.additional.memory_application.slice_0',
            'unaging.additional.memory_application.slice_0'
        ]
    }

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MemoryApplicationParser.parse'), \
                mock.patch(NAMESPACE + '.MemoryApplicationParser.save'):
            check = MemoryApplicationParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.MemoryApplicationParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = MemoryApplicationParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.MemoryApplicationParser.save', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.MemoryApplicationParser.parse'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = MemoryApplicationParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = MemoryApplicationParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        check = MemoryApplicationParser(self.file_list, CONFIG)
        check._memory_application_data = [[1, 2, 3, 4, 5], [4, 5, 6, 7, 8]]
        check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.MemoryApplicationParser.parse_bean_data', return_value=[]):
            check = MemoryApplicationParser(self.file_list, CONFIG)
            check.parse()

    def test_get_memory_application_data(self):
        ctx_data = (23130, 10000, *(0,) * 9, b'a', b'0')
        struct_data = struct.pack("HHIIIQQQQQQss", *ctx_data)
        data = MemoryApplicationBean.decode(struct_data)
        check = MemoryApplicationParser(self.file_list, CONFIG)
        result = check._get_memory_application_data(data)
        self.assertEqual(result, ['node', '0', 0, 0, '0', '0', '0', '0', '0', "b'a'", b'0'])


if __name__ == '__main__':
    unittest.main()
