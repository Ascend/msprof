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

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.hardware.nic_parser import ParsingNicData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.nic_parser'


class TestParsingNicData(unittest.TestCase):
    file_list = {DataTag.NIC: ['nic.data.0.slice_0']}

    def test_read_binary_data(self):
        data_none = 'rxPacket/s 000\n'
        data_16 = '176168:476067240 100000 0 0 0 0 0 0 0 0 0 0 0 0\n' + \
                  '176168:576032880 100000 0 0 0 0 0 0 0 0 0 0 0 0\n' + \
                  '176168:676029070 100000 0 0 0 0 0 0 0 0 0 0 0 0\n'
        data_17 = '176168:476067240 100000 0 0 0 0 0 0 0 0 0 0 0 0 0\n' + \
                  '176168:576032880 100000 0 0 0 0 0 0 0 0 0 0 0 0 0\n' + \
                  '176168:676029070 100000 0 0 0 0 0 0 0 0 0 0 0 0 0\n'
        with mock.patch('os.path.join',
                        return_value='test\\data\\roce.data.5.slice_0'), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
             mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch('builtins.open', side_effect=OSError):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingNicData(self.file_list, CONFIG)
                check.read_binary_data('test')
            with mock.patch('builtins.open', mock.mock_open(read_data="")):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingNicData(self.file_list, CONFIG)
                result = check.read_binary_data('test')
            self.assertEqual(result, None)
            with mock.patch('builtins.open', mock.mock_open(read_data=data_none)):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingNicData(self.file_list, CONFIG)
                result = check.read_binary_data('test')
            self.assertEqual(result, None)
            with mock.patch('builtins.open', mock.mock_open(read_data=data_16)), \
                    mock.patch(NAMESPACE + '.DBManager.get_table_field_num',
                               return_value=17):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingNicData(self.file_list, CONFIG)
                result = check.read_binary_data('test')
            self.assertEqual(result, None)
            with mock.patch('builtins.open', mock.mock_open(read_data=data_17)), \
                    mock.patch(NAMESPACE + '.DBManager.get_table_field_num',
                               return_value=17):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingNicData(self.file_list, CONFIG)
                result = check.read_binary_data('test')
            self.assertEqual(result, None)

    def test_start_parsing_data_file(self):
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingNicData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch('os.path.join', return_value='test\\data_0'), \
                mock.patch(NAMESPACE + '.is_valid_original_data'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.ParsingNicData.read_binary_data'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingNicData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch('os.path.join', return_value='test\\data_1'), \
                mock.patch(NAMESPACE + '.is_valid_original_data'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.ParsingNicData.read_binary_data'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingNicData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('msmodel.hardware.nic_model.NicModel.init'), \
                mock.patch('msmodel.hardware.nic_model.NicModel.flush'), \
                mock.patch('msmodel.hardware.nic_model.NicModel.create_table'), \
                mock.patch('msmodel.hardware.nic_model.NicModel.report_data'), \
                mock.patch('msmodel.hardware.nic_model.NicModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingNicData(self.file_list, CONFIG)
            check.nic_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingNicData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingNicData.save',
                           side_effect=RuntimeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            ParsingNicData(self.file_list, CONFIG).ms_run()
