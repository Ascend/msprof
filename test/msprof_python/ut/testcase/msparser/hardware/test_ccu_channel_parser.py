import os
import shutil
import struct
import unittest
from unittest import mock

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from msparser.hardware.ccu_channel_parser import CCUChannelParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.hardware.ccu_channel_parser'


class TestCcuChannelParser(unittest.TestCase):
    file_list = {DataTag.CCU_CHANNEL: ['ccu0.stat.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_channel")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1, 'chip_model': str(ChipModel.CHIP_V6_1_0.value)
    }

    def setUp(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{"hwts_frequency": "25.000000"}]}
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)
        os.makedirs(self.DATA_PATH)
        os.makedirs(self.SQLITE_PATH)
        self.make_ccu_channel_data()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_channel_data(cls):
        data_list = [i for i in range(704)]
        data = struct.pack("=704I", *data_list)
        with FdOpen(os.path.join(cls.DATA_PATH, "ccu0.stat.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data_should_return_success(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "ccu0.stat.0.slice_0"))
        self.assertEqual(result, NumberConstant.SUCCESS)
        self.assertEqual(128, len(check.channel_data))
        self.assertEqual([0, 4294967296, 4, 3, 2], check.channel_data[0])
        self.assertEqual([120, 2778843841158, 650, 649, 648], check.channel_data[120])
        self.assertEqual([124, 2916282794662, 682, 681, 680], check.channel_data[124])

    def test_read_binary_data_should_count_records_from_preprocessed_buffer(self):
        file_path = os.path.join(self.DATA_PATH, "ccu0.stat.0.slice_0")
        with open(file_path, "rb") as file:
            record = file.read()
        check = CCUChannelParser(self.file_list, self.CONFIG)
        check._source_calculator.pre_process = mock.Mock(return_value=record * 2)

        result = check.read_binary_data(file_path)

        self.assertEqual(NumberConstant.SUCCESS, result)
        self.assertEqual(256, len(check.channel_data))

    def test_parse(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        check.parse()

    def test_save(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        check.channel_data = [
            [0, 4919982785935, 25011, 25011, 25011],
            [1, 4919982785935, 4181298, 1435, 1068902],
            [2, 4919982785936, 13681, 13681, 13681]
        ]
        check.save()

    def test_ms_run(self):
        CCUChannelParser(self.file_list, self.CONFIG).ms_run()

    def test_failed_read_should_not_mark_source_complete(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        with mock.patch.object(check, 'read_binary_data', return_value=NumberConstant.ERROR), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file') as add_complete_file:
            check._handle_original_data('ccu2.stat.0.slice_0')
            check.save()

        add_complete_file.assert_not_called()

    def test_sqlite_integer_overflow_should_leave_source_incomplete(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        check.channel_data = [[0, NumberConstant.INT64_MAX + 1, 1, 1, 1]]
        check._pending_complete_files.append('ccu0.stat.0.slice_0')
        check._model = mock.MagicMock()

        self.assertFalse(check.save())
        check._model.__enter__.assert_not_called()
        self.assertEqual(['ccu0.stat.0.slice_0'], check._pending_complete_files)
