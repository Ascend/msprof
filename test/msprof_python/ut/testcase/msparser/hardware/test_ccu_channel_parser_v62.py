import os
import shutil
import sqlite3
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from msparser.hardware.ccu_channel_parser import CCUChannelParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


class TestCcuChannelParserV62(unittest.TestCase):
    file_list = {DataTag.CCU_CHANNEL: ['ccu0.stat.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_channel_v62")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'chip_model': 16, 'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }
    _chip_id = None

    @classmethod
    def setUpClass(cls):
        cls._chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        InfoConfReader()._info_json = {"platform_version": "16"}
        if not os.path.exists(cls.DATA_PATH):
            os.makedirs(cls.DATA_PATH)
            os.makedirs(cls.SQLITE_PATH)
            cls.make_ccu_channel_data()

    @classmethod
    def tearDownClass(cls):
        ChipManager().chip_id = cls._chip_id
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_channel_data(cls):
        words = [0] * 96
        channel_ids = list(range(100, 132))
        for idx in range(10):
            base = idx * 3
            words[idx] = channel_ids[base] | (channel_ids[base + 1] << 10) | (channel_ids[base + 2] << 20)
        words[10] = channel_ids[30] | (channel_ids[31] << 10)
        words[11] = 7
        words[12] = 0x12345678
        words[13] = 0x1ABCDEF0
        metric_start = 14
        for idx in range(32):
            words[metric_start + idx * 2] = 1000 + idx
            words[metric_start + idx * 2 + 1] = 2000 + idx
        for idx in range(78, 96):
            words[idx] = 0xFFFFFFFF
        data = struct.pack("=96I", *words)
        with FdOpen(os.path.join(cls.DATA_PATH, "ccu0.stat.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data_should_return_success(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        with self.assertLogs(level='INFO') as logs:
            result = check.read_binary_data(os.path.join(self.DATA_PATH, "ccu0.stat.0.slice_0"))
        self.assertEqual(result, NumberConstant.SUCCESS)
        self.assertEqual(3, len(check.channel_data))
        self.assertEqual([100, str(0x1ABCDEF012345678 * 4), 2000, 1000, 0], check.channel_data[0])
        self.assertEqual([102, str(0x1ABCDEF012345678 * 4), 2002, 1002, 0], check.channel_data[-1])
        self.assertEqual(1, len(logs.output))
        self.assertIn("Filtered 29 invalid CCU channel v6_2 rows", logs.output[0])

    def test_ms_run_should_persist_v62_records_to_sqlite(self):
        check = CCUChannelParser(self.file_list, self.CONFIG)
        check.ms_run()

        db_path = os.path.join(self.SQLITE_PATH, "ccu.db")
        conn = sqlite3.connect(db_path)
        try:
            cur = conn.cursor()
            rows = cur.execute(
                "select channel_id, timestamp, max_delay, avg_delay, die_id from OriginChannelV62 "
                "order by channel_id asc"
            ).fetchall()
        finally:
            conn.close()

        self.assertEqual(3, len(rows))
        self.assertEqual((100, str(0x1ABCDEF012345678 * 4), 2000, 1000, 0), rows[0])
        self.assertEqual((102, str(0x1ABCDEF012345678 * 4), 2002, 1002, 0), rows[-1])
