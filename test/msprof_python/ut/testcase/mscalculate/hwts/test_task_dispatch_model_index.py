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
from common_func.path_manager import PathManager
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from mscalculate.hwts.task_dispatch_model_index import TaskDispatchModelIndex
from profiling_bean.db_dto.step_trace_dto import IterationRange


class TestTaskDispatchModelIndex(unittest.TestCase):
    STEP_TRACE_DATA_SQL = (
        "CREATE TABLE IF NOT EXISTS step_trace_data "
        "(index_id INT, model_id INT, step_start INT, step_end INT, iter_id INT, ai_core_num INT DEFAULT 0)"
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

    def test_dispatch_uses_parallel_model_range_in_mix_scene(self):
        step_trace_data = [
            (1, Constant.GE_OP_MODEL_ID, 0, 90, 5, 60),
            (2, Constant.GE_OP_MODEL_ID, 100, 200, 20, 60),
            (10, 1, 110, 150, 10, 60),
            (11, 1, 160, 190, 11, 60),
        ]
        with tempfile.TemporaryDirectory(prefix='task_dispatch_mix_') as project_path:
            self._prepare_project(project_path, step_trace_data)
            self._configure_info_reader()
            self._scene.init(project_path)
            self._scene.set_mode(ExportMode.ALL_EXPORT)

            dispatcher = TaskDispatchModelIndex(IterationRange(Constant.GE_OP_MODEL_ID, 2, 1), project_path)

            self.assertEqual((1, 10), dispatcher.dispatch(145))
            self.assertEqual((1, 11), dispatcher.dispatch(175))
            self.assertEqual((Constant.GE_OP_MODEL_ID, 2), dispatcher.dispatch(195))
            self.assertEqual((Constant.GE_OP_MODEL_ID, 2), dispatcher.dispatch(260))

    def test_dispatch_falls_back_to_iteration_end_sequence_in_step_scene(self):
        step_trace_data = [
            (1, 1, 100, 200, 1, 60),
            (2, 1, 220, 320, 2, 60),
            (3, 1, 340, 450, 3, 60),
        ]
        with tempfile.TemporaryDirectory(prefix='task_dispatch_step_') as project_path:
            self._prepare_project(project_path, step_trace_data)
            self._configure_info_reader()
            self._scene.init(project_path)
            self._scene.set_mode(ExportMode.ALL_EXPORT)

            dispatcher = TaskDispatchModelIndex(IterationRange(1, 2, 2), project_path)

            self.assertEqual((1, 2), dispatcher.dispatch(250))
            self.assertEqual((1, 3), dispatcher.dispatch(400))
            self.assertEqual((1, 3), dispatcher.dispatch(600))

    def _prepare_project(self, project_path: str, step_trace_data: list) -> None:
        sqlite_dir = PathManager.get_sql_dir(project_path)
        os.makedirs(sqlite_dir, exist_ok=True)
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_STEP_TRACE)
        conn = sqlite3.connect(db_path)
        try:
            curs = conn.cursor()
            curs.execute(self.STEP_TRACE_DATA_SQL)
            curs.executemany('INSERT INTO step_trace_data VALUES (?,?,?,?,?,?)', step_trace_data)
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
