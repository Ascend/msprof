#!/usr/bin/python3
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
from viewer.chip_model_function.chip_model_decorators import format_pmu_data_by_headers

header = [
    'Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type', 'OP State', 'Task Type',
    'Task Start Time(us)', 'Task Duration(us)', 'Task Wait Time(us)', 'Block Num', 'Mix Block Num',
    'HF32 Eligible', 'Input Shapes', 'Input Data Types', 'Input Formats', 'Output Shapes', 'Output Data Types',
    'Output Formats', 'Context ID', 'total_time', 'total_cycles', 'ub_read_bw', 'ub_write_bw', 'l1_read_bw',
    'l1_write_bw', 'main_mem_read_bw', 'main_mem_write_bw'
]

tasks = [
    [
            'segformer_framework_onnx_aipp_0_batch_1_input_fp16_output_int32', 1, 0, 3, 1, 'trans_Cast_0', 'Cast',
            'static', 'AI_VECTOR_CORE', 13126617660075.5, 196916.666015625, 0, 1, 0, 'NO', '"1,3,512,1024"', 'FLOAT',
            'NCHW', '"1,3,512,1024"', 'FLOAT16', 'NCHW', 'N/A', 190.18, 237729, 0.0, 0.0, 0.0, 0.0, 0.482, 0.243
    ]
]

res = [
    [
        'segformer_framework_onnx_aipp_0_batch_1_input_fp16_output_int32', 1, 0, 3, 1, 'trans_Cast_0', 'Cast',
        'static', 'AI_VECTOR_CORE', 13126617660075.5, 196916.666015625, 0, 1, 0, 'NO', '"1,3,512,1024"', 'FLOAT',
        'NCHW', '"1,3,512,1024"', 'FLOAT16', 'NCHW', 'N/A', 190.18, 237729, 'N/A', 'N/A', 0.0, 0.0, 0.482, 'N/A'
     ]
]


class TestChipModeDecorators(unittest.TestCase):

    def test_pmu_format_for_chip_model(self):
        with mock.patch('viewer.chip_model_function.chip_model_decorators.get_disable_support_pmu_set',
                        return_value={'ub_read_bw', 'ub_write_bw', 'main_mem_write_bw'}):
            _, return_tasks = format_pmu_data_by_headers(header, tasks)
            self.assertEqual(res, return_tasks)


if __name__ == '__main__':
    unittest.main()
