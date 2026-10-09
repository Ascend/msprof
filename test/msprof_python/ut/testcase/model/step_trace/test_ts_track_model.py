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
from unittest import mock

from msmodel.step_trace.ts_track_model import TsTrackModel
from model.test_dir_cr_base_model import TestDirCRBaseModel
from common_func.db_name_constant import DBNameConstant
from common_func.profiling_scene import ProfilingScene
from profiling_bean.db_dto.step_trace_dto import IterationRange

NAMESPACE = 'msmodel.step_trace.ts_track_model'


class TestTsTrackModel(TestDirCRBaseModel):

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TsTrackModel.insert_data_to_db'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.flush('StepTrace', [])

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.sql_create_general_table'), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = TsTrackModel('test', 'step_trace.db', ['StepTrace', 'TsMemcpy'])
            check.create_table('StepTrace')

    def test_get_task_flip_data_should_return_1_data_when_1_data_in_table(self):
        data = [
            [1, 4367981467, 2, 0],
        ]
        with TsTrackModel(self.PROF_DEVICE_DIR,
                          DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]) as model:
            model.create_table(DBNameConstant.TABLE_DEVICE_TASK_FLIP)
            model.flush(DBNameConstant.TABLE_DEVICE_TASK_FLIP, data)
            result = model.get_task_flip_data()
        self.assertEqual(len(result), 1)

    def test_get_step_syscnt_range_by_iter_range_step_start_should_return_0_when_iter1(self):
        data = [
            [1, 1, 8046427017271, 8046427146986, 1],
            [2, 1, 8046487344086, 8046487472738, 2]
        ]
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value="CREATE TABLE IF NOT EXISTS step_trace_data(index_id INTEGER, "
                                     "model_id INTEGER, step_start INTEGER, step_end INTEGER, iter_id INTEGER)"):
            with TsTrackModel(self.PROF_DEVICE_DIR,
                              DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]) as model:
                model.create_table(DBNameConstant.TABLE_STEP_TRACE_DATA)
                model.flush(DBNameConstant.TABLE_STEP_TRACE_DATA, data)
                iteration_1 = IterationRange(model_id=1, iteration_id=1, iteration_count=1)
                result = model.get_step_syscnt_range_by_iter_range(iteration_1)
                self.assertEqual(result.step_start, 0)
                self.assertEqual(result.step_end, 8046427146986)

    def test_get_step_syscnt_range_by_iter_range_step_start_should_return_min_when_not_iter1(self):
        data = [
            [1, 1, 8046427017271, 8046427146986, 1],
            [2, 1, 8046487344086, 8046487472738, 2]
        ]
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value="CREATE TABLE IF NOT EXISTS step_trace_data(index_id INTEGER, "
                                     "model_id INTEGER, step_start INTEGER, step_end INTEGER, iter_id INTEGER)"):
            with TsTrackModel(self.PROF_DEVICE_DIR,
                              DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]) as model:
                model.create_table(DBNameConstant.TABLE_STEP_TRACE_DATA)
                model.flush(DBNameConstant.TABLE_STEP_TRACE_DATA, data)
                iteration_2 = IterationRange(model_id=1, iteration_id=2, iteration_count=1)
                result = model.get_step_syscnt_range_by_iter_range(iteration_2)
                self.assertEqual(result.step_start, 8046487344086)
                self.assertEqual(result.step_end, 8046487472738)

    def test_get_step_syscnt_range_should_return_min_start_and_max_end_syscnt_when_iter_1_and_2(self):
        data = [
            [1, 1, 8046427017271, 8046427146986, 1],
            [2, 1, 8046487344086, 8046487472738, 2]
        ]
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value="CREATE TABLE IF NOT EXISTS step_trace_data(index_id INTEGER, "
                                     "model_id INTEGER, step_start INTEGER, step_end INTEGER, iter_id INTEGER)"):
            with TsTrackModel(self.PROF_DEVICE_DIR,
                              DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]) as model:
                model.create_table(DBNameConstant.TABLE_STEP_TRACE_DATA)
                model.flush(DBNameConstant.TABLE_STEP_TRACE_DATA, data)
                iteration_1 = IterationRange(model_id=1, iteration_id=1, iteration_count=2)
                result = model.get_step_syscnt_range(iteration_1)
                self.assertEqual(8046427017271, result.step_start)
                self.assertEqual(8046487472738, result.step_end)

    def test_get_step_data(self):
        data = [
            [1, 4294967295, 8046427017271, 8046427146986, 1],
            [2, 4294967295, 8046487344086, 8046487472738, 2]
        ]
        table_name = ProfilingScene().get_step_table_name()
        with mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value="CREATE TABLE IF NOT EXISTS step_trace_data(index_id INTEGER, "
                                     "model_id INTEGER, step_start INTEGER, step_end INTEGER, iter_id INTEGER)"):
            with TsTrackModel(self.PROF_DEVICE_DIR,
                              DBNameConstant.DB_STEP_TRACE, [table_name]) as model:
                model.create_table(table_name)
                model.flush(table_name, data)
                result = model.get_step_trace_data(ProfilingScene().get_step_table_name())
                self.assertEqual(2, len(result))
                self.assertEqual(8046427017271, result[0].step_start)
                self.assertEqual(8046487344086, result[1].step_start)
