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
import struct
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.hbm_parser import decode_hbm_v1
from msparser.hardware.hbm_parser import decode_hbm_v2
from msparser.hardware.hbm_parser import ParsingHBMData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.hbm_parser'


class TestParsingHBMData(unittest.TestCase):
    file_list = {DataTag.HBM: ['hbm.data.0.slice_0']}

    def tearDown(self) -> None:
        InfoConfReader()._info_json = {}

    def test_decode_hbm_v1(self):
        info_reader_mock = mock.MagicMock()
        info_reader_mock.get_absolute_time_by_sampling_timestamp.return_value = 123456
        with mock.patch(NAMESPACE + '.InfoConfReader', return_value=info_reader_mock):
            self.assertEqual(decode_hbm_v1((66, 88, 0, 2)), (123456, 88, 'read', 2))

    def test_decode_hbm_v2(self):
        self.assertEqual(decode_hbm_v2((2, 0, 1, 3, 88, 123456789)), (123456789, 88, 'write', 3))

    def test_read_binary_data(self):
        data = struct.pack("=QQIIQQIIQQIIQQII", 66, 0, 0, 0, 66, 0, 0, 1, 66, 0, 0, 2, 66, 0, 0, 3)
        with (
            mock.patch('os.path.join', return_value='test\\test'),
            mock.patch('os.path.exists', return_value=True),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch('struct.calcsize', return_value=False),
            mock.patch('os.path.getsize', return_value=96),
        ):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            result = check.read_binary_data('hbm.data.5.slice_0', "5", "0")
        self.assertEqual(result, 1)
        with (
            mock.patch('os.path.join', return_value='test\\test'),
            mock.patch('os.path.exists', return_value=True),
            mock.patch('builtins.open', mock.mock_open(read_data=data)),
            mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch('os.path.getsize', return_value=96),
        ):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            InfoConfReader()._start_log_time = 10000
            result = check.read_binary_data('hbm.data.5.slice_0', "5", "0")
        self.assertEqual(result, 0)

    def test_read_binary_data_v2(self):
        data = struct.pack("=IIIIQQ", 2, 0, 1, 3, 88, 123456789)
        # 设置单例实例属性，让 get_absolute_time_by_sampling_timestamp 返回正确的值
        # get_absolute_time_by_sampling_timestamp = timestamp * USTONS + _start_log_time + (_host_mon - _start_log_time/NANO_SECOND) * NANO_SECOND + _local_time_offset * NS_TO_US
        # = timestamp * 1000 + _start_log_time + _host_mon * 1000000000 - _start_log_time + _local_time_offset * 1000
        # = timestamp * 1000 + _host_mon * 1000000000 + _local_time_offset * 1000
        # 我们希望对于 timestamp=123456789，返回值也是 123456789
        # 所以: 123456789 = 123456789 * 1000 + _host_mon * 1000000000 + _local_time_offset * 1000
        # => _host_mon * 1000000000 + _local_time_offset * 1000 = 123456789 - 123456789000 = -123333332211
        InfoConfReader()._info_json = {'version': '2.0'}
        InfoConfReader()._start_log_time = 0
        InfoConfReader()._host_mon = -0.123333332211  # -123333332211 / 1000000000
        InfoConfReader()._local_time_offset = 0
        with (
            mock.patch('os.path.join', return_value='test\\test'),
            mock.patch('os.path.exists', return_value=True),
            mock.patch('builtins.open', mock.mock_open(read_data=data)),
            mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch('os.path.getsize', return_value=32),
        ):
            check = ParsingHBMData(self.file_list, CONFIG)
            result = check.read_binary_data('hbm.data.5.slice_0', "5", "0")
        self.assertEqual(result, 0)
        self.assertEqual(check.hbm_data, [('5', '0', 123456789, 88, 'write', 3)])

    def test_start_parsing_data_file(self):
        with (
            mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with (
            mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True),
            mock.patch(NAMESPACE + '.logging.info'),
            mock.patch(NAMESPACE + '.FileManager.add_complete_file'),
        ):
            with mock.patch(NAMESPACE + '.ParsingHBMData.read_binary_data', return_value=0):
                InfoConfReader()._info_json = {'devices': '0'}
                check = ParsingHBMData(self.file_list, CONFIG)
                check.start_parsing_data_file()
            with (
                mock.patch(NAMESPACE + '.ParsingHBMData.read_binary_data', return_value=1),
                mock.patch(NAMESPACE + '.logging.error'),
            ):
                InfoConfReader()._info_json = {'devices': '0'}
                check = ParsingHBMData(self.file_list, CONFIG)
                check.start_parsing_data_file()

    def test_save(self):
        with (
            mock.patch('msmodel.hardware.hbm_model.HbmModel.init'),
            mock.patch('msmodel.hardware.hbm_model.HbmModel.create_table'),
            mock.patch('msmodel.hardware.hbm_model.HbmModel.flush'),
            mock.patch('msmodel.hardware.hbm_model.HbmModel.insert_bw_data'),
            mock.patch('msmodel.hardware.hbm_model.HbmModel.finalize'),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingHBMData(self.file_list, CONFIG)
            check.hbm_data = [123]
            check.save()

    def test_ms_run(self):
        with (
            mock.patch(NAMESPACE + '.ParsingHBMData.start_parsing_data_file'),
            mock.patch(NAMESPACE + '.ParsingHBMData.save', side_effect=OSError),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingHBMData(self.file_list, CONFIG).ms_run()
