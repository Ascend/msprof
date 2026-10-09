#!/usr/bin/python3
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

import unittest
from unittest import mock

from msmodel.step_trace.ts_track_model import TsTrackViewModel

NAMESPACE = "msmodel.step_trace.ts_track_model"


class TestTsTrackViewModel(unittest.TestCase):
    def test_get_hccl_operator_exe_data_without_ge(self):
        with mock.patch(NAMESPACE + ".TsTrackViewModel.attach_to_db", return_value=False):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_hccl_operator_exe_data(), [])

    def test_get_hccl_operator_exe_data(self):
        with mock.patch(NAMESPACE + ".TsTrackViewModel.attach_to_db", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[1]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_hccl_operator_exe_data(), [[1]])

    def test_get_ai_cpu_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[2]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_ai_cpu_data(), [[2]])

    def test_get_iter_time_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[3]]):
            with TsTrackViewModel("test") as _model:
                self.assertEqual(_model.get_iter_time_data(), [[3]])
