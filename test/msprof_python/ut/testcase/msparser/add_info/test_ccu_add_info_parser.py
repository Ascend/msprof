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

import os
import struct
import shutil
import unittest
from unittest import mock

from common_func.ms_constant.number_constant import NumberConstant
from common_func.info_conf_reader import InfoConfReader
from msparser.add_info.ccu_add_info_parser import CCUAddInfoParser
from msparser.add_info.ccu_add_info_bean import CCUGroupInfoBean
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag
from common_func.file_manager import FdOpen

NAMESPACE = 'msparser.add_info.ccu_add_info_parser'


class TestCcuAddInfoParser(unittest.TestCase):
    def test_reduce_product_should_match_native_mul(self):
        bean = CCUGroupInfoBean()
        bean._reduce_op_type = 1
        self.assertEqual("MUL", bean.reduce_op_type)

    file_list = {
        DataTag.CCU_TASK: [
            'unaging.additional.ccu_task_info.slice_0'
        ],
        DataTag.CCU_WAIT_SIGNAL: [
            'unaging.additional.ccu_wait_signal_info.slice_0'
        ],
        DataTag.CCU_GROUP: [
            'unaging.additional.ccu_group_info.slice_0'
        ],
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_add_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    DATA_PATH = os.path.join(DIR_PATH, "data")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    @classmethod
    def _reset_fixture(cls):
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)
        os.mkdir(cls.DIR_PATH)
        os.mkdir(cls.SQLITE_PATH)
        os.mkdir(cls.DATA_PATH)
        cls.make_ccu_task_info_data()
        cls.make_ccu_wait_signal_info_data()
        cls.make_ccu_group_info_data()

    def setUp(self):
        self._reset_fixture()

    @classmethod
    def tearDownClass(cls):
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)

    @classmethod
    def make_ccu_task_info_data(cls):
        ccu_task_info_data = (
            360471130, 14, 4549, 48, 4175640340, 276, 0, 1, 0, 0, 0, 9581505218827963271, 7415574198778220483, 0, 2, 1,
            0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.CCU_TASK_INFO_FMT, *ccu_task_info_data)
        with FdOpen(os.path.join(cls.DATA_PATH, "unaging.additional.ccu_task_info.slice_0"), operate="wb") as f:
            f.write(struct_data)

    @classmethod
    def make_ccu_wait_signal_info_data(cls):
        ccu_wait_signal_info_data = (
            360471130, 16, 4549, 152, 4175712479, 276, 0, 0, 0, 0, 0, 6823696491211891432, 7415574198778220483, 0, 2,
            1, 0, 1, 0, 1, 0, 39, 1, 0, 0, 0, 210, 2, 2, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
            65535, 65535, 65535, 65535, 65535, 65535, 1, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.CCU_WAIT_SIGNAL_INFO_FMT, *ccu_wait_signal_info_data)
        ccu_wait_signal_info_data2 = (
            360471130, 16, 4549, 152, 4175746632, 276, 0, 0, 0, 0, 0, 6823696491211891432, 7415574198778220483, 0, 2, 1,
            0, 1, 0, 1, 0, 40, 1, 0, 0, 0, 211, 2, 2, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
            65535, 65535, 65535, 65535, 65535, 65535, 1, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data += struct.pack(StructFmt.CCU_WAIT_SIGNAL_INFO_FMT, *ccu_wait_signal_info_data2)
        with FdOpen(os.path.join(cls.DATA_PATH, "unaging.additional.ccu_wait_signal_info.slice_0"), operate="wb") as f:
            f.write(struct_data)

    @classmethod
    def make_ccu_group_info_data(cls):
        ccu_group_info_data = (
            360471130, 15, 4549, 152, 4175841504, 276, 0, 0, 0, 0, 0, 1362511707695072317, 7415574198778220483, 0, 2, 1,
            0, 1, 0, 1, 0, 119, 255, 0, 2, 2, 8192, 2, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
            65535, 65535, 65535, 65535, 65535, 65535, 1, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.CCU_GROUP_INFO_FMT, *ccu_group_info_data)
        ccu_group_info_data2 = (
            360471130, 15, 4549, 152, 4175880241, 276, 0, 0, 0, 0, 0, 9129927687939955350, 7415574198778220483, 0, 2, 1,
            0, 1, 0, 1, 0, 197, 255, 255, 255, 255, 8192, 2, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
            65535, 65535, 65535, 65535, 65535, 65535, 65535, 1, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
            4294967295, 4294967295, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data += struct.pack(StructFmt.CCU_GROUP_INFO_FMT, *ccu_group_info_data2)
        with FdOpen(os.path.join(cls.DATA_PATH, "unaging.additional.ccu_group_info.slice_0"), operate="wb") as f:
            f.write(struct_data)

    def init_data(self):
        if os.path.exists(self.DATA_PATH):
            shutil.rmtree(self.DATA_PATH)
        if not os.path.exists(self.DATA_PATH):
            os.mkdir(self.DATA_PATH)
            self.make_ccu_task_info_data()
            self.make_ccu_wait_signal_info_data()
            self.make_ccu_group_info_data()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.CCUAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.CCUAddInfoParser.save'):
            check = CCUAddInfoParser(self.file_list, self.CONFIG)
            check.ms_run()

    def test_save(self):
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check.save()
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check._ccu_add_info_data.update({
            DataTag.CCU_TASK: [
                [0, 1, '9581505218827963271', '7415574198778220483', 0, 2, 1, 0, 1, 1, 0]
            ]
        })
        check._ccu_add_info_data.update({
            DataTag.CCU_WAIT_SIGNAL: [
                [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 38, 1, 209, 2, 2, 1],
                [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 39, 1, 210, 2, 2, 1],
                [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 40, 1, 211, 2, 2, 1],
                [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 205, 1, 208, 2, 2, 1]
            ]
        })
        check._ccu_add_info_data.update({
            DataTag.CCU_GROUP: [
                [0, '1362511707695072317', '7415574198778220483', 0, 2, 1, 1, 0, 1, 119, 255, 0, 2, 2, 8192, 2, 1],
                [0, '9129927687939955350', '7415574198778220483', 0, 2, 1, 1, 0, 1, 197, 255, 255, 255, 255, 8192, 2, 1]
            ]
        })
        check.save()

    def test_parse_should_return_ccu_task_info_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        task_data = check._ccu_add_info_data.get(DataTag.CCU_TASK)
        self.assertEqual(1, len(task_data))
        self.assertEqual([0, 1, '9581505218827963271', '7415574198778220483', 0, 2, 1, 0, 1, 1, 0], task_data[0])
        InfoConfReader()._info_json = {}
        self.init_data()

    def test_parse_should_return_wait_signal_info_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        wait_data = check._ccu_add_info_data.get(DataTag.CCU_WAIT_SIGNAL)
        self.assertEqual(2, len(wait_data))
        self.assertEqual([0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 39, 1, 210, 2, 2, 1],
                         wait_data[0])
        self.assertEqual([0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 40, 1, 211, 2, 2, 1],
                         wait_data[1])
        InfoConfReader()._info_json = {}
        self.init_data()

    def test_parse_should_return_group_info_data_when_data_exist(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check.parse()
        check.save()
        group_data = check._ccu_add_info_data.get(DataTag.CCU_GROUP)
        self.assertEqual(2, len(group_data))
        self.assertEqual([0, '1362511707695072317', '7415574198778220483',
                          0, 2, 1, 1, 0, 1, 119, 255, 'SUM', 'INT32', 'INT32', 8192, 2, 1], group_data[0])
        self.assertEqual([0, '9129927687939955350', '7415574198778220483',
                          0, 2, 1, 1, 0, 1, 197, 255, 'RESERVED', 'RESERVED', 'RESERVED', 8192, 2, 1], group_data[1])
        InfoConfReader()._info_json = {}
        self.init_data()

    def test_sqlite_integer_overflow_should_fail_before_opening_model(self):
        check = CCUAddInfoParser(self.file_list, self.CONFIG)
        check._ccu_add_info_data[DataTag.CCU_TASK] = [
            [0, 1, '1', '2', 0, 2, 1, 0, 1, 1, 0]
        ]
        check._ccu_add_info_data[DataTag.CCU_GROUP] = [
            [0, '1', '2', 0, 2, 1, 1, 0, 1, 119, 255, 'SUM', 'INT32', 'INT32',
             NumberConstant.INT64_MAX + 1, 2, 1]
        ]
        task_model = mock.MagicMock()
        group_model = mock.MagicMock()
        check._model_dict[DataTag.CCU_TASK] = task_model
        check._model_dict[DataTag.CCU_GROUP] = group_model

        with self.assertRaisesRegex(RuntimeError, "SQLite integer range"):
            check.save()

        task_model.__enter__.assert_not_called()
        group_model.__enter__.assert_not_called()

if __name__ == '__main__':
    unittest.main()
