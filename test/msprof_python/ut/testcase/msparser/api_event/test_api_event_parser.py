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
import struct
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from constant.constant import CONFIG
from msparser.api_event.api_event_parser import ApiEventParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.api_event.api_event_parser'


class TestApiEventParser(unittest.TestCase):
    file_list = {
        DataTag.API_EVENT: ['aging.api_event.slice_0', 'unaging.api_event.slice_0'],
    }

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ApiEventParser.parse'), \
                mock.patch(NAMESPACE + '.ApiEventParser.save'):
            check = ApiEventParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.ApiEventParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ApiEventParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = ApiEventParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        with mock.patch('msmodel.api.api_data_model.ApiDataModel.flush'), \
                mock.patch('msmodel.event.event_data_model.EventDataModel.flush'):
            check = ApiEventParser(self.file_list, CONFIG)
            check._api_data = [[1, 2, 3, 4, 5], [4, 5, 6, 7, 8]]
            check._event_data = [[1, 2, 3, 4, 5], [4, 5, 6, 7, 8]]
            check.save()

    def test_parse(self):
        struct_data = struct.pack('=HHIIIQQQHHIIIQQQ',
                                  23130, 5000, 1113, 94827, 0, 3095879082697020, 3095879082697020, 0,
                                  23130, 20000, 458759, 94815, 0, 0, 3095879086912038, 0)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.ApiEventParser.get_file_path_and_check', return_value=True), \
                mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info', return_value=True):
            with mock.patch('os.path.getsize', return_value=80), \
                    mock.patch('builtins.open', mock.mock_open(read_data=struct_data)):
                check = ApiEventParser(self.file_list, CONFIG)
                check.parse()
                # 过滤一条开始时间为0的异常数据,数据按self.file_list中aging/unaging分别落盘两次
                self.assertEqual(len(check._api_data), 2)


if __name__ == '__main__':
    unittest.main()
