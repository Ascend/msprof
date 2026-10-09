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
import struct
import unittest

from profiling_bean.ge.ge_tensor_bean import GeTensorBean

NAMESPACE = 'profiling_bean.struct_info.ge.ge_fusion_op_info_bean'


class TestGeTensorBean(unittest.TestCase):

    def test_construct(self):
        bean = GeTensorBean()
        binary_data = b'ZZ\x18\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\t\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\t\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\t\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\t\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

        bean.fusion_decode(binary_data)
        self.assertEqual(bean.input_data_type, 'INT64;INT64')
        self.assertEqual(bean.index_num, 0)
        self.assertEqual(bean.input_format, 'ND;ND')
        self.assertEqual(bean.input_shape, ';')
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.output_data_type, 'INT64')
        self.assertEqual(bean.output_format, 'ND')
        self.assertEqual(bean.output_shape, '')
        self.assertEqual(bean.stream_id, 9)
        self.assertEqual(bean.task_id, 2)
        self.assertEqual(bean.tensor_num, 3)
        self.assertEqual(bean.tensor_type, 0)

    def test_construct_when_data_type_and_format_is_undefined_or_subformat_is_valid(self):
        binary_data = struct.pack('=HHIQIHHI55IQ', 23130, 24, 1, 0, 123, 456, 0, 3, 0, 280, 35, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 2 ** 32 - 1, 20, 0, 0, 0, 0, 0, 0, 0, 0, 1, 13, 25, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        bean = GeTensorBean()

        bean.fusion_decode(binary_data)
        self.assertEqual(bean.input_data_type, 'DT_FLOAT8_E5M2;QINT32')
        self.assertEqual(bean.index_num, 0)
        self.assertEqual(bean.input_format, 'HASHTABLE_LOOKUP_HITS:1;UNDEFINED')
        self.assertEqual(bean.input_shape, ';')
        self.assertEqual(bean.model_id, 1)
        self.assertEqual(bean.output_data_type, 'DUAL')
        self.assertEqual(bean.output_format, 'FRACTAL_Z_C04')
        self.assertEqual(bean.output_shape, '')
        self.assertEqual(bean.stream_id, 123)
        self.assertEqual(bean.task_id, 456)
        self.assertEqual(bean.tensor_num, 3)
        self.assertEqual(bean.tensor_type, 0)


if __name__ == '__main__':
    unittest.main()
