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

import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.compact_info.node_attr_info_bean import NodeAttrInfoBean
from msparser.compact_info.node_attr_info_parser import NodeAttrInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.compact_info.node_attr_info_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.node_attr_info_model'
GE_HASH_MODEL_NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestNodeAttrInfoParser(unittest.TestCase):
    file_list = {
        DataTag.NODE_ATTR_INFO: [
            'aging.compact.node_attr_info.slice_0',
            'unaging.compact.node_attr_info.slice_0'
        ]
    }

    def test_parse(self):
        with mock.patch(NAMESPACE + '.NodeAttrInfoParser.parse_bean_data'):
            check = NodeAttrInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.NodeAttrInfoModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.NodeAttrInfoModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.NodeAttrInfoModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.NodeAttrInfoModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.NodeAttrInfoModel.finalize'):
            check = NodeAttrInfoParser(self.file_list, CONFIG)
            check._task_track_data = [
                'node', 'node_attr_info', 3602526, 'aclnnInplaceCopy_TransposeAiCore_Transpose', 38202864071026,
                'strides: 0; pads: 0; dilations: 0; groups: 1;data_formats: 0'
            ]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NodeAttrInfoParser.parse'), \
                mock.patch(NAMESPACE + '.NodeAttrInfoParser.save'):
            NodeAttrInfoParser(self.file_list, CONFIG).ms_run()
        NodeAttrInfoParser({DataTag.NODE_ATTR_INFO: []}, CONFIG).ms_run()

    def test_get_node_attr_info_data(self):
        node_attr_info_data = (
            23130, 10000, 9, 915562, 20, 1147447818475641, 5800791449677985058, 0, 1172856012119002644, *(0,) * 3
        )
        struct_data = struct.pack("HHIIIQQIQIQQ", *node_attr_info_data)
        data = NodeAttrInfoBean.decode(struct_data)
        check = NodeAttrInfoParser(self.file_list, CONFIG)
        result = check._get_node_attr_info_data(data)
        self.assertEqual(result, ['node', '9', 915562, 1147447818475641,
                                  5800791449677985058, 3965646057194389504])

    def test_transform_node_attr_info_data(self):
        level = 'node'
        type_hash_dict = {
            level: {
                '9': 'node_attr_info',
            }
        }
        ge_hash_dict = {
            '14943139970160221335': 'src_format:ND|dst_format:FRACTAL_NZ|src_subformat:0|dst_subformat:0|groups:0|',
            '6371697640902395132': 'aclnnMm_TransData_TransData',
        }
        hashid = '14943139970160221335'
        data_list = [
            [level, '9', 3602526, 38202863941896, 6371697640902395132, hashid],
            [level, '9', 3602526, 38202864071026, 6371697640902395132, hashid]
        ]
        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data',
                           return_value=type_hash_dict), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_ge_hash_data',
                           return_value=ge_hash_dict), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = NodeAttrInfoParser(self.file_list, CONFIG)
            data = check._transform_node_attr_info_data(data_list)
            self.assertEqual(len(data), 2)
            self.assertEqual(len(data[0]), 6)
            self.assertEqual(data[0][1], '9')
            self.assertEqual(data[0][4], '6371697640902395132')
            self.assertEqual(data[0][5], hashid)
