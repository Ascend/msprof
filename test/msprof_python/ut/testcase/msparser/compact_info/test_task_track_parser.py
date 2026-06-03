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

import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.compact_info.task_track_bean import TaskTrackBean
from msparser.compact_info.task_track_bean import DPUTaskTrackBean
from msparser.compact_info.task_track_bean import TaskTrackChip6Bean
from msparser.compact_info.task_track_parser import TaskTrackParser
from profiling_bean.prof_enum.data_tag import DataTag
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msparser.compact_info.task_track_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.task_track_model'
DPU_MODEL_NAMESPACE = 'msmodel.dpu.dpu_task_model'
GE_HASH_MODEL_NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestTaskTrackParser(unittest.TestCase):
    file_list = {DataTag.TASK_TRACK: ['aging.compact.task_track.slice_0', 'unaging.compact.task_track.slice_0'],
                 DataTag.DPU_TASK_TRACK: ['aging.compact.dpu_track.slice_0', 'unaging.compact.dpu_track.slice_0']}

    def test_reformat_data(self):
        InfoConfReader()._info_json = {"drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check._type_hash_dict = {
                'runtime': {
                    '1000': 'task_track',
                },
            }
            bean_data = TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 1, 123456789])
            check.format_task_track_data([bean_data])
            self.assertEqual(len(check._task_track_data), 1)
            self.assertEqual(len(check._task_track_data[0]), 11)
            self.assertEqual(check._task_track_data[0][7], 'task_track')
            check = TaskTrackParser(self.file_list, CONFIG)
            check.format_task_track_data([])
            self.assertEqual(check._task_track_data, [])

    def test_parse(self):
        with mock.patch(NAMESPACE + '.TaskTrackParser.parse_bean_data'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check._task_track_data = [0, 75838889645892, 'CREATE_STREAM', 0, 0, 270772, 0, 'task_track', 'runtime', 12]
            check._task_flip_data = [0, 75838889645892, "FLIP_TASK", 0, 0, 270772, 0, 'task_track', 'runtime', 12]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TaskTrackParser.parse'), \
                mock.patch(NAMESPACE + '.TaskTrackParser.save'):
            TaskTrackParser(self.file_list, CONFIG).ms_run()
        TaskTrackParser({DataTag.TASK_TRACK: []}, CONFIG).ms_run()

    def test_reformat_data_should_return_4_task_track_and_3_flip_and_1_dpu_when_filtered_maintenance(self):
        InfoConfReader()._info_json = {"drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
        task_data = [
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 1, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 97, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 2, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 97, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 97, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 6, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 3, 123456789]),
            TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 4, 123456789]),
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 4096, 0, 0, 0, 1, 123456789]),
        ]

        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):

            check = TaskTrackParser(self.file_list, CONFIG)
            check._type_hash_dict = {
                'runtime': {
                    '1000': 'task_track',
                    '97': 'FLIP_TASK',
                    '6': 'MAINTENANCE',
                },
            }
            check.format_task_track_data(task_data)
            self.assertEqual(len(check._task_track_data), 4)
            self.assertEqual(len(check._task_flip_data), 3)
            self.assertEqual(len(check._dpu_data), 1)

    def test_reformat_data_should_return_4_task_track_when_get_4_task(self):
        InfoConfReader()._info_json = {"drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
        task_data = [
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 1, 123456789]),
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 1, 123456789]),
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 2, 123456789]),
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 2, 123456789])
        ]

        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check.format_task_track_data(task_data)
            self.assertEqual(len(check._task_track_data), 4)

    def test_format_dpu_track_data_length_mismatch(self):
        """测试 DPU 数据长度不匹配警告"""
        dpu_tracks = [
            DPUTaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 4096, 0, 0, 1, 0, 75838889643892]),
        ]
        task_tracks = [
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 4096, 0, 0, 0, 1, 123456789]),
            TaskTrackChip6Bean([23130, 5000, 1000, 270722, 12, 75838889645892, 4096, 0, 1, 0, 1, 987654321]),
        ]

        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check._type_hash_dict = {
                'runtime': {
                    '1': 'KERNEL_AICPU',
                },
            }
            check._ge_hash_dict = {'123456789': 'dpu1', '987654321': 'dpu2'}
            check._dpu_data = task_tracks
            check.format_dpu_track_data(dpu_tracks)

            self.assertEqual(len(check._dpu_data), 1)
            expect_dpu_data = [[0, 270722, 75838889643892, 75838889645892, 'AI_CPU', 0, 0, 'dpu1']]
            self.assertEqual(check._dpu_data, expect_dpu_data)

    def test_ms_run_with_only_dpu_track(self):
        """测试只有 DPU track 的情况"""
        file_list = {DataTag.DPU_TASK_TRACK: ['dpu_track.slice_0']}
        with mock.patch(NAMESPACE + '.TaskTrackParser.parse'), \
                mock.patch(NAMESPACE + '.TaskTrackParser.save'), \
                mock.patch(NAMESPACE + '.logging.info') as mock_info:
            check = TaskTrackParser(file_list, CONFIG)
            check.ms_run()
            mock_info.assert_called_once()
            self.assertIn('DPU', str(mock_info.call_args))

    def test_save_with_empty_data(self):
        """测试保存空数据时不调用 flush"""
        with mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.__init__', return_value=None), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.__enter__'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.__exit__', return_value=False), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.flush') as mock_flush, \
                mock.patch(DPU_MODEL_NAMESPACE + '.DPUTaskModel.__init__', return_value=None), \
                mock.patch(DPU_MODEL_NAMESPACE + '.DPUTaskModel.__enter__'), \
                mock.patch(DPU_MODEL_NAMESPACE + '.DPUTaskModel.__exit__', return_value=False), \
                mock.patch(DPU_MODEL_NAMESPACE + '.DPUTaskModel.flush') as mock_dpu_flush:
            check = TaskTrackParser(self.file_list, CONFIG)
            check._task_track_data = []
            check._task_flip_data = []
            check._dpu_data = []
            check.save()
            mock_flush.assert_not_called()
            mock_dpu_flush.assert_not_called()

