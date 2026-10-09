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
from common_func.hash_dict_constant import HashDictData
from msparser.compact_info.hccl_op_info_bean import HcclOpInfoBean
from msparser.compact_info.hccl_op_info_parser import HcclOpInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.compact_info.hccl_op_info_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.hccl_op_info_model'
GE_HASH_MODEL_NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestHcclOpInfoParser(unittest.TestCase):
    file_list = {
        DataTag.HCCL_OP_INFO: [
            'aging.compact.hccl_op_info.slice_0',
            'unaging.compact.hccl_op_info.slice_0'
        ]
    }

    def test_parse(self):
        with mock.patch(NAMESPACE + '.HcclOpInfoParser.parse_bean_data'), \
                mock.patch(NAMESPACE + '.HcclOpInfoParser.reformat_data'):
            check = HcclOpInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.HcclOpInfoModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.HcclOpInfoModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.HcclOpInfoModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.HcclOpInfoModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.HcclOpInfoModel.finalize'):
            check = HcclOpInfoParser(self.file_list, CONFIG)
            check._hccl_op_info_data = [
                'node', 'hccl_op_info', 353695, 38202664081728, 1, 0, 'INT8', 'NB-RING', 30511, '72840854065026987'
            ]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.HcclOpInfoParser.parse'), \
                mock.patch(NAMESPACE + '.HcclOpInfoParser.save'):
            HcclOpInfoParser(self.file_list, CONFIG).ms_run()
        HcclOpInfoParser({DataTag.HCCL_OP_INFO: []}, CONFIG).ms_run()

    def test_reformat_data(self):
        data_list = [
            HcclOpInfoBean([23130, 10000, 10, 353695, 20, 38202863941896, 2, 3, 0b01000101, 43642, 72840854065026987]),
            HcclOpInfoBean([23130, 10000, 10, 353695, 20, 38202866941806, 0, 4, 0b00110001, 74162, 16426617625805244]),
            HcclOpInfoBean([23130, 10000, 10, 353695, 20, 38202863941896, 1, 2, 0, 61555, 9380863679156556794])
        ]
        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            HashDictData('./')._type_hash_dict = {
                'node': {
                    '10': 'hccl_op_info',
                }
            }
            check = HcclOpInfoParser(self.file_list, CONFIG)
            check.reformat_data(data_list)
            data = check._hccl_op_info_data
            self.assertEqual(len(data), 3)
            self.assertEqual(len(data[0]), 10)
            self.assertEqual(data[0][1], 'hccl_op_info')
            self.assertEqual(data[0][4:], [0, 1, 'FP16', '69', 43642, '72840854065026987'])
            self.assertEqual(data[1][4:], [0, 0, 'FP32', '49', 74162, '16426617625805244'])
            self.assertEqual(data[2][4:], [1, 0, 'INT32', '0', 61555, '9380863679156556794'])
