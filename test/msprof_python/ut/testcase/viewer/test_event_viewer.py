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
from profiling_bean.db_dto.api_data_dto import ApiDataDto
from viewer.event_viewer import EventViewer
from constant.info_json_construct import InfoJson

NAMESPACE = 'viewer.event_viewer'


class TestEventViewer(unittest.TestCase):

    def test_get_timeline_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        check = EventViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual([], ret)

    def test_get_timeline_data_should_return_empty_when_model_init_ok(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        with mock.patch(NAMESPACE + '.EventDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".EventDataViewModel.get_timeline_data", return_value=[]):
            check = EventViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual([], ret)

    def test_get_timeline_data_should_return_timeline_when_data_exist(self):
        config = {"headers": []}
        params = {
            "project": "test_event_view",
            "model_id": 1,
            "iter_id": 1
        }
        matched_event_dto = ApiDataDto()
        matched_event_dto.struct_type = "1"
        matched_event_dto.id = "2"
        matched_event_dto.start = 3
        matched_event_dto.end = 4
        matched_event_dto.thread_id = 5
        matched_event_dto.item_id = 6
        matched_event_dto.level = "7"
        matched_event_dto.request_id = 8
        matched_event_dto.connection_id = 0
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        InfoConfReader()._host_freq = 100
        expect = [
            OrderedDict([('name', 'process_name'), ('pid', 100), ('tid', 0), ('args', OrderedDict([('name', 'Event')])),
                         ('ph', 'M')]),
            OrderedDict([('name', 'thread_name'), ('pid', 100), ('tid', 5),
                         ('args', OrderedDict([('name', 'Thread 5')])), ('ph', 'M')]),
            OrderedDict([('name', 'thread_sort_index'), ('pid', 100), ('tid', 5),
                         ('args', OrderedDict([('sort_index', 5)])), ('ph', 'M')]),
            OrderedDict([('name', '1'), ('pid', 100), ('tid', 5), ('ts', '30010.000'), ('dur', 10000.0),
                         ('args', OrderedDict([('Thread Id', 5), ('level', '7'), ('id', '2'), ('item_id', 6),
                                               ('request_id', 8), ('connection_id', 0)])), ('ph', 'X')])
        ]

        with mock.patch(NAMESPACE + '.EventDataViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.EventDataViewModel.check_table', return_value=True), \
                mock.patch(NAMESPACE + ".EventDataViewModel.get_timeline_data", return_value=[matched_event_dto]):
            InfoJson(pid=100).apply()
            InfoConfReader()._local_time_offset = 10.0
            InfoConfReader()._host_local_time_offset = 10.0
            check = EventViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual(expect, ret)
