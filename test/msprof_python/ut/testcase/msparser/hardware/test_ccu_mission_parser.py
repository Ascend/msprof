import os
import shutil
import sqlite3
import struct
import unittest
from unittest import mock

from common_func.file_manager import FdOpen, FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.ccu_mission_parser'


class TestCcuMissionParser(unittest.TestCase):
    file_list = {DataTag.CCU_MISSION: ['ccu0.instr.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_mission")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1, 'chip_model': str(ChipModel.CHIP_V6_1_0.value)
    }

    def setUp(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{"hwts_frequency": "25.000000"}], "devices": "0"}
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)
        os.makedirs(self.DATA_PATH)
        os.makedirs(self.SQLITE_PATH)
        self.make_ccu_mission_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_mission_data(cls):
        data = struct.pack("=16HQHQQ3H", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           139937251735, 693, 139937251707, 139937231292, 691, 1, 4)
        with FdOpen(os.path.join(cls.DATA_PATH, "ccu0.instr.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data_should_return_success(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "ccu0.instr.0.slice_0"))
        self.assertEqual(result, NumberConstant.SUCCESS)
        self.assertEqual(16, len(check.mission_data))
        self.assertEqual([1, 262145, 691, 139937231292, 139937251707, 693, 139937251735, 15, 139937251735],
                         check.mission_data[0])
        self.assertEqual(0, check.mission_data[-1][7])

    def test_read_binary_data_should_count_records_from_preprocessed_buffer(self):
        file_path = os.path.join(self.DATA_PATH, "ccu0.instr.0.slice_0")
        with open(file_path, "rb") as file:
            record = file.read()
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check._source_calculator.pre_process = mock.Mock(return_value=record * 2)

        result = check.read_binary_data(file_path)

        self.assertEqual(NumberConstant.SUCCESS, result)
        self.assertEqual(32, len(check.mission_data))

    def test_read_binary_data_should_join_records_across_short_slices(self):
        first_slice = "ccu0.instr.0.slice_10001"
        second_slice = "ccu0.instr.0.slice_10002"
        source_path = os.path.join(self.DATA_PATH, "ccu0.instr.0.slice_0")
        with open(source_path, "rb") as file:
            record = file.read()
        with FdOpen(os.path.join(self.DATA_PATH, first_slice), operate="wb") as file:
            file.write(record + record[:1])
        with FdOpen(os.path.join(self.DATA_PATH, second_slice), operate="wb") as file:
            file.write(record[1:])

        check = CCUMissionParser({DataTag.CCU_MISSION: [first_slice, second_slice]}, self.CONFIG)

        self.assertEqual(NumberConstant.SUCCESS, check.read_binary_data(first_slice))
        self.assertEqual(NumberConstant.SUCCESS, check.read_binary_data(second_slice))
        self.assertEqual(32, len(check.mission_data))

    def test_read_binary_data_should_isolate_partial_records_by_source(self):
        source_files = [
            "ccu0.instr.0.slice_10011",
            "ccu1.instr.0.slice_10011",
            "ccu0.instr.0.slice_10012",
            "ccu1.instr.0.slice_10012",
        ]
        records = {
            0: bytes(range(64)),
            1: bytes(reversed(range(64))),
        }
        for die_id, offset in ((0, 0), (1, 1)):
            first_slice = records[die_id] + records[die_id][:1]
            second_slice = records[die_id][1:]
            with FdOpen(os.path.join(self.DATA_PATH, source_files[offset]), operate="wb") as file:
                file.write(first_slice)
            with FdOpen(os.path.join(self.DATA_PATH, source_files[offset + 2]), operate="wb") as file:
                file.write(second_slice)

        check = CCUMissionParser({DataTag.CCU_MISSION: source_files}, self.CONFIG)
        check._decoder = mock.Mock(record_size=64)
        check._decoder.decode.return_value = None

        for file_name in source_files:
            self.assertEqual(NumberConstant.SUCCESS, check.read_binary_data(file_name))

        decoded_records = [call.args[0] for call in check._decoder.decode.call_args_list]
        self.assertEqual([records[0], records[1], records[0], records[1]], decoded_records)

    def test_unsupported_source_should_not_change_supported_source_offset(self):
        valid_source = "ccu0.instr.0.slice_0"
        unsupported_source = "ccu2.instr.0.slice_10020"
        with FdOpen(os.path.join(self.DATA_PATH, unsupported_source), operate="wb") as file:
            file.write(b"\x00")

        check = CCUMissionParser({DataTag.CCU_MISSION: [valid_source, unsupported_source]}, self.CONFIG)

        self.assertEqual(NumberConstant.SUCCESS, check.read_binary_data(valid_source))
        self.assertEqual(16, len(check.mission_data))
        self.assertEqual(NumberConstant.ERROR, check.read_binary_data(unsupported_source))

    def test_parse(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.parse()

    def test_save(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.mission_data = [
            [1, 0, 0, 0, 0, 74, 133158368126, 15, 133158368126],
            [1, 0, 0, 0, 0, 74, 133158368126, 14, 133158368126]
        ]
        check.save()

    def test_ms_run(self):
        CCUMissionParser(self.file_list, self.CONFIG).ms_run()

    def test_failed_read_should_not_mark_source_complete(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        with mock.patch.object(check, 'read_binary_data', return_value=NumberConstant.ERROR), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file') as add_complete_file:
            check._handle_original_data('ccu2.instr.0.slice_0')
            check.save()

        add_complete_file.assert_not_called()

    def test_source_should_be_marked_complete_only_after_successful_save(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        with mock.patch.object(check, 'read_binary_data', return_value=NumberConstant.SUCCESS), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file',
                           wraps=FileManager.add_complete_file) as add_complete_file:
            check._handle_original_data('ccu0.instr.0.slice_0')
            add_complete_file.assert_not_called()
            self.assertTrue(check.save())

        add_complete_file.assert_called_once_with(self.DIR_PATH, 'ccu0.instr.0.slice_0')

    def test_failed_commit_should_leave_source_incomplete(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.mission_data = [[1, 0, 0, 0, 0, 74, 133158368126, 15, 133158368126]]
        check._pending_complete_files.append('ccu0.instr.0.slice_0')
        check._model = mock.MagicMock()
        check._model.__enter__.return_value.flush.return_value = False
        with mock.patch(NAMESPACE + '.FileManager.add_complete_file') as add_complete_file:
            self.assertFalse(check.save())

        add_complete_file.assert_not_called()
        self.assertEqual(['ccu0.instr.0.slice_0'], check._pending_complete_files)

    def test_sqlite_integer_overflow_should_leave_source_incomplete(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.mission_data = [[1, 0, 0, 0, 0, 74, NumberConstant.INT64_MAX + 1, 15, 0]]
        check._pending_complete_files.append('ccu0.instr.0.slice_0')
        check._model = mock.MagicMock()

        self.assertFalse(check.save())
        check._model.__enter__.assert_not_called()
        self.assertEqual(['ccu0.instr.0.slice_0'], check._pending_complete_files)

    def test_malformed_host_task_should_fail_instead_of_using_default_stream(self):
        runtime_path = os.path.join(self.SQLITE_PATH, "runtime.db")
        connection = sqlite3.connect(runtime_path)
        try:
            connection.execute("CREATE TABLE HostTask(task_id INTEGER)")
            connection.commit()
        finally:
            connection.close()
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.mission_data = [[1, 7, 0, 0, 0, 74, 100, 15, 100]]

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=runtime_path):
            with self.assertRaisesRegex(RuntimeError, "Query HostTask failed"):
                check._set_stream_id_by_host()

        self.assertEqual(1, check.mission_data[0][0])

    def test_completed_and_pending_sources_without_table_should_fail(self):
        completed_source = 'ccu0.instr.0.slice_0'
        pending_source = 'ccu0.instr.1.slice_1'
        with FdOpen(os.path.join(self.DATA_PATH, completed_source + '.complete')):
            pass
        with FdOpen(os.path.join(self.DATA_PATH, pending_source), operate='wb') as file:
            file.write(bytes(64))
        file_list = {DataTag.CCU_MISSION: [completed_source, pending_source]}
        check = CCUMissionParser(file_list, self.CONFIG)
        check.mission_data = [[1, 0, 0, 0, 0, 74, 100, 15, 100]]
        check._pending_complete_files.append(pending_source)
        check._model = mock.MagicMock()

        self.assertFalse(check.save())
        check._model.__enter__.assert_not_called()
        self.assertEqual([pending_source], check._pending_complete_files)
