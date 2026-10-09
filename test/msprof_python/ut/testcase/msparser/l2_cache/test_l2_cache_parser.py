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
function: test l2_cache_parser
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock
from unittest.mock import Mock

from msparser.l2_cache.l2_cache_parser import L2CacheParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.l2_cache import L2CacheDataBean

NAMESPACE = 'msparser.l2_cache.l2_cache_parser'
MODEL_NAMESPACE = 'msmodel.l2_cache.l2_cache_parser_model'
BEAN_NAMESPACE = 'profiling_bean.struct_info.l2_cache'


class TestL2CacheParser(unittest.TestCase):
    sample_config = {'l2CacheTaskProfiling': 'on',
                     'l2CacheTaskProfilingEvents': '0x78,0x79,0x77,0x71,0x6A,0x6C,0x74,0x62',
                     'result_dir': '/tmp/result',
                     'tag_id': 'JOBEJGBAHABDEEIJEDFHHFAAAAAAAAAA',
                     'host_id': '127.0.0.1'
                     }
    file_list = {DataTag.L2CACHE: ['l2_cache.data.0.slice_0']}

    def test_construct(self):
        check = L2CacheParser(self.file_list, self.sample_config)
        self.assertEqual(check._project_path, '/tmp/result')
        self.assertEqual(check._file_list, self.file_list)
        self.assertEqual(check._l2_cache_events, ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62'])


    def test_check_l2_cache_event_valid(self):
        check = L2CacheParser(self.file_list, self.sample_config)
        check._l2_cache_events = None
        self.assertEqual(check._check_l2_cache_event_valid(), False)
        check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0x62', '0x88']
        self.assertEqual(check._check_l2_cache_event_valid(), False)
        check._l2_cache_events = ['0x78', '0x79', '0x77', '0x71', '0x6a', '0x6c', '0x74', '0xmm']
        self.assertEqual(check._check_l2_cache_event_valid(), False)

        check = L2CacheParser(self.file_list, self.sample_config)
        self.assertEqual(check._check_l2_cache_event_valid(), True)

    def test_check_file_complete(self):
        with mock.patch('os.path.getsize', return_value=144):
            check = L2CacheParser(self.file_list, self.sample_config)
            self.assertEqual(check._check_file_complete("test"), 144)
        with mock.patch('os.path.getsize', return_value=143):
            check = L2CacheParser(self.file_list, self.sample_config)
            self.assertEqual(check._check_file_complete("test"), 143)

    def test_parse(self):
        data_bean = L2CacheDataBean()
        data = (0, 3, 26, 0, 991, 98, 0, 0, 505, 1089, 0, 106)
        data_bean.construct_bean(data)
        with mock.patch(NAMESPACE + '.L2CacheParser._check_file_complete', return_value=144), \
                mock.patch('builtins.open', mock.mock_open(read_data="")), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=100), \
                mock.patch('common_func.file_manager.check_dir_writable'), \
                mock.patch('os.access', return_value=True), \
                mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=0)), \
                mock.patch(BEAN_NAMESPACE + '.L2CacheDataBean.decode', return_value=data_bean):
            check = L2CacheParser(self.file_list, self.sample_config)
            check.parse()
        with mock.patch(NAMESPACE + '.L2CacheParser._check_file_complete', return_value=0):
            check = L2CacheParser(self.file_list, self.sample_config)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.flush'), \
                mock.patch(MODEL_NAMESPACE + '.L2CacheParserModel.finalize'):
            check = L2CacheParser(self.file_list, self.sample_config)
            check._l2_cache_data = [1]
            check.save()

    def test_ms_run(self):
        check = L2CacheParser(self.file_list, self.sample_config)
        check._file_list = None
        check.ms_run()
        with mock.patch(NAMESPACE + '.L2CacheParser._check_l2_cache_event_valid', return_value=True), \
                mock.patch(NAMESPACE + '.L2CacheParser.parse'), \
                mock.patch(NAMESPACE + '.L2CacheParser.save'):
            check = L2CacheParser(self.file_list, self.sample_config)
            check.ms_run()
