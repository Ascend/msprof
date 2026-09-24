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
Copyright Huawei Technologies Co., Ltd. 2023. All rights reserved.
"""
import unittest
from unittest import mock
from collections import OrderedDict

from common_func.info_conf_reader import InfoConfReader
from constant.constant import UT_CONFIG_FILE_PATH
from profiling_bean.db_dto.api_data_dto import ApiDataDto, ApiDataDtoTuple
from viewer.api_viewer import ApiViewer
from constant.info_json_construct import InfoJson

NAMESPACE = 'viewer.api_viewer'


class TestApiViewer(unittest.TestCase):

    def test_get_timeline_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        check = ApiViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual([], ret)

    def test_get_timeline_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.ApiDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_timeline_data", return_value=[]):
            InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
            InfoConfReader()._end_info = {}
            check = ApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual([], ret)

    def test_get_timeline_data_should_return_timeline_when_data_exist(self):
        config = {"headers": []}
        params = {
            "project": "test_api_view",
            "model_id": 1,
            "iter_id": 1
        }
        expect = [OrderedDict([('name', 'process_name'), ('pid', 100), ('tid', 0),
                               ('args', OrderedDict([('name', 'Api')])), ('ph', 'M')]),
                  OrderedDict([('name', 'thread_name'), ('pid', 100), ('tid', 456),
                               ('args', OrderedDict([('name', 'Thread 456')])), ('ph', 'M')]),
                  OrderedDict([('name', 'thread_name'), ('pid', 100), ('tid', 123),
                               ('args', OrderedDict([('name', 'Thread 123')])), ('ph', 'M')]),
                  OrderedDict([('name', 'thread_sort_index'), ('pid', 100), ('tid', 456),
                               ('args', OrderedDict([('sort_index', 456)])), ('ph', 'M')]),
                  OrderedDict([('name', 'thread_sort_index'), ('pid', 100), ('tid', 123),
                               ('args', OrderedDict([('sort_index', 123)])), ('ph', 'M')]),
                  OrderedDict([('name', 'ModelLoad'), ('pid', 100), ('tid', 123), ('ts', '10000010.000'),
                               ('dur', 2000000.0), ('args', OrderedDict([('Thread Id', 123), ('Mode', 'ModelLoad'),
                                                                         ('level', 'model'), ('id', 0), ('item_id', 7),
                                                                         ('connection_id', 1)])), ('ph', 'X')]),
                  OrderedDict([('name', 'launch'), ('pid', 100), ('tid', 123), ('ts', '11000010.000'),
                               ('dur', 1000000.0), ('args', OrderedDict([('Thread Id', 123), ('Mode', 'launch'),
                                                                         ('level', 'node'), ('id', '0'),
                                                                         ('item_id', 'GatherV2_1'),
                                                                         ('connection_id', 2)])), ('ph', 'X')]),
                  OrderedDict([('name', 'launch'), ('pid', 100), ('tid', 456), ('ts', '13000010.000'),
                               ('dur', 2500000.0), ('args', OrderedDict([('Thread Id', 456), ('Mode', 'launch'),
                                                                         ('level', 'node'), ('id', '0'),
                                                                         ('item_id', 'GatherV2_2'),
                                                                         ('connection_id', 3)])), ('ph', 'X')]),
                  OrderedDict([('name', 'launch'), ('pid', 100), ('tid', 123), ('ts', '200000010.000'),
                               ('dur', 30000000.0), ('args', OrderedDict([('Thread Id', 123), ('Mode', 'launch'),
                                                                          ('level', 'node'), ('id', '0'),
                                                                          ('item_id', 'GatherV2_3'),
                                                                          ('connection_id', 4)])), ('ph', 'X')]),
                  OrderedDict([('name', 'ModelExecute'), ('pid', 100), ('tid', 123), ('ts', '300000010.000'),
                               ('dur', 200000000.0), ('args', OrderedDict([('Thread Id', 123), ('Mode', 'ModelExecute'),
                                                                           ('level', 'model'), ('id', 0),
                                                                           ('item_id', 7),
                                                                           ('connection_id', 5)])), ('ph', 'X')])]

        # 共计6条数据，4 api， 2 event
        # 首条记录不在start时间范围内，也不在model 时间范围内将被过滤。其余数据正常按时间进行modelLoad的筛选
        api_datas = [('ACL_RTS', 1, 20, 123, 'acl', 'aclrtSynchronizeStream', '0', 0),
                     ('launch', 1100, 100, 123, 'node', '0', 'GatherV2_1', 2),
                     ('launch', 1300, 250, 456, 'node', '0', 'GatherV2_2', 3),
                     ('launch', 20000, 3000, 123, 'node', '0', 'GatherV2_3', 4)]

        matched_event_dto1 = ApiDataDtoTuple(struct_type="ModelLoad", level="model", thread_id=123, start=1000,
                                             end=1200, item_id=7, request_id=3, connection_id=1, id=0)

        matched_event_dto2 = ApiDataDtoTuple(struct_type="ModelExecute", level="model", thread_id=123, start=30000,
                                             end=50000, item_id=7, request_id=3, connection_id=5, id=0)

        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        InfoConfReader()._host_freq = 100
        with mock.patch(NAMESPACE + '.ApiDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_timeline_data",
                           return_value=api_datas), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".EventDataViewModel.get_timeline_data", return_value=[matched_event_dto1,
                                                                                              matched_event_dto2]):
            # 清空DeviceInfo内容，info_conf_reader根据DeviceInfo判定是否为host
            InfoJson(pid=100, DeviceInfo=[]).apply()
            InfoConfReader()._local_time_offset = 10.0
            InfoConfReader()._host_local_time_offset = 10.0
            InfoConfReader()._start_info = {"collectionTimeBegin": "50000"}
            check = ApiViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual(expect, ret)
            # 还原InfoJsonReaderManager数据，避免干扰其他用例
            InfoJson().apply()
