import os
import shutil
import sqlite3
import struct
import unittest

from common_func.file_manager import FdOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag


class TestCcuMissionParserV62(unittest.TestCase):
    file_list = {DataTag.CCU_MISSION: ['ccu0.instr.0.slice_0']}
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_mission_v62")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'chip_model': 16,
        'result_dir': DIR_PATH,
        'device_id': '0',
        'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default',
        'model_id': -1,
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
            cls.make_ccu_mission_data()

    @classmethod
    def tearDownClass(cls):
        ChipManager().chip_id = cls._chip_id
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_mission_data(cls):
        setckb_words = (
            (9 << 16) | 3,
            (5 << 19) | (7 << 9) | (1 << 4) | (2 << 2) | (1 << 1) | 1,
            0x12345678,
            0x89ABCDEF,
            0x01234567,
            0x11111111,
            0x22222222,
            0x55AA,
        )
        loopgroup_words = ((10 << 16) | 4, (0x1234 << 16), 0x33333333, 0x04444444, 0x55555555, 0x06666666, 0, 0)
        data = struct.pack("=8I", *setckb_words) + struct.pack("=8I", *loopgroup_words)
        with FdOpen(os.path.join(cls.DATA_PATH, "ccu0.instr.0.slice_0"), operate="wb") as f:
            f.write(data)

    def test_read_binary_data_should_split_v62_records(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        result = check.read_binary_data(os.path.join(self.DATA_PATH, "ccu0.instr.0.slice_0"))
        self.assertEqual(result, NumberConstant.SUCCESS)
        self.assertEqual(1, len(check.setckb_v62_data))
        self.assertEqual(1, len(check.loopgroup_v62_data))
        self.assertEqual(
            [0x90003, 0x5678, 7, 5, 0x1234, 0x0123456789ABCDEF * 4, str(0x2222222211111111), 1, 2, 1, 0],
            check.setckb_v62_data[0],
        )
        self.assertEqual(
            [0xA0004, 0x1234, 0x0444444433333333 * 4, 0x0666666655555555 * 4, 0], check.loopgroup_v62_data[0]
        )

    def test_ms_run_should_persist_v62_records_to_sqlite(self):
        check = CCUMissionParser(self.file_list, self.CONFIG)
        check.ms_run()

        db_path = os.path.join(self.SQLITE_PATH, "ccu.db")
        conn = sqlite3.connect(db_path)
        try:
            cur = conn.cursor()
            setckb_rows = cur.execute(
                "select task_id, setckb_instr_id, loop_id, loop_cnt, setckb_ckbinfo, setckb_syscnt, "
                "udf_profiling_data, udf_flag, syscnt_type, lp_flg, die_id from OriginSetCKBV62"
            ).fetchall()
            loopgroup_rows = cur.execute(
                "select task_id, loopgroup_instr_id, loopgroup_start_syscnt, "
                "loopgroup_stop_syscnt, die_id from OriginLoopGroupV62"
            ).fetchall()
        finally:
            conn.close()

        self.assertEqual(
            [(0x90003, 0x5678, 7, 5, str(0x1234), 0x0123456789ABCDEF * 4, str(0x2222222211111111), 1, 2, 1, 0)],
            setckb_rows,
        )
        self.assertEqual([(0xA0004, 0x1234, 0x0444444433333333 * 4, 0x0666666655555555 * 4, 0)], loopgroup_rows)
