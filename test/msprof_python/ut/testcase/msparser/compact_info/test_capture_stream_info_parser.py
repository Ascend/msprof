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
import sqlite3
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.compact_info.capture_stream_info_bean import CaptureStreamInfoBean
from msparser.compact_info.capture_stream_info_parser import CaptureStreamInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.compact_info.capture_stream_info_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.capture_stream_info_model'


class TestTaskTrackParser(unittest.TestCase):
    file_list = {DataTag.CAPTURE_STREAM_INFO: ['unaging.compact.capture_stream_info.slice_0']}

    def test_format_stream_data(self):
        check = CaptureStreamInfoParser(self.file_list, CONFIG)
        capture_stream_data = [
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 21, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 22, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 22, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 32, 20, 0, 1]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 1, 0xffff, 0xffff, 0, 0]),
        ]
        check._format_stream_data(capture_stream_data)
        self.assertEqual(len(check._capture_stream_data), 4)

    def test_format_mc2_comm_info_data(self):
        check = CaptureStreamInfoParser(self.file_list, CONFIG)
        # 无Mc2CommInfo db时
        check._format_mc2_comm_info_data()

        capture_stream_data = [
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 21, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 22, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 22, 20, 0, 0]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 32, 20, 0, 1]),
            CaptureStreamInfoBean([23130, 5000, 1000, 270722, 12, 75838889645892, 1, 0xffff, 0xffff, 0, 0]),
        ]
        check._format_stream_data(capture_stream_data)
        mc2_comm_info_db_data = [["5862276093215481612", 2, 0, 0, 20, "100,101"]]
        except_capture_mc2_comm_info_data = [['5862276093215481612', 2, 0, 0, 32, '100,101'],
                                             ['5862276093215481612', 2, 0, 0, 21, '100,101'],
                                             ['5862276093215481612', 2, 0, 0, 22, '100,101']]
        with mock.patch('msmodel.add_info.mc2_comm_info_model.Mc2CommInfoViewModel.init'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model.Mc2CommInfoViewModel.check_db',
                           return_value=True), \
                mock.patch('msmodel.add_info.mc2_comm_info_model.Mc2CommInfoViewModel.check_table',
                           return_value=True), \
                mock.patch(MODEL_NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(MODEL_NAMESPACE + '.DBManager.fetch_all_data', return_value=mc2_comm_info_db_data):
            check._format_mc2_comm_info_data()
            self.assertEqual(len(check._capture_mc2_comm_info_data), 3)
            self.assertEqual(check._capture_mc2_comm_info_data, except_capture_mc2_comm_info_data)

    def test_parse(self):
        with mock.patch(NAMESPACE + '.CaptureStreamInfoParser.parse_bean_data'):
            check = CaptureStreamInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.CaptureStreamInfoModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.CaptureStreamInfoModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.CaptureStreamInfoModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.CaptureStreamInfoModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.CaptureStreamInfoModel.finalize'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model' + '.Mc2CommInfoModel.init'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model' + '.Mc2CommInfoModel.check_db'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model' + '.Mc2CommInfoModel.create_table'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model' + '.Mc2CommInfoModel.insert_data_to_db'), \
                mock.patch('msmodel.add_info.mc2_comm_info_model' + '.Mc2CommInfoModel.finalize'):
            check = CaptureStreamInfoParser(self.file_list, CONFIG)
            check._capture_stream_data = [[0, 21, 20, 0, 0]]
            check._capture_mc2_comm_info_data = [["5862276093215481612", 2, 0, 0, 81, "100,101"]]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.CaptureStreamInfoParser.parse'), \
                mock.patch(NAMESPACE + '.CaptureStreamInfoParser.format_data'), \
                mock.patch(NAMESPACE + '.CaptureStreamInfoParser.save', side_effect=ValueError("Error")):
            CaptureStreamInfoParser(self.file_list, CONFIG).ms_run()
        CaptureStreamInfoParser({DataTag.CAPTURE_STREAM_INFO: []}, CONFIG).ms_run()
