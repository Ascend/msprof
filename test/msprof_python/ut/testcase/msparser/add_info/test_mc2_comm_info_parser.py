#!/usr/bin/python3
# -*- coding: utf-8 -*-
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

from msparser.data_struct_size_constant import StructFmt
from msparser.add_info.mc2_comm_info_bean import Mc2CommInfoBean
from msparser.add_info.mc2_comm_info_parser import Mc2CommInfoParser
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.mc2_comm_info_parser'


class TestMc2CommInfoParser(unittest.TestCase):
    file_list = {
        DataTag.MC2_COMM_INFO: [
            'aging.additional.mc2_comm_info.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "mc2_comm_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    def setup_class(self):
        if not os.path.exists(self.DIR_PATH):
            os.mkdir(self.DIR_PATH)
        if not os.path.exists(self.SQLITE_PATH):
            os.mkdir(self.SQLITE_PATH)

    def teardown_class(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)

    def test_parse_should_return_1_mc2_comm_info_data_when_256_bytes_in_file(self):
        mc2_comm_info_data = [
            23130, 10000, 0, 1, 128, 2000,
            7466789422691968299, 8, 0, 0, 1, 8, 52, 53, 54, 55, 56, 57, 58, 59
        ] + [0] * 43
        struct_data = struct.pack(StructFmt.MC2_COMM_INFO_FMT, *mc2_comm_info_data)
        data = Mc2CommInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.Mc2CommInfoParser.parse_bean_data', return_value=[data]):
            check = Mc2CommInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        self.assertEqual(1, len(check._communication_info))
        self.assertEqual("7466789422691968299", check._communication_info[0].group_name)

    def test_reformat_data_should_append_invalid_stream_when_chip_v6_level0(self):
        check = Mc2CommInfoParser(self.file_list, self.CONFIG)
        bean = mock.Mock()
        bean.group_name = "1"
        bean.rank_size = 2
        bean.rank_id = 0
        bean.usr_rank_id = 0
        bean.stream_id = 20
        bean.comm_stream_ids = "100,101"
        check._communication_info = [bean]
        with mock.patch(NAMESPACE + '.ChipManager') as mock_chip, \
                mock.patch(NAMESPACE + '.InfoConfReader') as mock_info:
            mock_chip.return_value.is_chip_v6.return_value = True
            mock_info.return_value.is_level0.return_value = True
            rows = check.reformat_data()
        self.assertEqual(2, len(rows))
        self.assertEqual(20, rows[0][4])
        self.assertEqual(20, rows[1][4])
        self.assertEqual("100,101", rows[0][5])
        self.assertEqual(str(Mc2CommInfoParser.INVALID_STREAM_ID), rows[1][5])
        self.assertEqual(rows[0][0], rows[1][0])
        self.assertEqual(rows[0][1:4], rows[1][1:4])

    def test_reformat_data_should_append_one_invalid_stream_row_per_stream_when_chip_v6_level0(self):
        check = Mc2CommInfoParser(self.file_list, self.CONFIG)
        bean1 = mock.Mock()
        bean1.group_name = "1"
        bean1.rank_size = 2
        bean1.rank_id = 0
        bean1.usr_rank_id = 0
        bean1.stream_id = 20
        bean1.comm_stream_ids = "100,101"
        bean2 = mock.Mock()
        bean2.group_name = "1"
        bean2.rank_size = 2
        bean2.rank_id = 0
        bean2.usr_rank_id = 0
        bean2.stream_id = 20
        bean2.comm_stream_ids = "100"
        check._communication_info = [bean1, bean2]
        with mock.patch(NAMESPACE + '.ChipManager') as mock_chip, \
                mock.patch(NAMESPACE + '.InfoConfReader') as mock_info:
            mock_chip.return_value.is_chip_v6.return_value = True
            mock_info.return_value.is_level0.return_value = True
            rows = check.reformat_data()
        self.assertEqual(3, len(rows))
        self.assertEqual(20, rows[0][4])
        self.assertEqual(20, rows[1][4])
        self.assertEqual(20, rows[2][4])
        self.assertEqual("100,101", rows[0][5])
        self.assertEqual(str(Mc2CommInfoParser.INVALID_STREAM_ID), rows[1][5])
        self.assertEqual("100", rows[2][5])
