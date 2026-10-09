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
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from mscalculate.step_trace.create_step_table import StepTableBuilder, StepTracePreProcess
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto
from sqlite.db_manager import DBManager

NAMESPACE = 'mscalculate.step_trace.create_step_table'


def get_data():
    return {
        1: {
            1: {
                "step_start": 2,
                "step_end": 7,
                "training_trace": [{"fp": 3, "bp": 5}],
                "all_reduce": [{"reduce_start": 4, "reduce_end": 6}]
            },

            2: {
                "step_start": 7,
                "step_end": 12,
                "training_trace": [{"fp": 9, "bp": 11}],
                "all_reduce": [{"reduce_start": 8, "reduce_end": 10}]
            },

            3: {
                "step_start": 16,
                "step_end": 18,
                "training_trace": [],
                "all_reduce": []

            },

            4: {
                "step_start": 19,
                "step_end": 21,
                "training_trace": [{"fp": 20, "bp": None}],
                "all_reduce": []
            },

            5: {
                "step_start": 22,
                "step_end": 24,
                "training_trace": [],
                "all_reduce": [{'reduce_end': None, 'reduce_start': 23}]
            }

        }
    }


class TestCreateStepTable(unittest.TestCase):
    record_1 = StepTraceOriginDto()
    record_1.model_id = 1
    record_1.timestamp = 1
    record_1.tag_id = 0
    record_2 = StepTraceOriginDto()
    record_2.model_id = 1
    record_2.timestamp = 2
    record_2.tag_id = 0
    record_3 = StepTraceOriginDto()
    record_3.model_id = 1
    record_3.timestamp = 3
    record_3.tag_id = 2
    record_4 = StepTraceOriginDto()
    record_4.model_id = 1
    record_4.timestamp = 4
    record_4.tag_id = 3
    record_5 = StepTraceOriginDto()
    record_5.model_id = 1
    record_5.timestamp = 5
    record_5.tag_id = 4
    record_6 = StepTraceOriginDto()
    record_6.model_id = 1
    record_6.timestamp = 6
    record_6.tag_id = 1
    record_7 = StepTraceOriginDto()
    record_7.model_id = 1
    record_7.timestamp = 7
    record_7.tag_id = 1
    record_8 = StepTraceOriginDto()
    record_8.model_id = 1
    record_8.timestamp = 8
    record_8.tag_id = 11

    step_trace = [record_1, record_2, record_3, record_4, record_5, record_6, record_7, record_8]
    ordered_step_trace = [record_1, record_3, record_4, record_5, record_6, record_2, record_7, record_8]

    def test_process_step_trace(self):
        with mock.patch(NAMESPACE + '.StepTableBuilder.build_table'):
            StepTableBuilder.process_step_trace(self.step_trace)

        collect_data = get_data()
        self.assertEqual(collect_data[1][2].get('step_start'), 7)

    def test_reorder_step_trace_for_pipe_stage(self):
        result = StepTracePreProcess().reorder_step_trace_for_pipe_stage(self.step_trace)
        self.assertEqual(result, self.ordered_step_trace)

    def test_build_table(self):
        sample_config = {"result_dir": ""}
        with mock.patch('common_func.ai_stack_data_check_manager.'
                        'AiStackDataCheckManager.contain_training_trace_data', return_value=True), \
                mock.patch(NAMESPACE + '.CreateStepTraceData.run', return_value=None), \
                mock.patch(NAMESPACE + '.CreateAllReduce.run', return_value=None), \
                mock.patch(NAMESPACE + '.CreateTrainingTrace.run', return_value=None), \
                mock.patch(NAMESPACE + '.GetNextCreator.run', return_value=None):
            StepTableBuilder.build_table(sample_config)

    def test_run(self):
        with mock.patch(NAMESPACE + '.StepTableBuilder._connect_step_db', return_value=None), \
                mock.patch(NAMESPACE + '.StepTableBuilder.process_step_trace'), \
                mock.patch(NAMESPACE + '.StepTableBuilder._get_step_trace_data'), \
                mock.patch(NAMESPACE + '.StepTableBuilder.build_table'):
            StepTableBuilder.run({})

    def test_get_step_trace_data(self):
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            test = StepTableBuilder._get_step_trace_data()
        self.assertEqual(test, [])

    def test_get_step_trace_data_2(self):
        sample_config = {"result_dir": ""}
        create_sql = "CREATE TABLE IF NOT EXISTS {}" \
                     " (index_id, model_id, timestamp, tag_id)".format(DBNameConstant.TABLE_STEP_TRACE)
        data = ((0, 2, 1, 3),)
        db_manager = DBManager()
        insert_sql = db_manager.insert_sql(DBNameConstant.TABLE_STEP_TRACE, data)

        test_sql = db_manager.create_table(DBNameConstant.DB_STEP_TRACE, create_sql, insert_sql, data)

        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=" "), \
                mock.patch(NAMESPACE + '.StepTableBuilder.process_step_trace'), \
                mock.patch(NAMESPACE + '.StepTableBuilder.build_table'), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            StepTableBuilder.run(sample_config)
        db_manager.destroy(test_sql)

    def test_connect_step_db(self):
        sample_config = {"result_dir": ""}
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=" "), \
                mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)), \
                mock.patch(NAMESPACE + '.logging.error'):
            StepTableBuilder._connect_step_db(sample_config)


if __name__ == '__main__':
    unittest.main()
