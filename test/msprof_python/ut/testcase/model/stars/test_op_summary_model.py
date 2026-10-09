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

from common_func.db_name_constant import DBNameConstant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.stars.op_summary_model import OpSummaryModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'msmodel.stars.op_summary_model'
SAMPLE_CONFIG = {"result_dir": "test",
                 "iter_id": 1,
                 "model_id": 1}


class TestOpSummaryModel(TestDirCRBaseModel):

    def test_get_operator_data_by_task_type_should_return_different_data_when_is_empty(self):
        create_ge_summary_sql = "create table IF NOT EXISTS {0} " \
                                "(model_id INTEGER, task_id INTEGER, stream_id INTEGER," \
                                "op_name TEXT, op_type TEXT, block_num INTEGER, mix_block_num INTEGER," \
                                "task_type TEXT, timestamp INTEGER,  batch_id INTEGER," \
                                "context_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_GE)
        create_task_time_sql = "create table IF NOT EXISTS {0} " \
                               "(task_id INTEGER, stream_id INTEGER, start_time INTEGER, duration_time INTEGER," \
                               "wait_time TEXT, task_type TEXT, index_id INTEGER, model_id INTEGER," \
                               "batch_id TEXT, subtask_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)

        ge_summary_data = [
            (4294967295, 1, 23, "ai_core", "ai_core", 1, 0, "AI_CORE", 397531459726750, 0, 4294967295),
            (4294967295, 2, 23, "ai_core", "ai_core", 1, 0, "AI_CORE", 397531459726751, 0, 4294967295),
            (4294967295, 3, 23, "ai_cpu", "ai_cpu", 1, 0, "AI_CPU", 397531459726752, 0, 4294967295),
            (4294967295, 4, 23, "ai_vector", "ai_vector", 1, 0, "AI_VECTOR_CORE", 397531459726753, 0, 4294967295),
            (4294967295, 5, 23, "mix_aiv", "mix_aiv", 1, 0, "MIX_AIV", 397531459726754, 0, 4294967295),
            (4294967295, 6, 23, "mix_aic", "mix_aic", 1, 0, "MIX_AIC", 397531459726755, 0, 4294967295),
        ]

        task_time_data = [
            (1, 23, 397531459795910, 4280, 0, "AI_CORE", -1, 4294967295, 0, 4294967295),
            (2, 23, 397531459795911, 4280, 0, "UNKNOWN", -1, 4294967295, 0, 4294967295),
            (3, 23, 397531459795912, 4280, 0, "AI_CPU", -1, 4294967295, 0, 4294967295),
            (4, 23, 397531459795913, 4280, 0, "AI_VECTOR_CORE", -1, 4294967295, 0, 4294967295),
            (5, 23, 397531459795914, 4280, 0, "MIX_AIV", -1, 4294967295, 0, 4294967295),
            (6, 23, 397531459795915, 4280, 0, "MIX_AIC", -1, 4294967295, 0, 4294967295),
        ]

        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open, \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_ge_summary_sql)
            db_open.create_table(create_task_time_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_summary_data)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, task_time_data)
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.cur = db_open.db_curs
            check.conn = db_open.db_conn
            res = check.get_operator_data_by_task_type()
            self.assertEqual(len(res), 5)

    def test_get_operator_data_by_task_type_should_return_different_data_when_is_empty_and_all_export(self):
        create_ge_summary_sql = "create table IF NOT EXISTS {0} " \
                                "(model_id INTEGER, task_id INTEGER, stream_id INTEGER," \
                                "op_name TEXT, op_type TEXT, block_num INTEGER, mix_block_num INTEGER," \
                                "task_type TEXT, timestamp INTEGER,  batch_id INTEGER, index_id INTEGER, " \
                                "context_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_GE)
        create_task_time_sql = "create table IF NOT EXISTS {0} " \
                               "(task_id INTEGER, stream_id INTEGER, start_time INTEGER, duration_time INTEGER," \
                               "wait_time TEXT, task_type TEXT, index_id INTEGER, model_id INTEGER," \
                               "batch_id TEXT, subtask_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)

        ge_summary_data = [
            (1, 1, 23, "PromptFlashAttention", "PromptFlashAttention",
             1, 0, "AI_CORE", 397531459726748, 0, 1, 4294967295),
            (1, 2, 23, "PromptFlashAttention", "PromptFlashAttention",
             1, 0, "AI_CORE", 397531459726749, 0, 2, 4294967295),
            (4294967295, 1, 23, "Add", "Add", 1, 0, "AI_CORE", 397531459726750, 0, -1, 4294967295),
        ]

        task_time_data = [
            (1, 23, 397531459795908, 4280, 0, "AI_CORE", 1, 1, 0, 4294967295),
            (1, 23, 397531459795909, 4280, 0, "AI_CORE", 1, 1, 0, 4294967295),
            (1, 23, 397531459795910, 4280, 0, "AI_CORE", 1, 4294967295, 0, 4294967295),
        ]

        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open, \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_ge_summary_sql)
            db_open.create_table(create_task_time_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_summary_data)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, task_time_data)
            ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.cur = db_open.db_curs
            check.conn = db_open.db_conn
            res = check.get_operator_data_by_task_type()
            self.assertEqual(len(res), 3)

    def test_get_operator_data_by_task_type_should_return_different_data_when_is_empty_and_not_all_export(self):
        create_ge_summary_sql = "create table IF NOT EXISTS {0} " \
                                "(model_id INTEGER, task_id INTEGER, stream_id INTEGER," \
                                "op_name TEXT, op_type TEXT, block_num INTEGER, mix_block_num INTEGER," \
                                "task_type TEXT, timestamp INTEGER,  batch_id INTEGER, index_id INTEGER, " \
                                "context_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_GE)
        create_task_time_sql = "create table IF NOT EXISTS {0} " \
                               "(task_id INTEGER, stream_id INTEGER, start_time INTEGER, duration_time INTEGER," \
                               "wait_time TEXT, task_type TEXT, index_id INTEGER, model_id INTEGER," \
                               "batch_id TEXT, subtask_id INTEGER)".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)

        ge_summary_data = [
            (4294967295, 1, 23, "PromptFlashAttention", "PromptFlashAttention",
             1, 0, "AI_CORE", 397531459726750, 0, 1, 4294967295),
            (4294967295, 2, 23, "PromptFlashAttention", "PromptFlashAttention",
             1, 0, "AI_CORE", 397531459726751, 0, 2, 4294967295),
        ]

        task_time_data = [
            (1, 23, 397531459795910, 4280, 0, "AI_CORE", -1, 4294967295, 0, 4294967295),
            (1, 23, 397531459795911, 4280, 0, "AI_CORE", -1, 4294967295, 0, 4294967295),
        ]

        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_open, \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_open.db_path):
            db_open.create_table(create_ge_summary_sql)
            db_open.create_table(create_task_time_sql)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_GE, ge_summary_data)
            db_open.insert_data(DBNameConstant.TABLE_SUMMARY_TASK_TIME, task_time_data)
            ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
            check = OpSummaryModel(SAMPLE_CONFIG)
            check.cur = db_open.db_curs
            check.conn = db_open.db_conn
            res = check.get_operator_data_by_task_type()
            self.assertEqual(len(res), 0)

    def test_separate_kfc_stream_should_return_empty_when_db_not_exist(self):
        data = []
        check = OpSummaryModel(SAMPLE_CONFIG)
        data = check.separate_kfc_stream(data)
        self.assertEqual([], data)
