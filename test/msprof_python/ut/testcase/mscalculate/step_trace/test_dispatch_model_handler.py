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
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
import unittest

from mscalculate.step_trace.tag_handler.tag_dispatch_handler import DispatchModelHandler

NAMESPACE = 'mscalculate.step_trace.tag_handler'


class TestDispatchModelHandler(unittest.TestCase):
    def test_receive_1(self):
        record = {"index_id": 0, "model_id": 1, "stream_id": 1, "task_id": 1}

        real_collect_data = {
            1: {
                1: {
                    "step_start": 2,
                    "step_end": 7,
                    "training_trace": {"fp": 3, "bp": 5},
                    "all_reduce": [{"reduce_start": 4, "reduce_end": 6}],
                    "get_next": {},
                },

                2: {
                    "step_start": 7,
                    "step_end": 12,
                    "training_trace": {"fp": 9, "bp": 11},
                    "all_reduce": [{"reduce_start": 8, "reduce_end": 10}],
                    "get_next": {},
                },

                3: {
                    "step_start": 16,
                    "step_end": 18,
                    "training_trace": {},
                    "all_reduce": [],
                    "get_next": {},
                },

                4: {
                    "step_start": 19,
                    "step_end": 21,
                    "training_trace": {"fp": 20},
                    "all_reduce": [],
                    "get_next": {},
                },

                5: {
                    "step_start": 22,
                    "step_end": 24,
                    "training_trace": {},
                    "all_reduce": [{'reduce_end': None, 'reduce_start': 23}],
                    "get_next": {},
                }

            }
        }

        tag_timestamps = [(2, 1, 1),
                          (0, 2, 1),
                          (2, 3, 1),
                          (10000, 4, 1),
                          (3, 5, 1),
                          (10001, 6, 1),
                          (4, 7, 1),
                          (10002, 8, 1),
                          (2, 9, 1),
                          (10003, 10, 1),
                          (3, 11, 1),
                          (4, 12, 1),
                          (2, 13, 1),
                          (1, 14, 1),
                          (10004, 15, 1),
                          (0, 16, 1),
                          (10005, 17, 1),
                          (1, 18, 1),

                          (0, 19, 1),
                          (2, 20, 1),
                          (1, 21, 1),
                          (0, 22, 1),
                          (10006, 23, 1),
                          (4, 24, 1),
                          (3, 25, 1),
                          (10007, 26, 1),
                          (1, 27, 1)]

        dispatch_model_handler = DispatchModelHandler()

        for tag_timestamp in tag_timestamps:
            record["tag_id"] = tag_timestamp[0]
            record["timestamp"] = tag_timestamp[1]
            record["model_id"] = tag_timestamp[2]
            dispatch_model_handler.receive_record(record)

        test_collect_data = dispatch_model_handler.get_data()
        test_collect_data_1 = dispatch_model_handler.get_data()
        test_collect_data_2 = dispatch_model_handler.get_data()
        self.assertEqual(test_collect_data_2, real_collect_data)

    def test_receive_2(self):
        record = {"index_id": 1, "model_id": 11, "stream_id": 27, "task_id": 16, "tag_id": 4, "timestamp": 3000}
        model_handler = DispatchModelHandler()

        for tag in [2, 3, 4, 2, 3, 4]:
            record["tag_id"] = tag
            model_handler.receive_record(record)
        test_collect_data_3 = model_handler.get_data()
        self.assertEqual(test_collect_data_3, {})

    def test_create_step_trace_run(self):
        record = {"index_id": 1, "model_id": 11, "stream_id": 27, "task_id": 16, "tag_id": 4, "timestamp": 3000}
        model_handler = DispatchModelHandler()

        for tag in [2, 3, 4, 2, 3, 4]:
            record["tag_id"] = tag
            model_handler.receive_record(record)
        test_collect_data_3 = model_handler.get_data()
        self.assertEqual(test_collect_data_3, {})

if __name__ == '__main__':
    unittest.main()
