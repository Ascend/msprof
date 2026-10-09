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
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import sqlite3
import struct
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from msparser.iter_rec.iter_rec_parser import NoGeIterRecParser
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'msparser.iter_rec.iter_rec_parser'


def get_step_trace_data():
    step_trace_dto = StepTraceDto()
    step_trace_dto.step_start = 1
    step_trace_dto.step_end = 3
    step_trace_dto.iter_id = 1
    return step_trace_dto


class TestNoGeIterRecParser(unittest.TestCase):
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0'], DataTag.AI_CORE: ['aicore.data.0.slice_0'], DataTag.GE_TASK: []}

    def test_get_iter_end_dict(self):
        pass

    def test_set_current_iter_id(self):
        step_trace_dto = StepTraceDto()
        step_trace_dto.step_start = 1
        step_trace_dto.step_end = 101
        step_trace_dto.iter_id = 1
        with mock.patch('msmodel.step_trace.ts_track_model.TsTrackModel.get_step_trace_data',
                        return_value=[step_trace_dto]), \
                mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check._iter_recorder.set_current_iter_id(100)
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={}):
            with pytest.raises(ProfException) as err:
                check = NoGeIterRecParser(self.file_list, CONFIG)
                check._iter_recorder.set_current_iter_id(100)
            self.assertEqual(err.value.code, 8)

    def test_read_hwts_data(self):
        data = struct.pack("=BBHHHQ12IBBHHHQ12IBBHHHQ12I",
                           1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6,
                           1, 2, 3, 4, 5, 0, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6,
                           1, 2, 3, 4, 5, 2, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6)

        with mock.patch("common_func.iter_recorder.IterRecorder.check_task_before_max_iter", return_value=True), \
                mock.patch("common_func.iter_recorder.IterRecorder.set_current_iter_id"), \
                mock.patch(NAMESPACE + ".IterParser._create_hwts_task_time_data"), \
                mock.patch(
                    "msparser.iter_rec.iter_info_updater.iter_info_updater.IterInfoUpdater.update_count_and_offset"), \
                mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_step_trace_data",
                           return_value=[get_step_trace_data()]), \
                mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check._read_hwts_data(bytes(data))
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        InfoConfReader()._info_json = {
            "devices": "0",
            "drvVersion": InfoConfReader().ALL_EXPORT_VERSION,
            "DeviceInfo": [{'hwts_frequency': 100}],
        }
        with mock.patch("common_func.iter_recorder.IterRecorder.check_task_before_max_iter", return_value=True), \
                mock.patch("common_func.iter_recorder.IterRecorder.set_current_iter_id"), \
                mock.patch(NAMESPACE + ".IterParser._create_hwts_task_time_data"), \
                mock.patch(
                    "msparser.iter_rec.iter_info_updater.iter_info_updater.IterInfoUpdater.update_count_and_offset"), \
                mock.patch(NAMESPACE + ".IterRecorder.check_task_in_iter", return_value=True):
            check._read_hwts_data(bytes(data))
        InfoConfReader()._info_json = {}

    def test_read_hwts_data_should_not_count_tasks_when_exceeding_max_iter(self):
        data = struct.pack("=BBHHHQ12IBBHHHQ12IBBHHHQ12IBBHHHQ12I",
                           0, 2, 3, 4, 5, 0, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6,  # syscnt 0, not in iter
                           0, 2, 3, 4, 5, 1, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6,  # syscnt 1, in iter
                           1, 2, 3, 4, 5, 2, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6,  # syscnt 2, in iter
                           0, 2, 3, 4, 5, 4, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6)  # syscnt 4, exceeds max iter

        with mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_step_trace_data",
                           return_value=[get_step_trace_data()]), \
                mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            iter_info = \
                check._iter_info_updater.iteration_manager.iter_to_iter_info.setdefault(1, IterInfo(1, 1, 1, 0, 3))
            iter_info.behind_parallel_iter.add(1)
            check._read_hwts_data(bytes(data))
            self.assertEqual(iter_info.hwts_count, 2)
            self.assertEqual({1: 1, 2: 1}, check._task_cnt_not_in_iter)  # count syscnt 0, not count syscnt 4

    def test_read_ai_core_data(self):
        data = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6, 1, 2, 3, 4, 5, 6,
                           7)
        check = NoGeIterRecParser(self.file_list, CONFIG)
        check._read_ai_core_data(bytes(data))

    def test_parse_hwts_data(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        InfoConfReader()._info_json = {"devices": "0", "drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
        data = struct.pack("=BBHHHQ12I", 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.NoGeIterRecParser._read_hwts_data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={1: 101}):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check._parse_hwts_data()
        InfoConfReader()._info_json = {}

    def test_parse_ai_core_data(self):
        data = struct.pack("=BBHHHII10Q8I", 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 5, 3, 2, 5, 4, 6, 9, 5, 3, 2, 5, 4, 6)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.NoGeIterRecParser._read_hwts_data'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('os.path.getsize', return_value=len(data)), \
                mock.patch('builtins.open', mock.mock_open(read_data=data)), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={1: 101}):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check._parse_ai_core_data()

    def test_save(self):
        with mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.init'), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.flush'), \
                mock.patch('msmodel.iter_rec.iter_rec_model.HwtsIterModel.finalize', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.Utils.obj_list_to_list'), \
                mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={1: 101}):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check._iter_info_dict = {'a': 'b'}
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.NoGeIterRecParser.parse'), \
                mock.patch(NAMESPACE + '.NoGeIterRecParser.save', side_effect=ProfException(0)), \
                mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch('common_func.utils.Utils.get_scene', return_value='step_info'), \
                mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                           return_value={1: 101}):
            check = NoGeIterRecParser(self.file_list, CONFIG)
            check.ms_run()
