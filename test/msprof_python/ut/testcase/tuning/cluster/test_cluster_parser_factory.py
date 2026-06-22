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
from collections import defaultdict
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from tuning.cluster.cluster_parser_factory import ClusterCommunicationParserFactory
from tuning.cluster.cluster_parser_factory import CommunicationMatrixParserFactory
from tuning.cluster.cluster_parser_factory import CriticalPathAnalysisParserFactory
from mscalculate.hccl.hccl_task import HcclTask

NAMESPACE = 'tuning.cluster.cluster_parser_factory'


class HcclOp:
    op_name = 'all_reduce'
    iteration = 0
    plane_id = 0


class GeOp:
    op_name = 'MatMul'
    iteration = 0
    plane_id = 0


class TestClusterCommunicationParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": -1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 6}

    def test_get_hccl_ops_by_iter(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.check_db', return_value=True), \
             mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                        return_value=[(0, 'hccldb_dir')]), \
             mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_conditions_from_db'), \
                mock.patch(NAMESPACE + '.LoadInfoManager.load_info'):
            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()

    def test_get_hccl_ops_by_iter_prof(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.check_db', return_value=True), \
                mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                           return_value=[]), \
                mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_conditions_from_db'), \
                mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                pytest.raises(ProfException) as err:

            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_get_hccl_ops_by_iter_rank_less_than_2(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                        return_value=[(0,)]), \
                pytest.raises(ProfException) as err:
            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_generate_parser_should_return_exception_when_no_rank_hccl_data_dict(self):
        with mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_hccl_ops_by_iter'), \
                pytest.raises(ProfException) as err:
            parser_factory = ClusterCommunicationParserFactory(self.params)
            parser_factory.rank_hccl_data_dict = {}
            parser_factory.generate_parser()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_conditions_from_db_invalid_rank_dir(self):
        with pytest.raises(ProfException) as err:
            parser_factory = ClusterCommunicationParserFactory(self.params)
            parser_factory.rank_dir_dict = {0: None, 1: None}
            parser_factory.get_conditions_from_db()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_get_conditions_from_db_invalid_rank_dir_1(self):
        with pytest.raises(ProfException) as err:
            parser_factory = ClusterCommunicationParserFactory(self.params)
            parser_factory.rank_dir_dict = {4088: 'dir', 5000: 'dir'}
            parser_factory.get_conditions_from_db()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_step_info_from_db_no_step_trace_table(self):
        test_iter_start_end = [[0, 9223372036854775807]]
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=False):
            rank_id = 1
            iter_start_end = ClusterCommunicationParserFactory(self.params).get_step_info_from_db(rank_id)
            self.assertEqual(iter_start_end, test_iter_start_end)

    def test_get_step_info_from_db_no_model_iteration_info_in_step_trace_table(self):
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
         pytest.raises(ProfException) as err:
            rank_id = 1
            ClusterCommunicationParserFactory(self.params).get_step_info_from_db(rank_id)
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_step_info_from_db_no_iteration_info_in_step_trace_table(self):
        iter_model = [(0, 123), (1, 232)]
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
             mock.patch(
                 'msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_model_id_with_iterations',
                 return_value=iter_model), \
             pytest.raises(ProfException) as err:
            rank_id = 1
            ClusterCommunicationParserFactory(self.params).get_step_info_from_db(rank_id)
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_step_info_from_db_should_return_exception_when_no_iteration_id(self):
        iter_model = [(0, 123), (1, 232)]
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
             mock.patch(
                'msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_model_id_with_iterations',
                return_value=iter_model), \
             pytest.raises(ProfException) as err:
            parser_factory = ClusterCommunicationParserFactory(self.params)
            parser_factory.iteration_id = None
            parser_factory.get_step_info_from_db(1)
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_get_conditions_from_db(self):
        iter_model = [(0, 123), (1, 232)]
        iter_start_end = [(44444444, 111)]
        test_hccl_op = HcclOp()
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
             mock.patch(
                 'msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_model_id_with_iterations',
                 return_value=iter_model), \
             mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_iter_start_end',
                        return_value=iter_start_end), \
             mock.patch(NAMESPACE + '.CommunicationModel.get_all_events_from_db',
                        return_value=[test_hccl_op]), \
                mock.patch(NAMESPACE + '.CommunicationModel.check_db', return_value=True):
            test_parser = ClusterCommunicationParserFactory(self.params)
            test_parser.rank_dir_dict = {0: 'dir0'}
            test_parser.get_conditions_from_db()
            self.assertEqual(test_parser.rank_hccl_data_dict, {'all_reduce': {0: [test_hccl_op]}})

    def test_get_hccl_events_from_db_invalid_connect(self):
        with pytest.raises(ProfException) as err:
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)])
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_hccl_events_from_db_return_none(self):
        model_space = 'msmodel.cluster_info.communication_model.CommunicationModel'
        with mock.patch(model_space + '.check_db', return_value=True), \
                mock.patch(model_space + '.get_all_events_from_db', return_value=[]):
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)])

    def test_get_hccl_events_from_db(self):
        hccl_task = HcclTask(op_name='all_reduce')
        model_space = 'msmodel.cluster_info.communication_model.CommunicationModel'
        with mock.patch(model_space + '.check_db', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.judge_table_exist', return_value=True), \
                mock.patch('common_func.db_manager.DBManager.fetch_all_data', side_effect=[[hccl_task], []]):
            test_parser = ClusterCommunicationParserFactory(self.params)
            test_parser.get_hccl_events_from_db(0, '', [(0, 100)])
            self.assertEqual(test_parser.rank_hccl_data_dict, {'all_reduce': {0: [hccl_task]}})

    def test_get_hccl_events_from_db_with_cpa(self):
        hccl_task = HcclTask(op_name='all_reduce')
        top_hccl_ops = tuple(('allReduce_1_1', 'allReduce_2_2', 'allReduce_3_3', 'allReduce_4_4', 'allReduce_5_5'))
        with mock.patch(NAMESPACE + '.CommunicationModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.CommunicationModel.get_all_events_from_db', return_value=[hccl_task]):
            test_parser = ClusterCommunicationParserFactory(self.params)
            test_parser.get_hccl_events_from_db(0, '', [(0, 100)], top_hccl_ops)
            self.assertEqual(test_parser.rank_hccl_data_dict, {'all_reduce': {0: [hccl_task]}})


class TestCommunicationMatrixParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": -1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 7}

    def test_generate_parser_should_return_exception_when_no_op_hccl_events(self):
        with mock.patch(NAMESPACE + '.CommunicationMatrixParserFactory.get_hccl_ops_by_iter'), \
                pytest.raises(ProfException) as err:
            parser_factory = CommunicationMatrixParserFactory(self.params)
            parser_factory.generate_parser()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)


class TestCriticalPathAnalysisParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {
        "collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
        "npu_id": -1,
        "model_id": 0,
        "iteration_id": 1,
        "data_type": 9
    }

    def test_get_conditions_from_db(self):
        iter_start_end = [(44444444, 111)]
        test_hccl_op = HcclOp()
        test_compute_op = GeOp()
        with mock.patch(NAMESPACE + '.CommunicationModel.get_all_events_from_db', return_value=[test_hccl_op]), \
                mock.patch(NAMESPACE + '.OpSummaryModel.get_operator_data_by_task_type',
                           return_value=[test_compute_op]), \
                mock.patch(NAMESPACE + '.CriticalPathAnalysisParserFactory.get_step_info_from_db',
                           return_value=iter_start_end), \
                mock.patch(NAMESPACE + '.CommunicationModel.check_db', return_value=True):
            critical_analysis_parser_factory = CriticalPathAnalysisParserFactory(self.params)
            critical_analysis_parser_factory.rank_dir_dict = {1: 'dir_1'}
            critical_analysis_parser_factory.get_conditions_from_db()
            self.assertEqual(critical_analysis_parser_factory.hccl_op_events, {'all_reduce': [test_hccl_op]})
            self.assertEqual(critical_analysis_parser_factory.compute_op_events, [test_compute_op])

    def test_get_conditions_from_db_should_return_exception_when_check_rank_info_fail(self):
        with pytest.raises(ProfException) as err:
            critical_analysis_parser_factory = CriticalPathAnalysisParserFactory(self.params)
            critical_analysis_parser_factory.rank_id = 2
            critical_analysis_parser_factory.rank_dir_dict = {1: 'dir_1'}
            critical_analysis_parser_factory.get_conditions_from_db()
            self.assertEqual(ProfException.PROF_INVALID_PARAM_ERROR, err.value.code)

    def test_update_data_should_success_when_get_hccl_events_ok(self):
        hccl_op = HcclOp()
        op_name_dict = defaultdict(list)
        op_name_dict[hccl_op.op_name].append(hccl_op)
        critical_analysis_parser_factory = CriticalPathAnalysisParserFactory(self.params)
        critical_analysis_parser_factory.update_data(op_name_dict, 1)
        self.assertEqual(critical_analysis_parser_factory.hccl_op_events, {'all_reduce': [hccl_op]})

    def test_update_data_should_return_exception_when_no_main_events(self):
        with pytest.raises(ProfException) as err:
            hccl_op = HcclOp()
            hccl_op.plane_id = 1
            op_name_dict = defaultdict(list)
            op_name_dict[hccl_op.op_name].append(hccl_op)
            critical_analysis_parser_factory = CriticalPathAnalysisParserFactory(self.params)
            critical_analysis_parser_factory.update_data(op_name_dict, 1)
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_generate_parser_should_return_exception_when_no_hccl_op_events(self):
        with mock.patch(NAMESPACE + '.CriticalPathAnalysisParserFactory.get_hccl_ops_by_iter'), \
                pytest.raises(ProfException) as err:
            parser_factory = CriticalPathAnalysisParserFactory(self.params)
            parser_factory.generate_parser()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_generate_parser_should_return_exception_when_no_compute_op_events(self):
        hccl_op = HcclOp()
        op_name_dict = defaultdict(list)
        op_name_dict[hccl_op.op_name].append(hccl_op)
        with mock.patch(NAMESPACE + '.CriticalPathAnalysisParserFactory.get_hccl_ops_by_iter'), \
                pytest.raises(ProfException) as err:
            parser_factory = CriticalPathAnalysisParserFactory(self.params)
            parser_factory.hccl_op_events = op_name_dict
            parser_factory.generate_parser()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)
