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
import unittest
from unittest import mock

from common_func.platform.chip_manager import ChipManager
from msparser.stars.acsq_task_parser import AcsqTaskParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.stars.acsq_task import AcsqTask
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'msparser.stars.acsq_task_parser'


class TestAcsqTaskParser(unittest.TestCase):

    def setUp(self) -> None:
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0

    def test_preprocess_data(self):
        result_dir = ''
        db = ''
        table_list = ['']
        with mock.patch(NAMESPACE + '.AcsqTaskParser.get_task_time', return_value=[1, 2]):
            key = AcsqTaskParser(result_dir, db, table_list)
            key.preprocess_data()

    def test_get_task_time_should_return_2_matched_task_and_no_remaining_when_no_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2), AcsqTask(args_end_2)]
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 25, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 8, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_1_end_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_end_3 = [1, 5, 67, 7, 93, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_end_3)
        ]
        ret = key.get_task_time()
        expect_ret = (
            [
                [3, 4, 25, 'AI_CORE', 1120.0, 1160.0, 40.0],
                [67, 7, 8, 'AI_CORE', 1780.0, 1820.0, 40.0]
            ],
            []
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_should_return_2_matched_task_and_1_remaining_when_1_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [3, 4, 25, 'AI_CORE', 1120.0, 1160.0, 40.0],
            [67, 7, 8, 'AI_CORE', 1780.0, 1820.0, 40.0]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_should_return_2_matched_and_1_start_remained_when_extra_start_group_has_no_ends(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        args_begin_4 = [0, 5, 67, 1000, 95, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3), AcsqTask(args_begin_4)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [3, 4, 25, 'AI_CORE', 1120.0, 1160.0, 40.0],
            [67, 7, 8, 'AI_CORE', 1780.0, 1820.0, 40.0]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_should_return_2_matched_and_1_start_remained_when_end_less_than_start(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_end_0 = [1, 5, 67, 7, 53, 47, 8, 99]
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        args_begin_2 = [0, 5, 67, 7, 89, 47, 8, 99]
        args_end_2 = [1, 5, 67, 7, 91, 47, 8, 99]
        args_begin_3 = [0, 5, 67, 1000, 93, 47, 8, 99]
        args_begin_4 = [0, 5, 67, 1000, 95, 47, 8, 99]
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            AcsqTask(args_end_0), AcsqTask(args_begin_1), AcsqTask(args_end_1), AcsqTask(args_begin_2),
            AcsqTask(args_end_2), AcsqTask(args_begin_3), AcsqTask(args_begin_4)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [3, 4, 25, 'AI_CORE', 1120.0, 1160.0, 40.0],
            [67, 7, 8, 'AI_CORE', 1780.0, 1820.0, 40.0]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_lookahead_middle_end_lost_multiple_starts(self):
        """Scenario: start1, start2, start3, end1, end3 (end2 lost).
        LIFO would incorrectly match start3<->end1 when end1 arrives.
        Lookahead correctly identifies start1/start2 as orphans."""
        result_dir = ''
        db = ''
        table_list = ['']
        s1 = [0, 2, 67, 7, 89, 47, 8, 99]   # t=1780
        s2 = [0, 5, 67, 7, 90, 47, 8, 99]   # t=1800, end2 lost
        s3 = [0, 2, 67, 7, 91, 47, 8, 99]   # t=1820
        e1 = [1, 2, 67, 7, 93, 47, 8, 99]   # t=1860
        e3 = [1, 5, 67, 7, 94, 47, 8, 99]   # t=1880
        key = AcsqTaskParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            AcsqTask(s1), AcsqTask(s2), AcsqTask(s3),
            AcsqTask(e1), AcsqTask(e3)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [67, 7, 8, 'AI_CORE', 1820.0, 1860.0, 40.0]  # s3<->e1
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 0)

    def test_flush(self):
        result_dir = ''
        db = ''
        table_list = ['']
        key = AcsqTaskParser(result_dir, db, table_list)
        key.flush()
        args_begin_1 = [0, 2, 3, 4, 56, 7, 89, 9]
        args_end_1 = [1, 2, 3, 4, 58, 7, 89, 9]
        key._data_list = [
            AcsqTask(args_begin_1), AcsqTask(args_end_1)
        ]
        with mock.patch(NAMESPACE + '.AcsqTaskModel.init'), \
                mock.patch(NAMESPACE + ".AcsqTaskParser.preprocess_data"), \
                mock.patch(NAMESPACE + ".AcsqTaskModel.flush"), \
                mock.patch(NAMESPACE + ".AcsqTaskModel.finalize"):
            key.flush()


if __name__ == '__main__':
    unittest.main()
