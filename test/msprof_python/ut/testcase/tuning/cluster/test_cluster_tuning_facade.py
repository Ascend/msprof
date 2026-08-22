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

import os
import unittest
from unittest import mock

import pytest

from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.cluster.communication_matrix_parser import CommunicationMatrixParser
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.critical_path_parser import CriticalPathParser
from msparser.cluster.meta_parser import OpTaskBundle
from tuning.cluster.cluster_tuning_facade import ClusterTuningFacade

NAMESPACE = 'tuning.cluster.cluster_tuning_facade'


class HcclOp:
    op_name = 'all_reduce'
    iteration = 0
    plane_id = 0
    stream_id = 1
    first_timestamp = 1
    timestamp = 1
    duration = 2


class GeOp:
    op_name = 'MatMul'
    task_type = 'AI_CORE'
    stream_id = 0
    start_time = 0
    end_time = 1
    duration_time = 1


class TestClusterTuningFacade(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": 1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 6}

    def setUp(self) -> None:
        os.makedirs(self.params['collection_path'])

    def tearDown(self) -> None:
        clear_dt_project(self.params['collection_path'])

    def test_cluster_communication(self):
        ClusterTuningFacade(self.params).cluster_communication()

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'):
            ClusterTuningFacade(self.params).process()

    def test_run_should_return_success_when_all_data_type(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade.cluster_communication'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.communication_matrix'):
            tuning_facade = ClusterTuningFacade(self.params)
            tuning_facade.data_type = -1
            tuning_facade.run()

    def test_run_should_return_success_when_data_type_9(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade.cluster_communication'):
            tuning_facade = ClusterTuningFacade(self.params)
            tuning_facade.data_type = 9
            tuning_facade.run()

    def test_run_should_return_success_when_data_type_10(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade.communication_matrix'):
            tuning_facade = ClusterTuningFacade(self.params)
            tuning_facade.data_type = 10
            tuning_facade.run()

    def test_run_invalid_collection_prof1(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
             mock.patch(NAMESPACE + '.ClusterTuningFacade.run'), \
             pytest.raises(ProfException) as err:
            ClusterTuningFacade(self.params).process()
            self.assertEqual(ProfException.PROF_CLUSTER_DIR_ERROR, err.value.code)

    def test_run_invalid_model_id_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": -1,
                      "iteration_id": 1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_model_id_is_none(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'):
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "iteration_id": 1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()

    def test_run_invalid_data_type_prof(self):
        with mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "data_type": 122,
                      "model_id": 0,
                      "iteration_id": 1}
            ClusterTuningFacade(params).process()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_data_type_is_none(self):
        with mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'):
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "iteration_id": 1,
                      "model_id": 0}
            ClusterTuningFacade(params).process()

    def test_run_invalid_iteration_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": 0,
                      "iteration_id": -1,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_run_invalid_iteration2_prof(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.run'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                pytest.raises(ProfException) as err:
            params = {"collection_path": self.DIR,
                      "npu_id": 1,
                      "model_id": 0,
                      "data_type": 6}
            ClusterTuningFacade(params).process()
        self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_cluster_communication(self):
        outspace1 = 'msparser.cluster.communication_parser'
        outspace2 = 'mscalculate.cluster.slow_rank_calculator'
        outspace3 = 'mscalculate.cluster.slow_link_calculator'
        dic = {StrConstant.TOTAL: {StrConstant.SLOW_RANK_SUGGESTION: 'RANK 1 is a slow rank.'}}
        parser = CommunicationParser({})
        with mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.generate_parser', return_value=parser), \
                mock.patch(outspace1 + '.CommunicationParser.run', return_value=dic), \
                mock.patch(NAMESPACE + '.SlowRankCalculatorFactory.generate_calculator'), \
                mock.patch(outspace2 + '.SlowRankCalculator.run'), \
                mock.patch(outspace2 + '.SlowRankCalculator.add_suggestions'), \
                mock.patch(NAMESPACE + '.SlowLinkCalculatorFactory.generate_calculator'), \
                mock.patch(outspace3 + '.SlowLinkCalculator.run'), \
                mock.patch(outspace3 + '.SlowLinkCalculator.add_suggestions'):
            ClusterTuningFacade(self.params).cluster_communication()

    def test_communication_matrix(self):
        outspace1 = 'msparser.cluster.communication_matrix_parser'
        outspace2 = 'mscalculate.cluster.communication_matrix_calculator'
        op_info = [{StrConstant.TOTAL: 'Communication Matrix Suggestions'}]
        parser = CommunicationMatrixParser({})
        with mock.patch(NAMESPACE + '.CommunicationMatrixParserFactory.generate_parser', return_value=parser), \
                mock.patch(outspace1 + '.CommunicationMatrixParser.run', return_value=op_info), \
                mock.patch(NAMESPACE + '.MatrixCalculatorFactory.generate_calculator'), \
                mock.patch(outspace2 + '.CommunicationMatrixCalculator.run'), \
                mock.patch(outspace2 + '.CommunicationMatrixCalculator.add_suggestions'):
            ClusterTuningFacade(self.params).communication_matrix()

    def test_critical_path_analysis_should_return_success_when_enable_critical_path(self):
        hccl_op_events = {'all_Reduce': OpTaskBundle(op_name='all_Reduce', start=1, end=3, tasks=[HcclOp()])}
        compute_op_events = [GeOp()]
        with mock.patch(NAMESPACE + '.CriticalPathAnalysisParserFactory.generate_parser',
                        return_value=CriticalPathParser(compute_op_events, hccl_op_events)):
            test_tuning_facade = ClusterTuningFacade(self.params)
            top_hccl_ops = test_tuning_facade.critical_path_analysis()
            self.assertEqual(top_hccl_ops, ('all_Reduce',))

    def test__check_params_valid_should_success_when_not_enable_critical_path(self):
        with pytest.raises(ProfException) as err:
            test_tuning_facade = ClusterTuningFacade(self.params)
            test_tuning_facade.data_type = 6
            test_tuning_facade._model_id = 1
            test_tuning_facade._iteration_id = 1
            test_tuning_facade._npu_id = 1
            test_tuning_facade._check_params_valid()
        self.assertEqual(ProfException.PROF_CLUSTER_DIR_ERROR, err.value.code)