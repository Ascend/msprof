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
from unittest import mock
from unittest.mock import Mock

from common_func.info_conf_reader import InfoConfReader
from msparser.aicpu.parse_dp_data import ParseDpData

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs'}
NAMESPACE = 'msparser.aicpu.parse_dp_data'


def test_get_files():
    path = '100\\3_1'
    tag = 'db_files'
    device_id = 123
    list_dir = ['test_1', 'test_2']
    with mock.patch('os.path.dirname', return_value='123'):
        with mock.patch('os.listdir', return_value=list_dir), \
                mock.patch(NAMESPACE + '.get_file_name_pattern_match', return_value=True), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True):
            result = ParseDpData.get_files(path, tag, device_id)
            unittest.TestCase().assertEqual(len(result), 2)
        with mock.patch('os.listdir', return_value=[]):
            result = ParseDpData.get_files(path, tag, device_id)
            unittest.TestCase().assertEqual(result, [])


def test_analyse_dp():
    InfoConfReader()._local_time_offset = 10.0
    dp_path = 'home\\parse_dp'
    device_id = 123
    data = bytes("[16953839403] Last queue dequeue, source:IteratorV2, index:0, size:111 ", "UTF-8")
    check = [(16953849.403, 'Last queue dequeue', 'IteratorV2', '111 ')]
    bin_data = struct.pack('=HH', 23130, 100)
    with mock.patch(NAMESPACE + '.ParseDpData.get_files', return_value=['test.slice_0']), \
            mock.patch('builtins.open', mock.mock_open(read_data=bin_data)), \
            mock.patch(NAMESPACE + '.ParseDpData.dp_data_dispatch', return_value='bin'), \
            mock.patch('os.path.getsize', return_value=4), \
            mock.patch(NAMESPACE + '.ParseDpData.analyse_bin_dp', return_value=(1, 2)):
        result = ParseDpData.analyse_dp(dp_path, device_id)
        unittest.TestCase().assertEqual(result, (1, 2))
    with mock.patch(NAMESPACE + '.ParseDpData.get_files', return_value=['test.slice_0']), \
            mock.patch('os.path.getsize', return_value=4), \
            mock.patch('os.path.exists', return_value=True), \
            mock.patch('os.path.isfile', return_value=True), \
            mock.patch('os.access', return_value=True), \
            mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=0)), \
            mock.patch(NAMESPACE + '.logging.error'), \
            mock.patch('builtins.open', mock.mock_open(read_data=data)):
        result = ParseDpData.analyse_dp(dp_path, device_id)
        unittest.TestCase().assertEqual(result, check)


def test_dp_data_dispatch():
    with mock.patch(NAMESPACE + '.ParseDpData.get_files', return_value=['test.slice_0']), \
            mock.patch('builtins.open', side_effect=OSError), \
            mock.patch(NAMESPACE + '.ParseDpData.get_dp_judge_data', return_value=('tset', 124)), \
            mock.patch('os.path.getsize', return_value=4), \
            mock.patch('os.path.exists', return_value=True), \
            mock.patch('os.path.isfile', return_value=True), \
            mock.patch('os.access', return_value=True), \
            mock.patch(NAMESPACE + '.ParseDpData.analyse_bin_dp', return_value=(1, 2)), \
            mock.patch(NAMESPACE + '.logging.error'):
        result = ParseDpData.dp_data_dispatch(['test'])
        unittest.TestCase().assertEqual(result, 'str_or_bytes')


def test_analyse_bin_dp():
    with mock.patch(NAMESPACE + '.ParseDpData.read_bin_data', return_value=[123]), \
            mock.patch(NAMESPACE + '.logging.error'):
        res_1 = ParseDpData.analyse_bin_dp(['test'])
        unittest.TestCase().assertEqual(res_1, [123])
        res_2 = ParseDpData.analyse_bin_dp([])
        unittest.TestCase().assertEqual(res_2, [])
    with mock.patch(NAMESPACE + '.ParseDpData.read_bin_data', side_effect=OSError), \
            mock.patch(NAMESPACE + '.logging.error'):
        res_1 = ParseDpData.analyse_bin_dp(['test'])
        unittest.TestCase().assertEqual(res_1, [])


def test_read_bin_data():
    InfoConfReader()._local_time_offset = 10.0
    data = struct.pack("=HHIQ16s64sQQ2Q", 23130, 100, 1, 2, b'test', b'test', 0, 0, 0, 0)
    with mock.patch('os.path.getsize', return_value=128), \
            mock.patch("common_func.file_manager.check_path_valid"), \
            mock.patch('builtins.open', mock.mock_open(read_data=data)):
        result = ParseDpData.read_bin_data('test')
        unittest.TestCase().assertEqual(result, [(10.002, 'test', 'test', 0)])
    with mock.patch('os.path.getsize', return_value=128), \
            mock.patch('builtins.open', side_effect=OSError), \
            mock.patch("common_func.file_manager.check_path_valid"), \
            mock.patch(NAMESPACE + '.logging.error'):
        result = ParseDpData.read_bin_data('test')
        unittest.TestCase().assertEqual(result, [])


def test_get_dp_judge_data():
    with mock.patch('os.path.getsize', return_value=1280):
        result = ParseDpData.get_dp_judge_data(['test.slice_0'])
        unittest.TestCase().assertEqual(result, ('test.slice_0', 0))


if __name__ == '__main__':
    unittest.main()
