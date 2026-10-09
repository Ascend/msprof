# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from framework.offset_calculator import OffsetCalculator
from msparser.biu_perf.biu_perf_chip6_parser import BiuPerfChip6Parser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.biu_perf.biu_perf_bean import CoreInfoChip6
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.biu_perf.biu_perf_chip6_parser"
CONFIG = {StrConstant.SAMPLE_CONFIG_PROJECT_PATH: "test_project"}


class TestBiuPerfChip6Parser(unittest.TestCase):
    def setUp(self):
        self.file_list = {
            DataTag.BIU_PERF_CHIP6: [
                "instr.biu_perf_group0_aic.0.slice_0",
                "instr.biu_perf_group0_aiv0.0.slice_0",
                "instr.biu_perf_group0_aiv1.0.slice_0"
            ]
        }
        InfoConfReader()._info_json = {"pid": 0, "tid": 0,
                                       "DeviceInfo": [{"hwts_frequency": "50", "aic_frequency": "50"}]}
        InfoConfReader()._host_mon = 0
        InfoConfReader()._dev_cnt = 0
        InfoConfReader()._local_time_offset = 0
        self.parser = BiuPerfChip6Parser(self.file_list, CONFIG)

    def test_ms_run_should_call_parse_and_save(self):
        with mock.patch(NAMESPACE + ".BiuPerfChip6Parser.parse") as mock_parse, \
                mock.patch(NAMESPACE + ".BiuPerfChip6Parser.save") as mock_save:
            self.parser.ms_run()
            mock_parse.assert_called_once()
            mock_save.assert_called_once()

    def test_parse_should_call_parse_data_three_times_and_status(self):
        with mock.patch(NAMESPACE + ".BiuPerfChip6Parser._parse_biu_perf_data") as mock_parse_data, \
                mock.patch(NAMESPACE + ".BiuPerfChip6Parser.parse_status_data") as mock_parse_status:
            self.parser.parse()
            mock_parse_data.assert_called()
            self.assertEqual(3, mock_parse_data.call_count)
            self.assertEqual(3, mock_parse_status.call_count)

    def test_save_should_call_db_flush(self):
        instruction_data = [[[0, 'aic', 0, 15, 1, 4181401579570], [0, 'aic', 0, 15, 0, 4181401581170],
                             [0, 'aic', 0, 15, 16, 4181401581218], [0, 'aic', 0, 15, 0, 4181401583377]]]
        instr_state_data = [[0, 'aic', 0, 'SU', 4181401579570, 1600], [0, 'aic', 0, 'MTE2', 4181401581218, 2159]]
        self.parser._instruction_data = instruction_data
        self.parser._instr_state_data = instr_state_data
        with mock.patch("msmodel.biu_perf.biu_perf_chip6_model.BiuPerfChip6Model.flush") as mock_flush:
            self.parser.save()
            mock_flush.assert_any_call(instruction_data, "OriBiuData")
            mock_flush.assert_any_call(instr_state_data, "BiuInstrStatus")

    def test_read_binary_data_success(self):
        test_file = "instr.biu_perf_group0_aic.0.slice_0"
        test_core_info = CoreInfoChip6("biu_perf_group0_aic")
        bin_data = (b'\x26\x99\x00\xe0\xf4\x8e\x00\xe1\xcd\x03\x00\xe2\x00\x00\x00\xe3\x0c\x17'
                    b'\x01\xf0\x40\x06\x00\xf0\x30\x00\x10\xf0\x6f\x08\x00\xf0')
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.getsize', return_value=4), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + ".FileOpen"), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=bin_data):
            self.parser._offset_calculator = OffsetCalculator([test_file], StructFmt.BIU_PERF_FMT_SIZE, "result_dir")
            result = self.parser.read_binary_data(test_file, test_core_info)
            self.assertEqual(result, NumberConstant.SUCCESS)

    def test_read_binary_data_should_return_error_code_when_files_error(self):
        test_file = "instr.biu_perf_group0_aic.0.slice_0"
        test_core_info = CoreInfoChip6("biu_perf_group0_aic")
        bin_data = (b'\x26\x99\x00\xe0\xf4\x8e\x00\xe1\xcd\x03\x00\xe2\x0c\x17'
                    b'\x01\xf0\x40\x06\x00\xf0\x30\x00\x10\xf0\x6f\x08\x00\xf0')
        # not exist
        result = self.parser.read_binary_data(test_file, test_core_info)
        self.assertEqual(result, NumberConstant.ERROR)
        # exist but size error
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.path.getsize', return_value=1):
            result = self.parser.read_binary_data(test_file, test_core_info)
            self.assertEqual(result, NumberConstant.ERROR)
            # exist but timestamp data lost
            with mock.patch(NAMESPACE + ".FileOpen"), \
                    mock.patch('os.path.getsize', return_value=4), \
                    mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=bin_data):
                self.parser._offset_calculator = OffsetCalculator([test_file], StructFmt.BIU_PERF_FMT_SIZE,
                                                                  "result_dir")
                result = self.parser.read_binary_data(test_file, test_core_info)
                self.assertEqual(result, NumberConstant.ERROR)

    def test_parse_status_data_should_record_four_status(self):
        instruction_data = [
            [1, 2, 3, 15, 0b01010101, 100],
            [1, 2, 3, 15, 0b00000000, 200],
            [1, 2, 3, 1, 0b11111111, 201]
        ]
        self.parser._cur_core_data = instruction_data
        self.parser.parse_status_data()
        self.assertEqual(4, len(self.parser._instr_state_data))
        self.assertEqual(1, len(self.parser._checkpoint_data))
        self.assertEqual([1, 2, 3, 'SU', 100, 100, None], self.parser._instr_state_data[0])
        self.assertEqual([1, 2, 3, 'VEC', 201, 0, 255], self.parser._checkpoint_data[0])

    def test_get_file_group_and_core_type(self):
        result = self.parser._get_file_group_and_core_type()
        self.assertEqual(3, len(result))
        self.assertIn("biu_perf_group0_aic", result)
        self.assertIn("biu_perf_group0_aiv0", result)
        self.assertIn("biu_perf_group0_aiv1", result)


if __name__ == "__main__":
    unittest.main()
