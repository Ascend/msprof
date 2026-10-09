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
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from sqlite.db_manager import DBManager

NAMESPACE = 'msmodel.ge.ge_info_calculate_model'


class TestGeInfoModel(unittest.TestCase):
    step_trace_dto_1 = StepTraceDto()
    step_trace_dto_1.model_id = 0
    step_trace_dto_1.index_id = 1
    step_trace_dto_1.iter_id = 1
    step_trace_dto_2 = StepTraceDto()
    step_trace_dto_2.model_id = 0
    step_trace_dto_2.index_id = 2
    step_trace_dto_2.iter_id = 2

    def test_check_table(self):
        db_manager = DBManager()
        res = db_manager.create_table('ge_info.db')
        with mock.patch(NAMESPACE + '.logging.warning'), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            check = GeInfoModel('test')
            result = check.check_table()
            self.assertFalse(result)
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = GeInfoModel('test')
            check.conn, check.cur = res[0], res[1]
            result = check.check_table()
            self.assertTrue(result)
        db_manager.destroy(res)

    def test_map_model_to_iter(self):
        db_name = "step_trace.db"
        table_name = "step_trace_data"
        db_manager = DBManager()
        if os.path.exists(os.path.join(db_manager.db_path, db_name)):
            os.remove(os.path.join(db_manager.db_path, db_name))

        create_sql = "CREATE TABLE IF NOT EXISTS {0} (model_id int, index_id int, iter_id int)".format(table_name)

        data = ((1, 1, 1),)
        insert_sql = "insert into {0} values ({1})".format(table_name, "?," * (len(data[0]) - 1) + "?")

        conn, curs = db_manager.create_table(db_name, create_sql, insert_sql, data)

        check = GeInfoModel(os.path.join(db_manager.db_path, "../"))

        with mock.patch(NAMESPACE + ".PathManager.get_db_path", return_value=""), \
                mock.patch(NAMESPACE + ".DBManager.create_connect_db", return_value=(conn, curs)):
            res = check.map_model_to_iter()

        if os.path.exists(db_manager.db_name):
            os.remove(db_manager.db_name)
        self.assertEqual(res, {(1, 1): 1})

    def test_get_batch_dict1(self):
        expect_res = {(-1, 2): (3, 4)}
        check = GeInfoModel("")
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(500000, 90000000, 2, 3, 4)]), \
                mock.patch(NAMESPACE + ".Utils.is_single_op_scene", return_value=True):
            res = check.get_batch_dict()
        self.assertEqual(expect_res, res)

    def test_get_batch_dict2(self):
        expect_res = {(5, 2): (3, 4)}
        check = GeInfoModel("")
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[(10, 3, 2, 3, 4)]), \
                mock.patch(NAMESPACE + ".Utils.is_single_op_scene", return_value=False), \
                mock.patch(NAMESPACE + ".GeInfoModel.map_model_to_iter", return_value={(10, 3): 5}):
            res = check.get_batch_dict()
        self.assertEqual(expect_res, res)

    def test_get_all_ge_static_shape_data_when_single_op_scene(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=False):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual(res, [{}, {}])

    def test_get_all_ge_static_shape_data_when_no_static_data(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_ge_task_data", return_value={}):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual(res, [{}, {}])

    def test_get_all_ge_static_shape_data(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_ge_task_data", return_value={0: 7, 1: 2}), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_step_trace_data",
                           return_value=[self.step_trace_dto_1, self.step_trace_dto_2]):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual([{1: 0, 2: 0}, {0: 7, 1: 2}], res)

    def test_get_all_ge_static_shape_data_1(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_ge_task_data", return_value={0: {7}, 1: {2}}), \
                mock.patch(NAMESPACE + ".PathManager.get_db_path"), \
                mock.patch(NAMESPACE + ".DBManager.create_connect_db", return_value=[1, 2]), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_step_trace_data",
                           return_value=[self.step_trace_dto_1, self.step_trace_dto_2]), \
                mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual([{1: 0, 2: 0}, {0: {7}, 1: {2}}], res)

    def test_get_all_ge_static_shape_data_2(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_step_trace_data",
                           return_value=[self.step_trace_dto_1, self.step_trace_dto_2]), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[[0, '7'], [1, '2']]):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual([{1: 0, 2: 0}, {0: {'7'}, 1: {'2'}}], res)

    def test_get_all_ge_dynamic_shape_data_when_single_op_scene(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=False):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_DYNAMIC_SHAPE)
        self.assertEqual({}, res)

    def test_get_all_ge_dynamic_shape_data_when_no_nonstatic_data(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_ge_task_data", return_value={}):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_DYNAMIC_SHAPE)
        self.assertEqual({}, res)

    def test_get_all_ge_dynamic_shape_data(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_ge_task_data", return_value={'0-1': 7}), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_step_trace_data",
                           return_value=[self.step_trace_dto_1, self.step_trace_dto_2]):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_DYNAMIC_SHAPE)
        self.assertEqual({1: 7}, res)

    def test_get_all_ge_dynamic_shape_data_1(self):
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value=[['0-1', '7,8']]), \
                mock.patch(NAMESPACE + ".GeInfoModel.get_step_trace_data",
                           return_value=[self.step_trace_dto_1, self.step_trace_dto_2]):
            check = GeInfoModel("")
            res = check.get_ge_data(Constant.GE_DYNAMIC_SHAPE)
        self.assertEqual({1: {'7', '8'}}, res)

    def test_get_ge_data_should_return_none_when_static_shape_and_step_export(self):
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value={}):
            check = GeInfoModel("")
            res = check.get_ge_task_data(Constant.GE_STATIC_SHAPE)
        self.assertEqual({}, res)
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_get_ge_data_should_return_none_when_dynamic_shape_and_step_export(self):
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        with mock.patch(NAMESPACE + ".Utils.is_step_scene", return_value=True), \
                mock.patch(NAMESPACE + ".DBManager.fetch_all_data", return_value={}):
            check = GeInfoModel("")
            res = check.get_ge_task_data(Constant.GE_DYNAMIC_SHAPE)
        self.assertEqual({}, res)
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

