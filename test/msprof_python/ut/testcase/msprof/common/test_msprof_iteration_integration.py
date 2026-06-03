#!/usr/bin/env python3
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
import sqlite3
import tempfile
import unittest

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from profiling_bean.db_dto.step_trace_dto import IterationRange


class TestMsprofIterationIntegration(unittest.TestCase):
    STEP_TRACE_DATA_SQL = (
        "CREATE TABLE IF NOT EXISTS step_trace_data "
        "(index_id INT, model_id INT, step_start INT, step_end INT, iter_id INT, ai_core_num INT DEFAULT 0)"
    )
    STEP_TRACE_SQL = (
        "CREATE TABLE IF NOT EXISTS StepTrace "
        "(stream_id INT, task_id INT, tag_id INT, timestamp INT)"
    )

    def setUp(self):
        self._info_reader = InfoConfReader()
        self._info_reader_state = dict(self._info_reader.__dict__)
        self._scene = ProfilingScene()
        self._scene_state = dict(self._scene.__dict__)

    def tearDown(self):
        self._info_reader.__dict__.clear()
        self._info_reader.__dict__.update(self._info_reader_state)
        self._scene.__dict__.clear()
        self._scene.__dict__.update(self._scene_state)

    def test_iteration_queries_in_step_scene_use_real_step_trace_db(self):
        step_trace_data = [
            (1, 1, 100, 200, 1, 60),
            (2, 1, 220, 320, 2, 60),
            (3, 1, 340, 450, 3, 60),
        ]
        step_trace = [
            (7, 100, 1, 1000),
            (9, 999, 2, 1500),
            (8, 200, 4, 2000),
        ]
        with tempfile.TemporaryDirectory(prefix='msprof_iteration_step_') as project_path:
            self._prepare_project(project_path, step_trace_data, step_trace)
            self._configure_info_reader()
            self._scene.init(project_path)
            self._scene.set_mode(ExportMode.ALL_EXPORT)

            iteration = MsprofIteration(project_path)
            iter_range = IterationRange(1, 1, 2)

            self.assertEqual([0, 320], iteration.get_step_syscnt_range_by_iter_range(iter_range))
            self.assertEqual([0.0, 320.0], iteration.get_iter_interval(iter_range))
            self.assertEqual({1: 200, 2: 320, 3: 450}, iteration.get_iteration_end_dict())
            self.assertEqual({'7-100', '8-200'}, iteration.get_step_trace_op())

            iter_set = iteration.get_index_id_list_with_index_and_model(iter_range)
            self.assertEqual({(1, 1), (2, 1), (0, 1)}, iter_set)

            step_end_list = iteration.get_step_end_range_by_iter_range(iter_range)
            self.assertEqual([1, 2], [datum.index_id for datum in step_end_list])
            self.assertEqual([200, 320], [datum.step_end for datum in step_end_list])

    def test_iteration_time_parallel_range_and_condition_follow_real_mix_scene(self):
        step_trace_data = [
            (1, 1, 100, 200, 1, 60),
            (2, 1, 220, 320, 2, 60),
            (1, Constant.GE_OP_MODEL_ID, 0, 90, 5, 60),
            (2, Constant.GE_OP_MODEL_ID, 100, 200, 20, 60),
            (10, 1, 110, 150, 10, 60),
            (11, 1, 160, 190, 11, 60),
        ]
        with tempfile.TemporaryDirectory(prefix='msprof_iteration_mix_') as project_path:
            self._prepare_project(project_path, step_trace_data)
            self._configure_info_reader()
            self._scene.init(project_path)
            self._scene.set_mode(ExportMode.GRAPH_EXPORT)

            step_iteration = MsprofIteration(project_path)
            step_range = IterationRange(1, 2, 1)
            mix_range = IterationRange(Constant.GE_OP_MODEL_ID, 2, 1)

            self.assertEqual([(200,), (320,)], step_iteration.get_step_iteration_time(step_range))
            self.assertEqual([(200.0, 320.0)], step_iteration.get_iteration_time(step_range,
                                                                                time_fmt=NumberConstant.NANO_SECOND))
            self.assertEqual(
                'where (start>=200.0 and start<=320.0) or (start<=200.0 and 200.0<=end)',
                step_iteration.get_condition_within_iteration(step_range, 'start', 'end')
            )

            iteration_info = step_iteration.get_iteration_info_by_index_id(2, 1)
            self.assertEqual((1, 2, 2), (iteration_info.model_id, iteration_info.index_id, iteration_info.iter_id))

            self.assertEqual([10, 20], step_iteration.get_parallel_iter_range(mix_range))
            self.assertEqual([], step_iteration.get_parallel_iter_range(IterationRange(Constant.GE_OP_MODEL_ID, 99, 1)))

            iter_set = step_iteration.get_index_id_list_with_index_and_model(mix_range)
            self.assertEqual(
                {
                    (0, Constant.GE_OP_MODEL_ID),
                    (2, Constant.GE_OP_MODEL_ID),
                    (0, 1),
                    (10, 1),
                    (11, 1),
                },
                iter_set,
            )

    def _prepare_project(self, project_path: str, step_trace_data: list, step_trace: list = None) -> None:
        sqlite_dir = PathManager.get_sql_dir(project_path)
        os.makedirs(sqlite_dir, exist_ok=True)
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_STEP_TRACE)
        conn = sqlite3.connect(db_path)
        try:
            curs = conn.cursor()
            curs.execute(self.STEP_TRACE_DATA_SQL)
            curs.executemany('INSERT INTO step_trace_data VALUES (?,?,?,?,?,?)', step_trace_data)
            if step_trace is not None:
                curs.execute(self.STEP_TRACE_SQL)
                curs.executemany('INSERT INTO StepTrace VALUES (?,?,?,?)', step_trace)
            conn.commit()
        finally:
            conn.close()

    def _configure_info_reader(self) -> None:
        self._info_reader._info_json = {
            'DeviceInfo': [{'hwts_frequency': '1000'}],
            'CPU': [{'Frequency': '1000'}],
        }
        self._info_reader._sample_json = {'devices': '0'}
        self._info_reader._start_info = {}
        self._info_reader._end_info = {}
        self._info_reader._start_log_time = 0
        self._info_reader._host_host_mon = 0
        self._info_reader._host_host_cnt = 0
        self._info_reader._host_mon = 0
        self._info_reader._host_cnt = 0
        self._info_reader._dev_cnt = 0
        self._info_reader._host_freq = NumberConstant.NANO_SECOND


if __name__ == '__main__':
    unittest.main()
