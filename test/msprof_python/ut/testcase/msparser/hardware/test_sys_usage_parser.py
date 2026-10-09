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
from msparser.hardware.sys_usage_parser import ParsingCpuUsageData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.sys_usage_parser'


class TestParsingCpuUsageData(unittest.TestCase):
    file_list = {DataTag.SYS_USAGE: ['SystemCpuUsage.data.0.slice_0'],
                 DataTag.PID_USAGE: ['0-CpuUsage.data.0.slice_0']}

    def test_get_sys_cpu_data(self):
        data = "TimeStamp:586319215082\n" + \
               "Index:0\n" + \
               "DataLen:572\n" + \
               "cpu  1950 0 3867 930063 146 0 58 0 0 0\n" + \
               "cpu0 108 0 340 58053 0 0 6 0 0 0\n" + \
               "cpu1 138 0 130 58252 13 0 3 0 0 0\n" + \
               "cpu2 159 0 267 58138 1 0 5 0 0 0\n" + \
               "cpu3 120 0 212 58221 10 0 2 0 0 0\n" + \
               "\n"
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('os.path.realpath', return_value='test\\data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {'devices': '0', "DeviceInfo": [
                {'ctrl_cpu': '0,1,2', 'ai_cpu': '3'}]}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            check.get_sys_cpu_data('SystemCpuUsage.data.0.slice_0')
        self.assertEqual(check.data_dict["sys_data_list"],
                         [['586319215082', 'cpu', '1950', '0', '3867', '930063', '146', '0', '58',
                           '0', '0', '0\n', None],
                          ['586319215082', 'cpu0', '108', '0', '340', '58053', '0', '0', '6', '0',
                           '0', '0\n', 'ctrlcpu'],
                          ['586319215082', 'cpu1', '138', '0', '130', '58252', '13', '0', '3', '0',
                           '0', '0\n', 'ctrlcpu'],
                          ['586319215082', 'cpu2', '159', '0', '267', '58138', '1', '0', '5', '0',
                           '0', '0\n', 'ctrlcpu'],
                          ['586319215082', 'cpu3', '120', '0', '212', '58221', '10', '0', '2', '0',
                           '0', '0\n', 'aicpu']])
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('os.path.realpath', return_value='test\\data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            InfoConfReader()._info_json = {"DeviceInfo": [
                {'ctrl_cpu': '0,1,2', 'ai_cpu': '3'}]}
            check.get_sys_cpu_data('SystemCpuUsage.data.0.slice_0')

    def test_get_proc_cpu_data(self):
        data = "TimeStamp:586333223806\n" + \
               "Index:0\n" + \
               "DataLen:366\n" + \
               "1 (init) S 0 1 1 0 -1 4194560 415 573186 30 289 3 301 822 1490 20 0 1 0 4 " \
               "4694016 520 18446744073709551615 187650726883328 187650727943932 281474508214768 " \
               "0 0 0 0 0 537414151 1 0 0 17 6 0 0 0 0 0 187650728011032 187650728026929 " \
               "187651467292672 281474508218185 281474508218196 281474508218196 281474508218349 0\n" + \
               "ProcessName:init\n" + \
               "cpu  1952 0 3869 930082 146 0 58 0 0 0\n" + \
               "\n"
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('os.path.realpath', return_value='test\\data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            InfoConfReader()._info_json = {"DeviceInfo": [
                {'ctrl_cpu': '0,1,2', 'ai_cpu': '3'}]}
            check.get_proc_cpu_data('1-CpuUsage.data.0.slice_0')
        self.assertEqual(check.data_dict["pid_data_list"],
                         [['1', 'init', '3', '301', '822', '1490', '586333223806', 936107.0]])
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('os.path.realpath', return_value='test\\data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            check.get_proc_cpu_data('1-CpuUsage.data.0.slice_0')

    def test_main(self):
        with mock.patch('os.path.join', side_effect=RuntimeError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            check.main()
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch(NAMESPACE + '.is_valid_original_data'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.ParsingCpuUsageData.get_sys_cpu_data'), \
                mock.patch(NAMESPACE + '.ParsingCpuUsageData.get_proc_cpu_data'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
            InfoConfReader()._info_json = {'devices': '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            check.main()

    def test_save(self):
        with mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.init'), \
                mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.flush'), \
                mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.create_table'), \
                mock.patch('msmodel.hardware.sys_usage_model.SysUsageModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingCpuUsageData(self.file_list, CONFIG)
            check.data_dict = {'sys_data_list': [123]}
            check.save()

    def test_ms_run(self):
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=['test', 'test1']), \
                mock.patch(NAMESPACE + '.ParsingCpuUsageData.main'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingCpuUsageData(self.file_list, CONFIG).ms_run()
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.ParsingCpuUsageData.main', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingCpuUsageData(self.file_list, CONFIG).ms_run()
        with mock.patch('os.path.join', return_value='test\\data'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            InfoConfReader()._info_json = {'devices': '0'}
            ParsingCpuUsageData(self.file_list, CONFIG).ms_run()
