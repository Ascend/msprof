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
from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from msparser.hardware.llc_parser import decode_llc_v1
from msparser.hardware.llc_parser import decode_llc_v2
from msparser.hardware.llc_parser import NonMiniLLCParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.llc_parser'


class TestNonMiniLLCParser(unittest.TestCase):
    file_list = {DataTag.LLC: ['llc.data.0.slice_0']}

    def tearDown(self) -> None:
        InfoConfReader()._info_json = {}

    def test_decode_llc_v1(self):
        with mock.patch.object(InfoConfReader(), 'get_absolute_time_by_sampling_timestamp', return_value=123456):
            self.assertEqual(decode_llc_v1((66, 88, 1, 2)), (123456, 88, 1, 2))

    def test_decode_llc_v2(self):
        self.assertEqual(decode_llc_v2((2, 88, 1, 3, 123456789)), (123456789, 88, 1, 3))

    def test_read_binary_data(self):
        data = struct.pack("=QQIIQQII", 17152, 49307, 0, 0, 17152, 0, 1, 0)
        with (
            mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'),
            mock.patch('os.path.getsize', return_value=len(data)),
            mock.patch('builtins.open', side_effect=OSError),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.read_binary_data('llc.data.0.slice_0')
        with (
            mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'),
            mock.patch('os.path.getsize', return_value=len(data)),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch('builtins.open', mock.mock_open(read_data=data)),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.read_binary_data('llc.data.0.slice_0')

        data_v2 = struct.pack("=IIIIQ", 2, 88, 1, 3, 123456789)
        with (
            mock.patch.object(InfoConfReader(), 'get_collection_version', return_value="2.0"),
            mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'),
            mock.patch('os.path.getsize', return_value=len(data_v2)),
            mock.patch('common_func.file_manager.check_path_valid'),
            mock.patch('builtins.open', mock.mock_open(read_data=data_v2)),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.read_binary_data('llc.data.0.slice_0')
        self.assertEqual(check.origin_data, [('0', 123456789, 88, 1, 3)])
        # self.assertEqual(check.origin_data,
        #                  [('0', 17152, 49307, 0, 0), ('0', 17152, 0, 1, 0)])

    def test_start_parsing_data_file(self):
        with (
            mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=ValueError),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            result = check.start_parsing_data_file()
        self.assertEqual(result, None)
        with (
            mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True),
            mock.patch(NAMESPACE + '.logging.info'),
            mock.patch(NAMESPACE + '.NonMiniLLCParser.read_binary_data'),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)

            result = check.start_parsing_data_file()
        self.assertEqual(result, None)

    def test_save(self):
        with (
            mock.patch('msmodel.hardware.llc_model.LlcModel.init'),
            mock.patch('msmodel.hardware.llc_model.LlcModel.create_events_trigger'),
            mock.patch('msmodel.hardware.llc_model.LlcModel.flush'),
            mock.patch('msmodel.hardware.llc_model.LlcModel.create_table'),
            mock.patch('msmodel.hardware.llc_model.LlcModel.insert_metrics_data'),
            mock.patch('msmodel.hardware.llc_model.LlcModel.finalize'),
        ):
            InfoConfReader()._info_json = {"devices": '0'}
            check = NonMiniLLCParser(self.file_list, CONFIG)
            check.origin_data = [123]
            check.save()

    def test_run(self):
        with (
            mock.patch(NAMESPACE + '.NonMiniLLCParser.start_parsing_data_file'),
            mock.patch(NAMESPACE + '.NonMiniLLCParser.save', side_effect=OSError),
            mock.patch(NAMESPACE + '.logging.error'),
        ):
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            InfoConfReader()._info_json = {"devices": '0'}
            NonMiniLLCParser(self.file_list, CONFIG).ms_run()
