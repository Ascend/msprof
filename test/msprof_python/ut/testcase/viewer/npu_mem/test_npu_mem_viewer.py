#!/usr/bin/python3
# -*- coding: utf-8 -*-
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

import unittest
from unittest import mock
from collections import OrderedDict

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from msparser.npu_mem.npu_mem_dto import NpuMemDto
from viewer.npu_mem.npu_mem_viewer import NpuMemViewer

NAMESPACE = 'viewer.npu_mem.npu_mem_viewer'


class TestNpuMemViewer(unittest.TestCase):
    def test_get_summary_data_should_return_empty_when_model_init_fail(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {"project": "test_npu_mem_view", "model_id": 1, "iter_id": 1}
        check = NpuMemViewer(config, params)
        ret = check.get_summary_data()
        self.assertEqual(MsvpConstant.MSVP_EMPTY_DATA, ret)

    def test_get_summary_data_should_return_success_when_model_init_ok(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {"project": "test_npu_mem_view", "model_id": 1, "iter_id": 1}
        npu_mem_dto = NpuMemDto()
        npu_mem_dto.event = '1'
        npu_mem_dto.hbm = 0
        npu_mem_dto.ddr = 0
        npu_mem_dto.memory = 0
        npu_mem_dto.timestamp = 5  # 数据库中存储的是纳秒值
        InfoConfReader()._local_time_offset = 10.0
        with (
            mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.get_summary_data', return_value=[npu_mem_dto]),
        ):
            check = NpuMemViewer(config, params)
            ret = check.get_summary_data()
            # 数据库中的 timestamp 已经是纳秒值，viewer 会转换为微秒: 5ns / 1000 = 0.005us
            self.assertEqual(
                (["event", "ddr", "hbm", "timestamp", "memory"], [['Device', 0.0, 0.0, 0.0, '0.005\t']], 1), ret
            )

    def test_get_timeline_data_should_return_empty_when_db_check_fail(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {"project": "test_npu_mem_view", "model_id": 1, "iter_id": 1}
        check = NpuMemViewer(config, params)
        ret = check.get_timeline_data()
        self.assertEqual([], ret)

    def test_get_timeline_data_should_return_empty_when_db_check_ok(self):
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {"project": "test_npu_mem_view", "model_id": 1, "iter_id": 1}
        with (
            mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.get_timeline_data', return_value=[]),
        ):
            check = NpuMemViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual([], ret)

    def test_get_timeline_data_should_return_empty_when_data_exist(self):
        InfoConfReader()._info_json = {"devices": '0'}
        InfoConfReader()._local_time_offset = 10.0
        config = {"headers": ["event", "ddr", "hbm", "timestamp", "memory"]}
        params = {"project": "test_npu_mem_view", "model_id": 1, "iter_id": 1}
        NpuMemDtoTuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(NpuMemDto, [])
        npu_mem_dto = NpuMemDtoTuple("0", 0, 0, 0, 6)  # 数据库中存储的是纳秒值
        expect = [
            OrderedDict(
                [
                    ('name', 'process_name'),
                    ('pid', 0),
                    ('tid', 0),
                    ('args', OrderedDict([('name', 'NPU MEM')])),
                    ('ph', 'M'),
                ]
            ),
            OrderedDict(
                [
                    ('name', 'APP/DDR'),
                    ('ts', '0.006'),
                    ('pid', 0),
                    ('tid', 0),
                    ('args', OrderedDict([('KB', 0.0)])),
                    ('ph', 'C'),
                ]
            ),
            OrderedDict(
                [
                    ('name', 'APP/HBM'),
                    ('ts', '0.006'),
                    ('pid', 0),
                    ('tid', 0),
                    ('args', OrderedDict([('KB', 0.0)])),
                    ('ph', 'C'),
                ]
            ),
            OrderedDict(
                [
                    ('name', 'APP/Memory'),
                    ('ts', '0.006'),
                    ('pid', 0),
                    ('tid', 0),
                    ('args', OrderedDict([('KB', 0.0)])),
                    ('ph', 'C'),
                ]
            ),
        ]

        with (
            mock.patch(NAMESPACE + '.NpuMemModel.check_db', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.check_table', return_value=True),
            mock.patch(NAMESPACE + '.NpuMemModel.get_timeline_data', return_value=[npu_mem_dto]),
        ):
            check = NpuMemViewer(config, params)
            ret = check.get_timeline_data()
            self.assertEqual(expect, ret)
