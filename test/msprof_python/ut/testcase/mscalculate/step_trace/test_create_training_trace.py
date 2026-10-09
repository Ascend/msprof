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
import unittest
from unittest import mock

from mscalculate.step_trace.create_step_table import CreateTrainingTrace

NAMESPACE = 'mscalculate.step_trace.create_step_table'


def get_data():

    return {
            1: {
                1: {
                    "step_start": 2,
                    "step_end": 7,
                    "training_trace": {"fp": 3, "bp": 5},
                    "all_reduce": [{"reduce_start": 4, "reduce_end": 6}]
                },

                2: {
                    "step_start": 7,
                    "step_end": 12,
                    "training_trace": {"fp": 9, "bp": 11},
                    "all_reduce": [{"reduce_start": 8, "reduce_end": 10}]
                },

                3: {
                    "step_start": 16,
                    "step_end": 18,
                    "training_trace": {},
                    "all_reduce": []

                },

                4: {
                    "step_start": 19,
                    "step_end": 21,
                    "training_trace": {"fp": 20},
                    "all_reduce": []
                },

                5: {
                    "step_start": 22,
                    "step_end": 24,
                    "training_trace": {},
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
        sample_config = {"result_dir": "./", "devices": '0'}
        with mock.patch(NAMESPACE + '.CreateTrainingTrace.connect_db'):
            CreateTrainingTrace.run(sample_config, A)
            self.assertEqual(CreateTrainingTrace.data,
                             [['0', 1, 1, 3, 5, 7, 5, 2, 2, 0],
                              ['0', 1, 2, 9, 11, 12, 5, 2, 1, 0],
                              ['0', 1, 3, 0, 0, 18, 2, 0, 0, 0],
                              ['0', 1, 4, 20, 0, 21, 2, 0, 0, 0],
                              ['0', 1, 5, 0, 0, 24, 2, 0, 0, 0]]
                             )


if __name__ == '__main__':
    unittest.main()