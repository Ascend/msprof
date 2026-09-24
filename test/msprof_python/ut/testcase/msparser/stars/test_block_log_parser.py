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
from msparser.stars import BlockLogParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.stars.block_log_bean import BlockLogBean
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'msparser.stars.block_log_parser'


class TestBlockLogParser(unittest.TestCase):
    def test_preprocess_data(self):
        result_dir = ''
        db = ''
        table_list = ['']
        with mock.patch(NAMESPACE + '.BlockLogParser.get_task_time', return_value=[1, 2]):
            key = BlockLogParser(result_dir, db, table_list)
            key.preprocess_data()

    def test_get_task_time_should_return_2_matched_task_and_no_remaining_when_no_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1),
            BlockLogBean(args_begin_2), BlockLogBean(args_end_2)
        ]
        ret = key.get_task_time()
        expect_ret = (
            [
                [4, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
                [7, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
            ],
            []
        )
        self.assertEqual(expect_ret, ret)

    def test_get_task_time_should_return_2_matched_task_and_0_remaining_when_1_end_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_3 = [37, 5, 7, 0, 93, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1), BlockLogBean(args_begin_2),
            BlockLogBean(args_end_2), BlockLogBean(args_end_3)
        ]
        ret = key.get_task_time()
        expect_ret = (
            [
                [4, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
                [7, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
            ],
            []
        )
        self.assertEqual(expect_ret, ret)

    def test_get_task_time_should_return_2_matched_task_and_1_remaining_when_1_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        args_begin_3 = [36, 5, 1000, 0, 93, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1), BlockLogBean(args_begin_2),
            BlockLogBean(args_end_2), BlockLogBean(args_begin_3)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [4, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
            [7, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_should_return_2_matched_task_and_1_remaining_when_2_start_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        args_begin_3 = [36, 5, 1000, 0, 93, 47, 8, 99, 1, 9, 10, 11, 22]
        args_begin_4 = [36, 5, 1000, 0, 95, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1), BlockLogBean(args_begin_2),
            BlockLogBean(args_end_2), BlockLogBean(args_begin_3), BlockLogBean(args_begin_4)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [4, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
            [7, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_should_return_2_matched_task_and_1_remaining_when_end_less_than_start(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_end_0 = [37, 5, 7, 7, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        args_begin_3 = [36, 5, 1000, 0, 93, 47, 8, 99, 1, 9, 10, 11, 22]
        args_begin_4 = [36, 5, 1000, 0, 95, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_end_0), BlockLogBean(args_begin_1), BlockLogBean(args_end_1),
            BlockLogBean(args_begin_2),
            BlockLogBean(args_end_2), BlockLogBean(args_begin_3), BlockLogBean(args_begin_4)
        ]
        ret = key.get_task_time()
        expect_ret = [
            [4, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
            [7, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
        ]
        self.assertEqual(ret[0], expect_ret)
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 1000)

    def test_get_task_time_and_set_stream_id_should_return_2_matched_task_and_no_remaining_when_no_task_mismatched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_begin_1 = [36, 2, 4, 0, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 4, 0, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        args_begin_2 = [36, 5, 7, 0, 89, 47, 8, 99, 1, 9, 10, 11, 22]
        args_end_2 = [37, 5, 7, 0, 91, 47, 8, 99, 1, 9, 10, 11, 22]
        key = BlockLogParser(result_dir, db, table_list)
        InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info]).apply()
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1),
            BlockLogBean(args_begin_2), BlockLogBean(args_end_2)
        ]
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        with mock.patch('mscalculate.ascend_task.host_task_collector.HostTaskCollector.get_host_task_stream_table',
                        return_value={4: 1000}):
            key._set_stream_id_by_host()
            ret = key.get_task_time()
        expect_ret = (
            [
                [1000, 4, 12, 'AI_CORE', 56, 58, 40.0, 1, 9],
                [65535, 7, 22, 'AI_CORE', 89, 91, 40.0, 1, 9]
            ],
            []
        )
        self.assertEqual(expect_ret, ret)

    def test_flush(self):
        result_dir = ''
        db = ''
        table_list = ['']
        key = BlockLogParser(result_dir, db, table_list)
        key.flush()
        args_begin_1 = [36, 2, 3, 4, 56, 7, 89, 9, 1, 9, 10, 11, 12]
        args_end_1 = [37, 2, 3, 4, 58, 7, 89, 9, 1, 9, 10, 11, 12]
        key._data_list = [
            BlockLogBean(args_begin_1), BlockLogBean(args_end_1)
        ]
        with mock.patch(NAMESPACE + '.BlockLogModel.init'), \
                mock.patch(NAMESPACE + ".BlockLogParser.preprocess_data"), \
                mock.patch(NAMESPACE + ".BlockLogModel.flush"), \
                mock.patch(NAMESPACE + ".BlockLogModel.finalize"):
            key.flush()


if __name__ == '__main__':
    unittest.main()
