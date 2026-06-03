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
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from msmodel.stars.fusion_task_model import FusionTaskModel
from msparser.stars.fusion_task_parser import FusionTaskParser
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.stars.fusion_task_bean import FusionTaskBean

NAMESPACE = 'msparser.stars.fusion_task_parser'

SAMPLE_CONFIG = {
    "model_id": 1,
    'iter_id': '1919',
    'result_dir': '114514',
    "ai_core_profiling_mode": "task-based",
    "aiv_profiling_mode": "sample-based",
}


class TestFusionTaskParser(unittest.TestCase):

    def setUp(self) -> None:
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0

    def test_init(self):
        parser = FusionTaskParser(SAMPLE_CONFIG.get('result_dir'), 'test.db', [])
        self.assertEqual(parser._start_functype, '010110')
        self.assertEqual(parser._end_functype, '010111')
        self.assertIsInstance(parser._model, FusionTaskModel)
        self.assertEqual(parser._decoder, FusionTaskBean)
        self.assertEqual(parser._data_list, [])

    def test_preprocess_data(self):
        result_dir = ''
        db = ''
        table_list = ['']
        with mock.patch(NAMESPACE + '.FusionTaskParser.get_task_time', return_value=([1], [2])):
            parser = FusionTaskParser(result_dir, db, table_list)
            parser.preprocess_data()
            self.assertEqual(parser._data_list, [1])
            self.assertEqual(parser._mismatch_task, [2])

    def test_handle_valid_magic(self):
        result_dir = ''
        db = ''
        table_list = ['']
        mock_bean = mock.MagicMock()
        mock_bean.magic = FusionTaskBean.MAGIC_NUM
        parser = FusionTaskParser(result_dir, db, table_list)
        with mock.patch(NAMESPACE + '.FusionTaskBean.decode', return_value=mock_bean):
            parser.handle(None, b'\x00' * 32)
        self.assertEqual(len(parser._data_list), 1)
        self.assertIs(parser._data_list[0], mock_bean)

    def test_handle_invalid_magic(self):
        result_dir = ''
        db = ''
        table_list = ['']
        mock_bean = mock.MagicMock()
        mock_bean.magic = 0x0000
        parser = FusionTaskParser(result_dir, db, table_list)
        with mock.patch(NAMESPACE + '.FusionTaskBean.decode', return_value=mock_bean):
            parser.handle(None, b'\x00' * 32)
        self.assertEqual(len(parser._data_list), 0)

    def test_get_task_time_fully_matched(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_start_1 = [22, 0x6BD3, 100, 56, 1, 3, 0, 0, 0]
        args_end_1 = [23, 0x6BD3, 100, 60, 1, 3, 0, 0, 0]
        args_start_2 = [22, 0x6BD3, 200, 80, 2, 7, 0, 0, 0]
        args_end_2 = [23, 0x6BD3, 200, 90, 2, 7, 0, 0, 0]
        parser = FusionTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        parser._data_list = [
            FusionTaskBean(args_start_1), FusionTaskBean(args_end_1),
            FusionTaskBean(args_start_2), FusionTaskBean(args_end_2),
        ]
        ret = parser.get_task_time()
        expect_ret = (
            [
                [0, 100, 3, 'AI_CORE', 1120.0, 1200.0, 80.0, '0000000000000001'],
                [0, 200, 7, 'AI_CORE', 1600.0, 1800.0, 200.0, '0000000000000010'],
            ],
            [],
        )
        self.assertEqual(ret, expect_ret)

    def test_get_task_time_end_before_start_mismatch(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_start_1 = [22, 0x6BD3, 100, 56, 1, 3, 0, 0, 0]
        args_end_1 = [23, 0x6BD3, 100, 50, 1, 3, 0, 0, 0]
        parser = FusionTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        parser._data_list = [
            FusionTaskBean(args_start_1), FusionTaskBean(args_end_1),
        ]
        ret = parser.get_task_time()
        self.assertEqual(ret[0], [])
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 100)

    def test_get_task_time_remaining_start(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_start_1 = [22, 0x6BD3, 100, 56, 1, 3, 0, 0, 0]
        parser = FusionTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        parser._data_list = [FusionTaskBean(args_start_1)]
        ret = parser.get_task_time()
        self.assertEqual(ret[0], [])
        self.assertEqual(len(ret[1]), 1)
        self.assertEqual(ret[1][0].task_id, 100)

    def test_get_task_time_extra_end_discarded(self):
        result_dir = ''
        db = ''
        table_list = ['']
        args_start_1 = [22, 0x6BD3, 100, 56, 1, 3, 0, 0, 0]
        args_end_1 = [23, 0x6BD3, 100, 60, 1, 3, 0, 0, 0]
        args_end_2 = [23, 0x6BD3, 100, 70, 1, 3, 0, 0, 0]
        parser = FusionTaskParser(result_dir, db, table_list)
        InfoJsonReaderManager(info_json=InfoJson(DeviceInfo=[DeviceInfo(hwts_frequency=50).device_info])).process()
        parser._data_list = [
            FusionTaskBean(args_start_1), FusionTaskBean(args_end_1),
            FusionTaskBean(args_end_2),
        ]
        ret = parser.get_task_time()
        expect_ret = (
            [[0, 100, 3, 'AI_CORE', 1120.0, 1200.0, 80.0, '0000000000000001']],
            [],
        )
        self.assertEqual(ret, expect_ret)

    def test_flush_empty(self):
        result_dir = ''
        db = ''
        table_list = ['']
        parser = FusionTaskParser(result_dir, db, table_list)
        parser.flush()

    def test_flush(self):
        result_dir = ''
        db = ''
        table_list = ['']
        parser = FusionTaskParser(result_dir, db, table_list)
        parser._data_list = [1, 2]
        with mock.patch(NAMESPACE + '.FusionTaskModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.FusionTaskParser.preprocess_data'), \
                mock.patch(NAMESPACE + '.FusionTaskModel.flush'), \
                mock.patch(NAMESPACE + '.FusionTaskModel.finalize'):
            parser.flush()


if __name__ == '__main__':
    unittest.main()
