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

from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from msparser.stars.parser_dispatcher import ParserDispatcher
from msparser.stars.stars_log_parser import StarsLogCalCulator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.stars.stars_log_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}

def _make_chunk(magic: int = NumberConstant.STARS_MAGIC_NUM,
                header_byte: int = 0x03) -> bytes:
    """Build a fmt_size-length chunk with given magic at [2:4]."""
    chunk = bytearray(StarsLogCalCulator.DEFAULT_FMT_SIZE)
    chunk[0] = header_byte & 0xFF
    chunk[2] = magic & 0xFF
    chunk[3] = (magic >> 8) & 0xFF
    return bytes(chunk)


class TestStarsLogCalCulator(unittest.TestCase):

    def test_init_dispatcher(self):
        with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.init'):
            check = StarsLogCalCulator({}, sample_config)
            check.init_dispatcher()

    def test_calculate_when_db_exist_then_do_not_execute(self):
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.init_dispatcher'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='123'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='step_info'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch('logging.warning'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2']}, sample_config={'1': 'ada'})
            key._parse_by_iter = mock.Mock()
            key.calculate()
            key._parse_by_iter.assert_not_called()
        ProfilingScene().init(sample_config.get('result_dir'))
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.init_dispatcher'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='123'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=True), \
                mock.patch('logging.info'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2']}, sample_config={'1': 'ada'})
            key._parse_all_file = mock.Mock()
            key.calculate()
            key._parse_all_file.assert_not_called()

    def test_save(self):
        with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.flush_all_parser'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
            key._parser_dispatcher = ParserDispatcher(result_dir='11')
            key.save()

    def test_ms_run(self):
        key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']}, sample_config={'1': 'ada'})
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.calculate'), \
                mock.patch(NAMESPACE + '.StarsLogCalCulator.save'):
            key.ms_run()

    def test_parse_all_file(self):
        data = struct.pack('=4HQ4HQ12Q', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path'), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.FileOpen', return_value=FileOpen('a_2', 'rb')), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=data), \
                mock.patch('common_func.file_manager.check_path_valid'):
            with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.flush_all_parser'), \
                    mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'), \
                    mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.init'):
                key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']},
                                         sample_config={'1': 'ada', 'result_dir': '11'})
                key.calculate()

    def test_parse_by_iter(self):
        ProfilingScene().init("")
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch('common_func.utils.Utils.get_scene', return_value='step_info'), \
                mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.init'), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.init'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove'), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(0, 0)):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']},
                                     sample_config={'1': 'ada', 'result_dir': '11'})
            key.calculate()
        with mock.patch('msparser.stars.parser_dispatcher.ParserDispatcher.init'), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.init'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove'), \
                mock.patch(NAMESPACE + '.HwtsIterModel.get_task_offset_and_sum', return_value=(63, 63)), \
                mock.patch(NAMESPACE + '.FileCalculator.prepare_process', return_value=(63, 63)):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2', 'b_1']},
                                     sample_config={'1': 'ada', 'result_dir': '11'})
            key.calculate()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_validate_chunk_valid_magic(self):
        key = StarsLogCalCulator({}, sample_config)
        self.assertTrue(key._validate_chunk(
            _make_chunk(NumberConstant.STARS_MAGIC_NUM)))

    def test_validate_chunk_invalid_magic(self):
        key = StarsLogCalCulator({}, sample_config)
        self.assertFalse(key._validate_chunk(_make_chunk(0x0000)))
        self.assertFalse(key._validate_chunk(_make_chunk(0xFFFF)))

    def test_parse_data_invalid_chunk_not_dispatched(self):
        key = StarsLogCalCulator({}, sample_config)
        key._parser_dispatcher = mock.Mock()
        key._fmt_size = StarsLogCalCulator.DEFAULT_FMT_SIZE
        data = _make_chunk(0x0000)
        key._parse_data(data)
        self.assertEqual(key.invalid_count, 1)
        key._parser_dispatcher.dispatch.assert_not_called()

    def test_parse_data_valid_chunk_dispatched(self):
        key = StarsLogCalCulator({}, sample_config)
        key._parser_dispatcher = mock.Mock()
        key._fmt_size = StarsLogCalCulator.DEFAULT_FMT_SIZE
        data = _make_chunk(NumberConstant.STARS_MAGIC_NUM, header_byte=0x03)
        key._parse_data(data)
        self.assertEqual(key.invalid_count, 0)
        key._parser_dispatcher.dispatch.assert_called_once()

    def test_parse_data_mixed_chunks(self):
        key = StarsLogCalCulator({}, sample_config)
        key._parser_dispatcher = mock.Mock()
        key._fmt_size = StarsLogCalCulator.DEFAULT_FMT_SIZE
        data = (_make_chunk(0x0000) +
                _make_chunk(NumberConstant.STARS_MAGIC_NUM) +
                _make_chunk(0xFFFF))
        key._parse_data(data)
        self.assertEqual(key.invalid_count, 2)
        self.assertEqual(key._parser_dispatcher.dispatch.call_count, 1)

    def test_parse_data_all_invalid(self):
        key = StarsLogCalCulator({}, sample_config)
        key._parser_dispatcher = mock.Mock()
        key._fmt_size = StarsLogCalCulator.DEFAULT_FMT_SIZE
        data = _make_chunk(0x0000) + _make_chunk(0xAAAA)
        key._parse_data(data)
        self.assertEqual(key.invalid_count, 2)
        key._parser_dispatcher.dispatch.assert_not_called()

    def test_calculate_logs_error_when_invalid_chunks(self):
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + '.StarsLogCalCulator.init_dispatcher'), \
                mock.patch('common_func.db_manager.DBManager.check_tables_in_db', return_value=False), \
                mock.patch(NAMESPACE + '.StarsLogCalCulator._parse_all_file') as mock_parse, \
                mock.patch('logging.error') as mock_log_error, \
                mock.patch('common_func.utils.Utils.get_scene', return_value='single_op'):
            key = StarsLogCalCulator(file_list={DataTag.STARS_LOG: ['a_2']},
                                     sample_config={'1': 'ada'})
            key.invalid_count = 5
            key.calculate()
            mock_log_error.assert_called_once()
            args, _ = mock_log_error.call_args
            self.assertIn('5', str(args))
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)


if __name__ == '__main__':
    unittest.main()
