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
from msparser.hardware.dvpp_parser import ParsingPeripheralData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.dvpp_parser'


class TestParsingPeripheralData(unittest.TestCase):
    file_list = {DataTag.DVPP: ['dvpp.data.0.slice_0']}

    def test_dvpp_data_parsing(self):
        data = 'time_stamp(s):time_stamp(ns) dvpp_id engine_type engine_id all_time all_frame ' \
               'all_utilization proc_time proc_frame  proc_utilization last_time last_frame\n' + \
               '176167:889901260 8 0 3 0 0 0% 0 0 0% 0 0\n' + \
               '176167:889901260 8 0 2 0 0 0% 0 0 0% 0 0\n'
        null_data = 'time_stamp(s):time_stamp(ns) dvpp_id engine_type engine_id all_time all_frame ' \
                    'all_utilization proc_time proc_frame  proc_utilization last_time last_frame\n' + \
                    'a b\n'
        data_with_no_dvpp_id = 'time_stamp(s):time_stamp(ns) engine_type engine_id all_time all_frame ' \
                               'all_utilization proc_time proc_frame  proc_utilization last_time last_frame\n' + \
                               '176167:889901260 8 0 3 0 0 0% 0 0 0% 0 0\n'
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.dvpp_data_parsing('test\\dvpp.data.5.slice_0')
        with mock.patch('os.path.join', return_value='test\\dvpp.data.5.slice_0'), \
                mock.patch('builtins.open', mock.mock_open(read_data='')), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.warning'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            result = check.dvpp_data_parsing('test\\dvpp.data.5.slice_0')
        self.assertEqual(result, None)
        with mock.patch('os.path.join', return_value='test\\dvpp.data.5.slice_0'), \
                mock.patch('builtins.open', mock.mock_open(read_data=null_data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.warning'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            result = check.dvpp_data_parsing('test\\dvpp.data.5.slice_0')
        self.assertEqual(result, None)
        with mock.patch('os.path.join', return_value='test\\dvpp.data.5.slice_0'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.warning'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.dvpp_data_parsing('test\\dvpp.data.5.slice_0')
        with mock.patch('os.path.join', return_value='test\\dvpp.data.5.slice_0'), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch('builtins.open', mock.mock_open(read_data=data_with_no_dvpp_id)):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.dvpp_data_parsing('test\\dvpp.data.5.slice_0')

    def test_start_parsing_data_file(self):
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=RuntimeError), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch("common_func.file_manager.check_path_valid"), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('msmodel.hardware.dvpp_model.DvppModel.init'), \
                mock.patch('msmodel.hardware.dvpp_model.DvppModel.create_table'), \
                mock.patch('msmodel.hardware.dvpp_model.DvppModel.flush'), \
                mock.patch('msmodel.hardware.dvpp_model.DvppModel.report_data'), \
                mock.patch('msmodel.hardware.dvpp_model.DvppModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.dvpp_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingPeripheralData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingPeripheralData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingPeripheralData(self.file_list, CONFIG)
            check.ms_run()
