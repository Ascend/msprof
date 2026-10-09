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
import struct
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from constant.constant import CONFIG
from msparser.add_info.hccl_info_bean import HcclInfoBean
from msparser.add_info.hccl_info_parser import HcclInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.hccl_info_parser'


class TestHcclInfoParser(unittest.TestCase):
    file_list = {
        DataTag.HCCL_INFO: ['aging.additional.hccl_info.slice_0', 'unaging.additional.hccl_info.slice_0']
    }

    def test_parse(self):
        ctx_data = (23130, 5000, 1113, 320736, 0, 0, 0, *(0,) * 41)
        struct_data = struct.pack("=HHIIIQI25I16Q", *ctx_data)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.HcclInfoParser.get_file_path_and_check', return_value=True), \
                mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info', return_value=True):
            with mock.patch('os.path.getsize', return_value=256), \
                    mock.patch('builtins.open', mock.mock_open(read_data=struct_data)):
                check = HcclInfoParser(self.file_list, CONFIG)
                check.parse()

    def test_save(self):
        check = HcclInfoParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        check = HcclInfoParser(self.file_list, CONFIG)
        check._hccl_info_data = [
            HcclInfoBean((0, 5000, 0, 0, 0, 0, 0, *(0,) * 32)),
            HcclInfoBean((0, 5000, 0, 0, 0, 0, 0, *(0,) * 32))
        ]
        check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.HcclInfoParser.parse'), \
                mock.patch(NAMESPACE + '.HcclInfoParser.save'):
            check = HcclInfoParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.HcclInfoParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = HcclInfoParser(self.file_list, CONFIG)
            check.ms_run()


if __name__ == '__main__':
    unittest.main()
