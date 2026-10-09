#!/usr/bin/env python
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2025 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from mock_tools import ClassMock
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.iter_rec.iter_info_updater.iter_info_manager import IterInfoManager

NAMESPACE = "msparser.iter_rec.iter_info_updater.iter_info_manager"


class TestIterInfoUpdater(unittest.TestCase):
    def test_initial_iter_to_info_1(self: any) -> None:
        with mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.init"), \
                mock.patch("common_func.msvp_common.path_check", return_value=False):
            IterInfoManager("").initial_iter_to_info()

    def test_initial_iter_to_info_2(self: any) -> None:
        with mock.patch("common_func.msvp_common.path_check", return_value=True), \
                mock.patch("common_func.path_manager.PathManager.get_db_path", return_value='test'):
            ge_mock = mock.Mock
            ge_mock.__enter__ = GeInfoModel.__enter__
            ge_mock.__exit__ = GeInfoModel.__exit__
            with ClassMock(GeInfoModel, ge_mock()):
                IterInfoManager.initial_iter_to_info(mock.Mock())

    def test_register_parallel_set(self: any) -> None:
        instance = mock.Mock()
        instance.iter_to_iter_info = {}
        step_trace_datum = mock.Mock()

        step_trace_datum.model_id = 1
        step_trace_datum.index_id = 1
        step_trace_datum.iter_id = 1
        step_trace_datum.step_start = 1
        step_trace_datum.step_end = 1

        step_trace_data = [step_trace_datum, step_trace_datum]

        IterInfoManager.register_parallel_set(instance, step_trace_data)

    def test_register_aicore_set(self: any) -> None:
        instance = mock.Mock()
        iter_info_bean = mock.Mock()
        iter_info_bean.model_id = 1
        iter_info_bean.index_id = 2
        instance.iter_to_iter_info = {1: iter_info_bean}

        IterInfoManager.register_aicore_set(instance, {}, {})

    def test_check_parallel_1(self: any) -> None:
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_data", return_value=[]):
            check = IterInfoManager("")
            self.assertEqual(False, check.check_parallel(""))

    def test_check_parallel_2(self: any) -> None:
        step_trace_data1 = mock.Mock()
        step_trace_data1.step_start = 1
        step_trace_data1.step_end = 3
        step_trace_data2 = mock.Mock()
        step_trace_data2.step_start = 2
        step_trace_data2.step_end = 4
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_data",
                           return_value=[step_trace_data1, step_trace_data2]):
            check = IterInfoManager("")
            self.assertEqual(True, check.check_parallel(""))

    def test_check_parallel_3(self: any) -> None:
        step_trace_data1 = mock.Mock()
        step_trace_data1.step_start = 1
        step_trace_data1.step_end = 3
        step_trace_data3 = mock.Mock()
        step_trace_data3.step_start = 5
        step_trace_data3.step_end = 9
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + ".TsTrackModel.get_step_trace_data",
                           return_value=[step_trace_data1, step_trace_data3]):
            check = IterInfoManager("")
            self.assertEqual(False, check.check_parallel(""))
