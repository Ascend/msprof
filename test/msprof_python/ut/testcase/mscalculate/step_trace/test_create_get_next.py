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

from mscalculate.step_trace.create_step_table import GetNextCreator
from mscalculate.step_trace.create_step_table import StepTableBuilder
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto

NAMESPACE = 'mscalculate.step_trace.create_step_table'


def get_data():
    timestamp = 189746300646091
    model_ids = [1, 1, 1, 1, 1, 2, 2, 2, 2, 2]
    tag_ids = [0, 20001, 20000, 20001, 1, 0, 20000, 20000, 20001, 1]
    data = [StepTraceOriginDto() for _ in range(len(model_ids))]
    for step_trace, model_id, tag_id in zip(data, model_ids, tag_ids):
        step_trace.index_id = 1
        step_trace.model_id = model_id
        step_trace.timestamp = timestamp
        step_trace.stream_id = 20
        step_trace.task_id = 5
        step_trace.tag_id = tag_id
        timestamp += 10
    return data


class TestCreateGetNext(unittest.TestCase):
    def test_run(self):
        sample_config = {"result_dir": "./", "devices": 0}
        with mock.patch(NAMESPACE + '.StepTableBuilder._connect_step_db'), \
                mock.patch(NAMESPACE + '.StepTableBuilder._get_step_trace_data', return_value=get_data()), \
                mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_training_trace_data', return_value=False), \
                mock.patch(NAMESPACE + '.CreateStepTraceData.run', return_value=None), \
                mock.patch(NAMESPACE + '.CreateAllReduce.run', return_value=None), \
                mock.patch(NAMESPACE + '.CreateTrainingTrace.run', return_value=None), \
                mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            StepTableBuilder.run(sample_config)
            self.assertEqual(GetNextCreator.data, [[1, 1, 189746300646111, 189746300646121],
                                                   [2, 1, 189746300646161, 189746300646171],
                                                   ])


if __name__ == '__main__':
    unittest.main()
