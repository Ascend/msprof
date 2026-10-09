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
from msparser.hardware.pcie_parser import ParsingPcieData
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.pcie_parser'


class TestMergeOPCounter(unittest.TestCase):
    file_list = {DataTag.PCIE: ['pcie.data.0.slice_0']}

    def test_read_binary_data(self):
        with mock.patch('os.path.join', return_value='test\\data'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('os.path.getsize', return_value=192):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingPcieData(self.file_list, CONFIG)
            result = check.read_binary_data('pcie.data.5.slice_0', 5)
        self.assertEqual(result, 1)
        data = struct.pack("=Q22IQ22I",
                           80, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
                           40, 40, 40, 40, 40, 40, 100, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
                           50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50)
        with mock.patch('os.path.join', return_value='test\\data'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
             mock.patch('os.path.getsize', return_value=192):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingPcieData(self.file_list, CONFIG)
            result = check.read_binary_data('pcie.data.5.slice_0', 5)
        self.assertEqual(result, 0)

    def test_start_parsing_data_file(self):
        with mock.patch('os.path.join', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingPcieData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch('os.path.join', return_value='test\\db'), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True):
            with mock.patch(NAMESPACE + '.logging.error'):
                InfoConfReader()._info_json = {"devices": '0'}
                check = ParsingPcieData(self.file_list, CONFIG)
                result = check.start_parsing_data_file()
            self.assertEqual(result, None)
            with mock.patch(NAMESPACE + '.logging.info'):
                with mock.patch(NAMESPACE + '.ParsingPcieData.read_binary_data',
                                return_value=1), \
                        mock.patch(NAMESPACE + '.logging.error'):
                    InfoConfReader()._info_json = {"devices": '0'}
                    check = ParsingPcieData(self.file_list, CONFIG)
                    result = check.start_parsing_data_file()
                self.assertEqual(result, None)
                with mock.patch(NAMESPACE + '.ParsingPcieData.read_binary_data',
                                return_value=0), \
                        mock.patch(NAMESPACE + '.FileManager.add_complete_file'):
                    InfoConfReader()._info_json = {"devices": '0'}
                    check = ParsingPcieData(self.file_list, CONFIG)
                    check.start_parsing_data_file()

    def test_save(self):
        with mock.patch('msmodel.hardware.pcie_model.PcieModel.init'), \
                mock.patch('msmodel.hardware.pcie_model.PcieModel.flush'), \
                mock.patch('msmodel.hardware.pcie_model.PcieModel.flush'), \
                mock.patch('msmodel.hardware.pcie_model.PcieModel.finalize'):
            InfoConfReader()._info_json = {"devices": '0'}
            check = ParsingPcieData(self.file_list, CONFIG)
            check.pcie_data = [123]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParsingPcieData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingPcieData.save',
                           side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            InfoConfReader()._info_json = {"devices": '0'}
            ParsingPcieData(self.file_list, CONFIG).ms_run()
