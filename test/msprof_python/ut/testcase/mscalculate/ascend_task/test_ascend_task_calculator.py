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
import os
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task_calculator import AscendTaskCalculator
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'mscalculate.ascend_task.ascend_task_calculator'


class TestAscendTaskCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_DeviceTaskCollector")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    calculator = AscendTaskCalculator({}, {StrConstant.PARAM_ITER_ID: IterationRange(1, 1, 1)})
    info_reader = InfoConfReader()
    info_reader._info_json = {'platform_version': "5"}

    def test__collect_ascend_tasks_in_graph_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        with mock.patch(NAMESPACE + ".AscendTaskGenerator.run",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", "", 0)]):
            self.assertEqual(len(self.calculator._collect_ascend_tasks()), 1)
        scene._scene = None

    def test__collect_ascend_tasks_in_operator_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        with mock.patch(NAMESPACE + ".AscendTaskGenerator.run",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", "", 0),
                                      TopDownTask(1, 2, 1, 1, 1, 1, 1000, 1000, "", "", 1)]):
            self.assertEqual(len(self.calculator._collect_ascend_tasks()), 2)
        scene._scene = None

    def test__judge_calculate_again_should_return_true_when_in_graph_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        self.assertTrue(self.calculator._judge_calculate_again())
        scene._scene = None
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test__judge_calculate_again_should_return_true_when_no_ascend_task_db_in_op_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertTrue(self.calculator._judge_calculate_again())
        scene._scene = None

    def test__judge_calculate_again_should_return_false_when_exist_ascend_task_db_in_op_scene(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertFalse(self.calculator._judge_calculate_again())
        scene._scene = None

    def test__save(self):
        data = [TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, "", "", 0)]
        with mock.patch(NAMESPACE + ".AscendTaskModel.init"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.drop_table"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.create_table"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.insert_data_to_db"), \
                mock.patch(NAMESPACE + ".AscendTaskModel.finalize"):
            self.calculator._save(data)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".AscendTaskCalculator._collect_ascend_tasks", return_value=[]):
            with mock.patch(NAMESPACE + ".AscendTaskCalculator._judge_calculate_again", return_value=False):
                self.calculator.ms_run()
            with mock.patch(NAMESPACE + ".AscendTaskCalculator._judge_calculate_again", return_value=True):
                self.calculator.ms_run()
