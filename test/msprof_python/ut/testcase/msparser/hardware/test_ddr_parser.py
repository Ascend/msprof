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

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from constant.constant import INFO_JSON
from msparser.hardware.ddr_parser import ParsingDDRData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.ddr_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs', 'devices': '0'}


class TestParsingDDRData(unittest.TestCase):
    file_list = {DataTag.DDR: ['ddr.data.0.slice_0']}

    def test_read_binary_data(self):
        file_name = '123'
        device_id = '0'
        replayid = 1
        data = struct.pack("=IIIII", 1, 2, 3, 4, 5)
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.struct.calcsize', return_value=1280), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch("common_func.file_manager.check_path_valid"):
            InfoConfReader()._info_json = INFO_JSON
            key = ParsingDDRData(self.file_list, sample_config)
            result = key.read_binary_data(file_name, device_id, replayid)
        self.assertEqual(result, 0)
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.struct.calcsize', return_value=1280), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch("common_func.file_manager.check_path_valid"):
            InfoConfReader()._info_json = {'devices': '0'}
            key = ParsingDDRData(self.file_list, sample_config)
            result = key.read_binary_data(file_name, device_id, replayid)
        self.assertEqual(result, 1)

    def test_update_ddr_data_1(self):
        time_start = [(1, 2, 3, 4, 5, 6)]
        headers = 1
        item = [(1, 2, 3, 4, 5, 6)]
        InfoConfReader()._info_json = {'devices': '0'}
        key = ParsingDDRData(self.file_list, sample_config)
        key._update_ddr_data(time_start, item, headers)

    def test_update_ddr_data_2(self):
        time_start = [1, 2, 3, 4, 5, 6]
        headers = [1]
        item = [[1], [2], [3], [4], [5], [6]]
        InfoConfReader()._info_json = {'devices': '0'}
        key = ParsingDDRData(self.file_list, sample_config)
        key._update_ddr_data(time_start, item, headers)

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError):
            InfoConfReader()._info_json = {'devices': '0'}
            key = ParsingDDRData(self.file_list, sample_config)
            result = key.start_parsing_data_file()
        self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.logging.info'):
            with mock.patch(NAMESPACE + '.ParsingDDRData.read_binary_data',
                            return_value=0), \
                    mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
                InfoConfReader()._info_json = {'devices': '0'}
                key = ParsingDDRData(self.file_list, sample_config)
                result = key.start_parsing_data_file()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.ParsingDDRData.read_binary_data',
                            return_value=1), \
                    mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
                InfoConfReader()._info_json = {'devices': '0'}
                key = ParsingDDRData(self.file_list, sample_config)
                result = key.start_parsing_data_file()
            self.assertEqual(result, None)

    def test_save(self):
        with mock.patch('msmodel.hardware.ddr_model.DdrModel.init'), \
                mock.patch('msmodel.hardware.ddr_model.DdrModel.create_table'), \
                mock.patch('msmodel.hardware.ddr_model.DdrModel.flush'), \
                mock.patch('msmodel.hardware.ddr_model.DdrModel.insert_metric_data'), \
                mock.patch('msmodel.hardware.ddr_model.DdrModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingDDRData(self.file_list, CONFIG)
            check.ddr_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingDDRData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingDDRData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            key = ParsingDDRData(self.file_list, sample_config)
            key.ms_run()


if __name__ == '__main__':
    unittest.main()
