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
from collections import namedtuple

from mscalculate.flip.flip_calculator import FlipCalculator
from msparser.compact_info.task_track_bean import TaskTrackBean
from common_func.msprof_object import CustomizedNamedtupleFactory
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip

NAMESPACE = 'mscalculate.flip.flip_calculator'


class TestFlipCalculator(unittest.TestCase):

    def test_compute_batch_id_should_do_nothing_when_task_data_is_empty(self):
        task_data = []
        flip_data = []
        task_data = FlipCalculator.compute_batch_id(task_data, flip_data)
        self.assertEqual(task_data, [])

    def test_compute_batch_id_should_set_batch_id_0_when_flip_data_is_empty(self):
        task_data = [
            TaskTrackBean([0, '0', '0', 0, 0, 111111, 0, 1, 1, 1, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111111, 0, 1, 2, 1, 1, 123456789, 0, 0, 0, 0]),
        ]
        flip_data = []
        task_data = FlipCalculator.compute_batch_id(task_data, flip_data)
        self.assertEqual(len(task_data), 2)
        self.assertEqual(task_data[0].batch_id, 0)
        self.assertEqual(task_data[1].batch_id, 0)

    def test_compute_batch_id_should_set_proper_batch_id_with_each_stream_when_flip_sep_task_data(self):
        task_data = [
            TaskTrackBean([0, '0', '0', 0, 0, 111111, 0, 1, 1, 0, 1, 123456789, 0, 0, 0, 0]),  # stream id 1
            TaskTrackBean([0, '0', '0', 0, 0, 111112, 0, 1, 2, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111116, 0, 1, 65534, 0, 1, 123456789, 0, 0, 0, 0]),  # flip
            TaskTrackBean([0, '0', '0', 0, 0, 111120, 0, 1, 1, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111130, 0, 1, 2, 0, 1, 123456789, 0, 0, 0, 0]),

            TaskTrackBean([0, '0', '0', 0, 0, 111130, 0, 2, 1, 0, 1, 123456789, 0, 0, 0, 0]),  # stream id 2
            TaskTrackBean([0, '0', '0', 0, 0, 111131, 0, 2, 65534, 0, 1, 123456789, 0, 0, 0, 0]),  # flip
            TaskTrackBean([0, '0', '0', 0, 0, 111140, 0, 2, 1, 0, 1, 123456789, 0, 0, 0, 0]),  # stream destroy

            TaskTrackBean([0, '0', '0', 0, 0, 111150, 0, 2, 1, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111160, 0, 2, 2, 0, 1, 123456789, 0, 0, 0, 0]),
        ]
        flip_data = [
            TaskFlip(1, 111118, 0, 1),
            TaskFlip(1, 111140, 3, 65535),

            TaskFlip(2, 111132, 0, 1),
            TaskFlip(2, 111141, 3, 65535),  # stream destroy, flip_num=65535
        ]
        task_data = FlipCalculator.compute_batch_id(task_data, flip_data)
        self.assertEqual(len(task_data), 10)
        self.assertEqual([data.batch_id for data in task_data[:3]], [0, 0, 0])
        self.assertEqual([data.batch_id for data in task_data[3:5]], [1, 1])

        self.assertEqual([data.batch_id for data in task_data[5:7]], [0, 0])
        self.assertEqual(task_data[7].batch_id, 1)
        self.assertEqual([data.batch_id for data in task_data[8:10]], [2, 2])

    def test_compute_batch_id_should_set_proper_batch_id_when_flip_task_id_is_not_0(self):
        device_task_type = CustomizedNamedtupleFactory.enhance_namedtuple(
            namedtuple("DeviceTask",
                       ["stream_id", "task_id", "batch_id", "timestamp"]),
            {})
        task_data = [
            TaskTrackBean([0, '0', '0', 0, 0, 111111, 0, 1, 1, 0, 1, 123456789, 0, 0, 0, 0]),  # stream id 1
            TaskTrackBean([0, '0', '0', 0, 0, 111112, 0, 1, 2, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111116, 0, 1, 65534, 0, 1, 123456789, 0, 0, 0, 0]),
            # real flip 1
            TaskTrackBean([0, '0', '0', 0, 0, 111120, 0, 1, 0, 0, 1, 123456789, 0, 0, 0, 0]),
            device_task_type(1, 1, 0, 111130),  # report flip 1
            TaskTrackBean([0, '0', '0', 0, 0, 111140, 0, 1, 3, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111141, 0, 1, 4, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111142, 0, 1, 10, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111144, 0, 1, 65532, 0, 1, 123456789, 0, 0, 0, 0]),
            # real flip 2
            TaskTrackBean([0, '0', '0', 0, 0, 111150, 0, 1, 2, 0, 1, 123456789, 0, 0, 0, 0]),
            TaskTrackBean([0, '0', '0', 0, 0, 111160, 0, 1, 3, 0, 1, 123456789, 0, 0, 0, 0]),  # report flip 2
            TaskTrackBean([0, '0', '0', 0, 0, 111170, 0, 1, 9, 0, 1, 123456789, 0, 0, 0, 0]),
            # stream destroy
            TaskTrackBean([0, '0', '0', 0, 0, 111190, 0, 1, 1, 0, 1, 123456789, 0, 0, 0, 0]),
        ]
        flip_data = [
            TaskFlip(1, 111135, 2, 1),  # report flip 1
            TaskFlip(1, 111165, 6, 1),  # report flip 2
            TaskFlip(1, 111175, 10, 65535),  # stream destroy, flip_num=65535
        ]
        task_data = FlipCalculator.compute_batch_id(task_data, flip_data)
        self.assertEqual(len(task_data), 13)
        self.assertEqual([data.batch_id for data in task_data[:3]], [0, 0, 0])
        self.assertEqual([data.batch_id for data in task_data[3:9]], [1] * 6)

        self.assertEqual([data.batch_id for data in task_data[9:12]], [2, 2, 2])
        self.assertEqual(task_data[12].batch_id, 3)
