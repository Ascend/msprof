#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.add_info.runtime_op_info_bean import RuntimeOpInfo256Bean
from msparser.add_info.runtime_op_info_parser import RuntimeOpInfoParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.runtime_op_info_parser'

RUNTIME_OP_INFO_PATTERN = r"^(unaging|aging)\.additional\.capture_op_info\.slice_\d+"
RUNTIME_OP_INFO_VAR_PATTERN = r"^unaging.\variable\.capture_op_info\.slice_\d+"
class TestRuntimeOpInfoParser(unittest.TestCase):
    file_list = {
        DataTag.RUNTIME_OP_INFO: ['aging.additional.capture_op_info.slice_0',
                                  'unaging.variable.capture_op_info.slice_0']
    }

    def test_ms_run_when_no_file(self):
        check = RuntimeOpInfoParser({}, CONFIG)
        check.ms_run()

    def test_ms_run_when_run_error(self):
        with mock.patch(NAMESPACE + '.RuntimeOpInfoParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_when_run_success(self):
        with mock.patch(NAMESPACE + '.RuntimeOpInfoParser._parse_for_256_data'), \
                mock.patch(NAMESPACE + '.RuntimeOpInfoParser._parse_for_var_data'), \
                mock.patch(NAMESPACE + '.RuntimeOpInfoParser.save'):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save_when_no_data(self):
        check = RuntimeOpInfoParser(self.file_list, CONFIG)
        check.save()

    def test_save_when_run_success(self):
        with mock.patch('msmodel.add_info.runtime_op_info_model.RuntimeOpInfoModel.init'), \
                mock.patch('msmodel.add_info.runtime_op_info_model.RuntimeOpInfoModel.flush'):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check._runtime_op_info_data = [[1, 2, 3, 4, 5]]
            check.save()

    def test_parse_for_256_data(self):
        info = (23130, 10000, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 14, 2,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # input
                1, 88, 2, 3, 4, 5, 0, 0, 0, 0, 0, # output
                *(0,) * 20)
        struct_data = struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.RUNTIME_OP_INFO_256_FMT, *info)
        with mock.patch(NAMESPACE + '.RuntimeOpInfoParser.parse_bean_data',
                        return_value=[RuntimeOpInfo256Bean.decode(struct_data)]):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check._parse_for_256_data(['aging.additional.capture_op_info.slice_0'])
            self.assertEqual(check._runtime_op_info_data, [['node', '2', 3, 5, 7, 6, 8, 9, '12', 'FUSION',
                                                            '13', 'N/A', 11, 0, 14, 1, 2, 'NCHW', 'FLOAT', 'N/A', '88',
                                                            'INT8', '"3,4,5"']])
            self.assertEqual(check._256_size, 1)
            self.assertEqual(check._var_size, 0)

    def test_parse_for_var_data_when_only_check_file(self):
        with mock.patch('os.path.exists', side_effect=[True, False]), \
                mock.patch(NAMESPACE + '.RuntimeOpInfoParser.get_file_path_and_check', return_value="./test"), \
                mock.patch(NAMESPACE + '.RuntimeOpInfoParser._read_data'):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check._parse_for_var_data(['unaging.variable.capture_op_info.slice_0',
                                       'unaging.variable.capture_op_info.slice_1'])

    def test_read_data_with_variable_length_data_when_data_is_valid_then_success(self):
        tensor_num = 7
        info = (23130, 10000, 2, 3, 372, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 14, tensor_num,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  # input
                1, 88, 2, 3, 4, 5, 0, 0, 0, 0, 0,  # output
                0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0,  # input
                1, 30, 10, 3, 8, 5, 17, 0, 0, 0, 0,  # output
                0, 10, 11, 12, 13, 0, 0, 0, 0, 0, 0,  # input
                1, 20, 5, 3, 15, 5, 16, 0, 0, 0, 0,  # output
                0, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0)  # input
        struct_data = struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.RUNTIME_OP_INFO_FMT +
                                  tensor_num * StructFmt.RUNTIME_OP_INFO_TENSOR_FMT,
                                  *info)
        with mock.patch('os.path.getsize',return_value=StructFmt.RUNTIME_OP_INFO_BODY_SIZE +
                                                       tensor_num * StructFmt.RUNTIME_OP_INFO_TENSOR_SIZE,), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=struct_data)):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check._read_data("unaging", 'unaging.variable.capture_op_info.slice_0')
            self.assertEqual(check._runtime_op_info_data,
                             [['node', '2', 3, 5, 7, 6, 8, 9, '12', 'FUSION', '13', 'N/A', 11, 0, 14, 0, 7,
                              'NCHW;NHWC;FRACTAL_DECONV_TRANSPOSE;FRACTAL_DECONV', 'FLOAT;INT8;DOUBLE;UINT16',
                              '";3,4,5,6,7;12,13;6,5,4,3,2,1"', '88;NCDHW;HASHTABLE_LOOKUP_LOOKUPS',
                              'INT8;UINT64;UNDEFINED', '"3,4,5;3,8,5,17;3,15,5,16"']])
            self.assertEqual(check._256_size, 0)
            self.assertEqual(check._var_size, 1)

    def test_read_data_with_variable_length_data_when_tensor_num_is_invalid_then_failed(self):
        tensor_num = 7
        # dataLen 盖不住 固定体(64) + 7 * tensor(44) = 372，应报错并跳过
        info = (23130, 10000, 2, 3, 100, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0, 14, tensor_num,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  # input
                1, 88, 2, 3, 4, 5, 0, 0, 0, 0, 0,  # output
                0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0,  # input
                1, 30, 10, 3, 8, 5, 17, 0, 0, 0, 0,  # output
                0, 10, 11, 12, 13, 0, 0, 0, 0, 0, 0,  # input
                1, 20, 5, 3, 15, 5, 16, 0, 0, 0, 0,  # output
                0, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0)  # input
        struct_data = struct.pack(StructFmt.BYTE_ORDER_CHAR + StructFmt.RUNTIME_OP_INFO_FMT +
                                  tensor_num * StructFmt.RUNTIME_OP_INFO_TENSOR_FMT,
                                  *info)
        header_size = StructFmt.RUNTIME_OP_INFO_BODY_SIZE - StructFmt.RUNTIME_OP_INFO_WITHOUT_HEAD_SIZE
        with mock.patch('os.path.getsize', return_value=header_size + 100), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=struct_data)):
            check = RuntimeOpInfoParser(self.file_list, CONFIG)
            check._read_data("unaging", 'unaging.variable.capture_op_info.slice_0')
            self.assertEqual(check._runtime_op_info_data, [])
            self.assertEqual(check._256_size, 0)
            self.assertEqual(check._var_size, 1)


if __name__ == '__main__':
    unittest.main()
