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
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.msprof_step import MsprofStep
from profiling_bean.db_dto.step_trace_dto import StepTraceDto

NAMESPACE = 'common_func.msprof_step'


class TestMsprofStep(unittest.TestCase):

    @staticmethod
    def _build_step(index_id: int, model_id: int, iter_id: int, step_start: int, step_end: int) -> StepTraceDto:
        return StepTraceDto(index_id=index_id, model_id=model_id, iter_id=iter_id,
                            step_start=step_start, step_end=step_end)

    def test_enter_returns_self_when_table_missing(self):
        step = MsprofStep('output')
        step.model = mock.Mock()
        step.model.check_table.return_value = False
        step.get_step_data = mock.Mock()

        result = step.__enter__()

        self.assertIs(result, step)
        step.get_step_data.assert_not_called()

    def test_enter_loads_data_when_table_exists(self):
        step = MsprofStep('output')
        step.model = mock.Mock()
        step.model.check_table.return_value = True
        step.get_step_data = mock.Mock()

        result = step.__enter__()

        self.assertIs(result, step)
        step.get_step_data.assert_called_once_with()

    def test_exit_finalizes_model(self):
        step = MsprofStep('output')
        step.model = mock.Mock()

        step.__exit__(None, None, None)

        step.model.finalize.assert_called_once_with()

    def test_get_step_data_fetches_from_step_trace_table(self):
        step = MsprofStep('output')
        step.model = mock.Mock()
        step.model.cur = 'cursor'
        expected = [self._build_step(1, 2, 3, 10, 20)]
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=expected) as fetch_all_data:
            step.get_step_data()

        step.model.init.assert_called_once_with()
        fetch_all_data.assert_called_once_with(
            'cursor',
            'select * from {}'.format(DBNameConstant.TABLE_STEP_TRACE_DATA),
            dto_class=StepTraceDto
        )
        self.assertEqual(expected, step.data)

    def test_get_step_iteration_time_returns_expected_range(self):
        step = MsprofStep('output')
        step.data = [
            self._build_step(1, 2, 2, 10, 20),
            self._build_step(2, 2, 5, 20, 50),
        ]
        with mock.patch.object(step, 'get_iter_id', return_value=(2, 5)):
            result = step.get_step_iteration_time(1, 2)

        self.assertEqual([20, 50], result)

    def test_get_step_iteration_time_returns_empty_when_iter_missing(self):
        step = MsprofStep('output')
        step.data = [self._build_step(1, 2, 2, 10, 20)]
        with mock.patch.object(step, 'get_iter_id', return_value=()):
            self.assertEqual([], step.get_step_iteration_time(1, 2))

        with mock.patch.object(step, 'get_iter_id', return_value=(2, 3)):
            self.assertEqual([], step.get_step_iteration_time(1, 2))

    def test_get_mix_op_iter_id_returns_parallel_bounds(self):
        step = MsprofStep('output')
        step.data = [
            self._build_step(10, Constant.GE_OP_MODEL_ID, 3, 10, 30),
            self._build_step(7, 2, 9, 15, 18),
            self._build_step(11, Constant.GE_OP_MODEL_ID, 8, 40, 60),
        ]

        result = step.get_mix_op_iter_id(7, 2)

        self.assertEqual((2, 7), result)

    def test_get_mix_op_iter_id_returns_empty_for_unknown_step(self):
        step = MsprofStep('output')
        step.data = [self._build_step(10, Constant.GE_OP_MODEL_ID, 3, 10, 30)]

        self.assertEqual((), step.get_mix_op_iter_id(7, 2))

    def test_get_graph_iter_id_and_model_index_lookup(self):
        step = MsprofStep('output')
        step.data = [self._build_step(7, 2, 5, 15, 18)]

        self.assertEqual((4, 5), step.get_graph_iter_id(7, 2))
        self.assertEqual((), step.get_graph_iter_id(8, 2))
        self.assertEqual((2, 7), step.get_model_and_index_id_by_iter_id(5))

        missing_model_id, missing_index_id = step.get_model_and_index_id_by_iter_id(9)
        self.assertIsInstance(missing_model_id, EmptyClass)
        self.assertIsInstance(missing_index_id, EmptyClass)

    def test_get_iter_id_dispatches_to_matching_scene_handler(self):
        step = MsprofStep('output')
        with mock.patch(NAMESPACE + '.Utils.need_all_model_in_one_iter', return_value=True), \
                mock.patch.object(step, 'get_mix_op_iter_id', return_value=(1, 3)) as get_mix_op_iter_id, \
                mock.patch.object(step, 'get_graph_iter_id', return_value=(4, 5)) as get_graph_iter_id:
            self.assertEqual((1, 3), step.get_iter_id(7, 2))

        get_mix_op_iter_id.assert_called_once_with(7, 2)
        get_graph_iter_id.assert_not_called()

        with mock.patch(NAMESPACE + '.Utils.need_all_model_in_one_iter', return_value=False), \
                mock.patch.object(step, 'get_mix_op_iter_id', return_value=(1, 3)) as get_mix_op_iter_id, \
                mock.patch.object(step, 'get_graph_iter_id', return_value=(4, 5)) as get_graph_iter_id:
            self.assertEqual((4, 5), step.get_iter_id(7, 2))

        get_mix_op_iter_id.assert_not_called()
        get_graph_iter_id.assert_called_once_with(7, 2)


if __name__ == '__main__':
    unittest.main()
