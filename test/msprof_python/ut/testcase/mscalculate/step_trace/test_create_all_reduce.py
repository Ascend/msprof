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

from mscalculate.step_trace.create_step_table import CreateAllReduce


def get_data():

    return {
            1: {
                1: {
                    "step_start": 2,
                    "step_end": 7,
                    "training_trace": [{"fp": 3, "bp": 5}],
                    "all_reduce": [{"reduce_start": 4, "reduce_end": 6}]
                },

                2: {
                    "step_start": 7,
                    "step_end": 12,
                    "training_trace": [{"fp": 9, "bp": 11}],
                    "all_reduce": [{"reduce_start": 8, "reduce_end": 10}]
                },

                3: {
                    "step_start": 16,
                    "step_end": 18,
                    "training_trace": [],
                    "all_reduce": []

                },

                4: {
                    "step_start": 19,
                    "step_end": 21,
                    "training_trace": [{"fp": 20, "bp": None}],
                    "all_reduce": []
                },

                5: {
                    "step_start": 22,
                    "step_end": 24,
                    "training_trace": [],
                    "all_reduce": [{'reduce_end': None, 'reduce_start': 23}]
                }

            }
        }


class A:

    @classmethod
    def get_data(cls):
        collect_data = get_data()
        return collect_data


class TestCreateStepTraceData(unittest.TestCase):
    def test_run(self):
        sample_config = {"result_dir": "./"}
        CreateAllReduce.run(sample_config, A)
        self.assertEqual([[None, 1, 1, 7, 4, 6], [None, 1, 2, 12, 8, 10], [None, 1, 5, 24, 23, None]],
                         CreateAllReduce.data)
