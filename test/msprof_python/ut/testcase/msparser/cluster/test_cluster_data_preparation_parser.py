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

import pytest

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from constant.constant import clear_dt_project
from msmodel.ai_cpu.data_preparation_view_model import DataPreparationViewModel
from msparser.aicpu.data_preparation_parser import DataPreparationParser
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto
from profiling_bean.db_dto.data_queue_dto import DataQueueDto
from profiling_bean.db_dto.host_queue_dto import HostQueueDto
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBOpen
from subclass.cluster_data_preparation_parser_subclass import ClusterDataPreparationParserSubclass

NAMESPACE = 'msparser.cluster.cluster_data_preparation_parser'


class DataList:
    def __init__(self, *args):
        self.op_type = args[1]
        self.total_fops = args[0]
        self.total_time = args[2]
        self.rank_id = args[3]
        self.dir_name = args[4]


class TestClusterDataPreparationParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterDataPreparationParser')
    params = {'model_id': 1, 'iteration_id': 1, 'npu_id': 1,
              'collection_path': os.path.join(DIR_PATH, "PROF1", "device_0", "sqlite")}
    cluster_rank_dto = ClusterRankDto()
    cluster_rank_dto.job_info = "N/A"
    cluster_rank_dto.device_id = "0"
    cluster_rank_dto.rank_id = "0"
    cluster_rank_dto.dir_name = "PROF1/device_0"
    dataQueueDto1 = DataQueueDto()
    dataQueueDto1.node_name = 'GetNext_dequeue_wait'
    dataQueueDto1.queue_size = 108
    dataQueueDto1.start_time = 65347257104.0
    dataQueueDto1.end_time = 65347257142.0
    dataQueueDto1.duration = 38.0

    def setUp(self):
        os.makedirs(os.path.join(self.DIR_PATH, "PROF1", "device_0", "sqlite"))

    def tearDown(self):
        clear_dt_project(self.DIR_PATH)

    def test_get_data_queue_data(self):
        db_path = os.path.join(self.DIR_PATH, "PROF1", "device_0", "sqlite", "data_preprocess.db")
        with DBOpen(DBNameConstant.DB_CLUSTER_DATA_PREPROCESS) as db_connect:
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_path), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.check_db',
                               return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.get_data_queue',
                               return_value=[]):
                check = ClusterDataPreparationParserSubclass({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                check.set_model(DataPreparationViewModel(self.DIR_PATH))
                self.assertEqual(check.get_data_queue_data(), [])
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value=db_path), \
                    mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                    mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.check_db',
                               return_value=False):
                check = ClusterDataPreparationParserSubclass({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                check.set_model(DataPreparationViewModel(self.DIR_PATH))
                ChipManager().chip_id = ChipModel.CHIP_V4_1_0
                self.assertEqual(check.get_data_queue_data(), [])

    def test_run(self):
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterDataPreparationParser._calculate'):
            check = ClusterDataPreparationParserSubclass(self.params)
            check.process()
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterDataPreparationParser._calculate'):
            check = ClusterDataPreparationParserSubclass({})
            check.process()

    def test__calculate_queue_data(self):
        queue_list = [self.dataQueueDto1, self.dataQueueDto1, self.dataQueueDto1, self.dataQueueDto1]
        check = ClusterDataPreparationParserSubclass(self.params)
        check.set_host_queue_step_count(len(queue_list))
        check.calculate_queue_data(queue_list)

    def test_calculate(self):
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._check_device_path_valid', return_value=True):
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data',
                            side_effect=(True, False, False)), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch(NAMESPACE + '.check_path_valid'), \
                    mock.patch(NAMESPACE + '.ClusterDataPreparationParser._query_host_queue'):
                check = ClusterDataPreparationParserSubclass(self.params)
                check.set_host_queue_mode(DataPreparationParser.HOST_DATASET_NOT_SINK_MODE)
                check.calculate()
        with mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ClusterDataPreparationParser._check_device_path_valid', return_value=False):
            with pytest.raises(ProfException):
                check = ClusterDataPreparationParserSubclass(self.params)

                check.calculate()
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._check_device_path_valid', return_value=True):
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data',
                            side_effect=(False,)), \
                    mock.patch(NAMESPACE + '.check_path_valid'):
                with pytest.raises(ProfException):
                    check = ClusterDataPreparationParserSubclass(self.params)

                    check.calculate()

    def test__query_host_queue(self):
        host_queue_dto_1 = HostQueueDto()
        host_queue_dto_1.index_id = 1
        host_queue_dto_1.queue_capacity = 16
        host_queue_dto_1.queue_size = 0
        host_queue_dto_1.mode = 1
        host_queue_dto_1.get_time = 1041
        host_queue_dto_1.send_time = 78
        host_queue_dto_1.total_time = 1119
        get_host_queue_from_dataset = [host_queue_dto_1]
        with mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.check_db',
                        return_value=True), \
                mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.get_host_queue_mode',
                           return_value=[[DataPreparationParser.HOST_DATASET_SINK_MODE]]), \
                mock.patch('msmodel.ai_cpu.data_preparation_view_model.DataPreparationViewModel.get_host_queue',
                           return_value=get_host_queue_from_dataset):
            check = ClusterDataPreparationParserSubclass(self.params)
            check.set_model(DataPreparationViewModel(self.DIR_PATH))
            check.query_host_queue()
            self.assertEqual(len(check.get_data().get("host_data_list")), 1)

    def test_storage_data(self):
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._get_cluster_path', return_value=self.DIR_PATH), \
                mock.patch('builtins.open', mock.mock_open(read_data='')), \
                mock.patch('os.fdopen', side_effect=OSError), \
                mock.patch('os.chmod'), \
                mock.patch('os.remove'):
            with pytest.raises(ProfException):
                check = ClusterDataPreparationParserSubclass(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                check.set_data({'total_info': {'step_count': 469, 'empty_queue': 0,
                                               'total_time': 37856.0, 'avg_time': 80.7164}})
                check.storage_data()

    def test_query_data_queue(self):
        queue_list = [self.dataQueueDto1, self.dataQueueDto1]
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._get_data_queue_data', return_value=[]), \
                mock.patch(NAMESPACE + '.ClusterDataPreparationParser._calculate_queue_data', return_value=[]):
            with pytest.raises(ProfException):
                check = ClusterDataPreparationParserSubclass(self.params)
                check.query_data_queue()
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._get_data_queue_data',
                        return_value=queue_list):
            check = ClusterDataPreparationParserSubclass(self.params)
            check.set_host_queue_step_count(len(queue_list))
            check.sample_config = {'ai_core_metrics': ''}
            check.query_data_queue()
        with mock.patch(NAMESPACE + '.ClusterDataPreparationParser._get_data_queue_data', return_value=[]), \
                mock.patch(NAMESPACE + '.ClusterDataPreparationParser._calculate_queue_data',
                           return_value=[(1, 2, 3, 4, 5, 6)]):
            with pytest.raises(ProfException):
                check = ClusterDataPreparationParserSubclass(self.params)
                check.query_data_queue()

    def test_get_cluster_path(self):
        with mock.patch('os.path.realpath', return_value='test\\query\\test\\test'), \
                mock.patch("os.path.exists", return_value=True):
            check = ClusterDataPreparationParserSubclass(self.params)
            result = check.get_cluster_path('test\\test')
            self.assertEqual(result, 'test\\query\\test\\test')

    def test_check_device_path_valid(self):
        with DBOpen(DBNameConstant.DB_CLUSTER_RANK) as db_connect:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect', return_value=[]):
                check = ClusterDataPreparationParserSubclass(self.params)
                result = check.check_device_path_valid()
                self.assertFalse(result)
            with mock.patch(NAMESPACE + '.ClusterInfoViewModel.check_db', return_value=True), \
                    mock.patch('os.path.exists', return_value=True), \
                    mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_info_based_on_rank_or_device_id',
                               return_value=self.cluster_rank_dto):
                check = ClusterDataPreparationParserSubclass(self.params)
                result = check.check_device_path_valid()
                self.assertTrue(result)


if __name__ == '__main__':
    unittest.main()
