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
from msparser.compact_info.node_basic_info_bean import NodeBasicInfoBean
from msparser.compact_info.node_basic_info_parser import NodeBasicInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.compact_info.node_basic_info_parser'


class TestNodeBasicInfoParser(unittest.TestCase):
    file_list = {
        DataTag.NODE_BASIC_INFO: [
            'aging.additional.node_basic_info.slice_0',
            'unaging.additional.node_basic_info.slice_0'
        ]
    }

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NodeBasicInfoParser.parse'), \
                mock.patch(NAMESPACE + '.NodeBasicInfoParser.save'):
            check = NodeBasicInfoParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NodeBasicInfoParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NodeBasicInfoParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NodeBasicInfoParser.save', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.NodeBasicInfoParser.parse'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NodeBasicInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = NodeBasicInfoParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        with mock.patch(NAMESPACE + '.NodeBasicInfoParser._transform_node_basic_info_data', return_value=[]):
            check = NodeBasicInfoParser(self.file_list, CONFIG)
            check._node_basic_info_data = [[1, 2, 3, 4, 5], [4, 5, 6, 7, 8]]
            check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.NodeBasicInfoParser.parse_bean_data', return_value=[]):
            check = NodeBasicInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_transform_node_basic_info_data(self):
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        data_dict = {
            'unaging_file':
                [
                    [
                        'node', '0', 320724, 4144668392374756, '18053128117395105485', 'AI_CORE',
                        '6456437209198804521', 2, 0, 0
                    ],
                    [
                        'node', '0', 320724, 4144668392565848, '18080335418676727411',
                        'AI_CORE', '7383439776149831', 2, 0, 0
                    ],
                ]
        }
        check = NodeBasicInfoParser(self.file_list, CONFIG)
        result = check._transform_node_basic_info_data(data_dict)
        self.assertEqual(
            result, [
                [
                    'node', '0', 320724, 4144668392374756, '18053128117395105485',
                    'AI_CORE', '6456437209198804521', 2, 0, 0, 0
                ],
                [
                    'node', '0', 320724, 4144668392565848, '18080335418676727411',
                    'AI_CORE', '7383439776149831', 2, 0, 0, 0
                ]
            ]
        )

    def test_get_node_basic_data(self):
        ctx_data = (23130, 10000, *(0,) * 11)
        struct_data = struct.pack("HHIIIQQIQIIQI", *ctx_data)
        data = NodeBasicInfoBean.decode(struct_data)
        check = NodeBasicInfoParser(self.file_list, CONFIG)
        result = check._get_node_basic_data(data)
        self.assertEqual(result, ['node', '0', 0, 0, '0', 'AI_CORE', '0', 0, 0, 0])


if __name__ == '__main__':
    unittest.main()
