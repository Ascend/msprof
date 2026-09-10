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

from common_func.platform.chip_manager import ChipManager
from constant.constant import CONFIG
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from msparser.aic_sample.ai_core_sample_parser import ParsingAICoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingAIVectorCoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingCoreSampleData
from msparser.aic_sample.ai_core_sample_parser import ParsingFftsAICoreSampleData
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.aic_sample.ai_core_sample_parser'


class TestParsingCoreSampleData(unittest.TestCase):

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        ParsingCoreSampleData(CONFIG).ms_run()


class TestParsingFftsAICoreSampleData(unittest.TestCase):
    file_list = {DataTag.FFTS_PMU: ['ffts_profiler.data.0.slice_0']}

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        ParsingFftsAICoreSampleData(self.file_list, CONFIG).ms_run()

    def test_read_binary_data(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', platform_version='5')).process()
        binary_data_path = 'test.slice_0'
        data = struct.pack("=BBHHH8QQQBBHHHBBHHH8QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 49, 88, 99, 100,
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 1, 88, 99, 100)

        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(
                    read_data=data)):
            check = ParsingFftsAICoreSampleData(self.file_list, CONFIG)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_1
            check.read_binary_data(binary_data_path)

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch(NAMESPACE + '.check_aicore_events'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingFftsAICoreSampleData(self.file_list, CONFIG)
            check.data_dict = {
                'aic': {'data_list': [[66, 1, 0, 55.0, 1, 44, 6, 7, 8, 9, 10, 11, 22, 33]], 'db_name': 'aicore_0.db',
                        'event': ['0x50'], 'metrics_key': 'l2_cache'},
                'aiv': {'data_list': [[66, 1, 0, 55.0, 49, 44, 6, 7, 8, 9, 10, 11, 22, 33]],
                        'db_name': 'ai_vector_core_0.db', 'event': ['0x55'], 'metrics_key': 'l2_cache'}}
            check.save()

    def test_insert_ai_core_data_core_id_boundary_chip_v6_2_0(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        check = ParsingFftsAICoreSampleData(self.file_list, CONFIG)
        check._fmt_size = 1
        check._decoder = mock.Mock()
        with mock.patch(NAMESPACE + '.InfoConfReader'), \
                mock.patch.object(check, '_calculate_timestamp', return_value=0.0):
            # core_id == 69 (max ai core id) should be classified as aic
            check._decoder.decode.return_value = mock.Mock(
                count_num=0, mode=0, timestamp=0, core_id=69, task_cyc=0, event_count=[])
            check._insert_ai_core_data(1, [1])
            self.assertEqual(len(check.data_dict['aic']['data_list']), 1)
            self.assertEqual(check.data_dict['aic']['data_list'][0][4], 69)
            self.assertEqual(len(check.data_dict['aiv']['data_list']), 0)

            # core_id == 70 (over max ai core id) should be classified as aiv
            check._decoder.decode.return_value = mock.Mock(
                count_num=0, mode=0, timestamp=0, core_id=70, task_cyc=0, event_count=[])
            check._insert_ai_core_data(1, [1])
            self.assertEqual(len(check.data_dict['aiv']['data_list']), 1)
            self.assertEqual(check.data_dict['aiv']['data_list'][0][4], 70)


class TestParsingAICoreSampleData(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_read_binary_data(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        binary_data_path = 'test.slice_0'
        data = struct.pack("=BBHHH8QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 33, 44, 55, 66, 77, 88, 99, 100)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value='test\\test'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(
                    read_data=data)):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)

    def test_read_binary_data_with_chipV6(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        binary_data_path = 'test.slice_0'
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        data = struct.pack("=BBHHH10QQQBBHHH",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 22, 23, 25, 33, 44, 55, 66, 77, 88, 99, 100)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('builtins.open', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)
        with mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.read_binary_data(binary_data_path)

    def test_start_parsing_data_file(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        with mock.patch('os.path.join', return_value='test\\test'), \
                mock.patch(NAMESPACE + '.logging.error'):
            with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError):
                check = ParsingAICoreSampleData(self.file_list, CONFIG)
                check.start_parsing_data_file()
            with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                    mock.patch(NAMESPACE + '.ParsingAICoreSampleData.read_binary_data'), \
                    mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                    mock.patch(NAMESPACE + '.logging.info'):
                check = ParsingAICoreSampleData(self.file_list, CONFIG)
                check.start_parsing_data_file()

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.ai_core_data = [123]
            check.save()

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch(NAMESPACE + '.ParsingAICoreSampleData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingAICoreSampleData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAICoreSampleData(self.file_list, {'ai_core_profiling_mode': 'sample-based'})
            check.ms_run()
            check = ParsingAICoreSampleData(self.file_list, {'ai_core_profiling_mode': 'task-based'})
            check.ms_run()


class TestParsingAIVectorCoreSampleData(unittest.TestCase):
    file_list = {DataTag.AIV: ['aiVectorCore.data.0.slice_0']}

    def test_start_parsing_data_file(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        with mock.patch(NAMESPACE + '.is_valid_original_data', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()
        with mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.read_binary_data'), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = ParsingAIVectorCoreSampleData(self.file_list, CONFIG)
            check.start_parsing_data_file()

    def test_save(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(aic_frequency='1150', aiv_frequency='1000').device_info])).process()
        with mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.init'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_summary_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.ConfigMgr.read_sample_config'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.create_core_table'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.insert_metric_value'), \
                mock.patch('msmodel.aic.ai_core_sample_model.AiCoreSampleModel.finalize'):
            check = ParsingAICoreSampleData(self.file_list, CONFIG)
            check.ai_core_data = [123]
            check.save()

    def test_ms_run(self):
        InfoJsonReaderManager(info_json=InfoJson(devices='0')).process()
        with mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.start_parsing_data_file'), \
                mock.patch(NAMESPACE + '.ParsingAIVectorCoreSampleData.save', side_effect=OSError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'sample-based'})
            check.ms_run()
            check = ParsingAIVectorCoreSampleData(self.file_list, {'ai_core_profiling_mode': 'task-based'})
            check.ms_run()


if __name__ == '__main__':
    unittest.main()
