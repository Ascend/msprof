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

from common_func.db_name_constant import DBNameConstant
from common_func.platform.chip_manager import ChipManager
from msparser.cluster.fops_parser import FopsParser
from profiling_bean.prof_enum.chip_model import ChipModel
from sqlite.db_manager import DBOpen

NAMESPACE = 'msparser.cluster.fops_parser'


class DataList:
    def __init__(self, *args):
        self.op_type = args[1]
        self.total_fops = args[0]
        self.total_time = args[2]
        self.rank_id = args[3]
        self.dir_name = args[4]


class TestFopsParser(unittest.TestCase):
    params = {'model_id': 1, 'iteration_id': 1, 'rank_id': 1, 'collection_path': 'test\\test'}

    def test_get_computation_data(self):
        with DBOpen(DBNameConstant.DB_AICORE_OP_SUMMARY) as db_connect:
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                               return_value=(False, False)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=10):
                check = FopsParser({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                self.assertEqual(check.get_fops_data(), [])
            with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test'), \
                    mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                               return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=10):
                check = FopsParser({'model_id': 1, 'iter_id': 1, 'rank_id': 1})
                ChipManager().chip_id = ChipModel.CHIP_V4_1_0
                self.assertEqual(check.get_fops_data(), 10)

    def test_run(self):
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.FopsParser.calculate'):
            check = FopsParser(self.params)
            check.process()
        with mock.patch('common_func.utils.Utils.is_step_scene', side_effect=(False, True)), \
                mock.patch('common_func.utils.Utils.is_single_op_graph_mix', return_value=False), \
                mock.patch(NAMESPACE + '.QueryArgumentCheck.check_arguments_valid'), \
                mock.patch(NAMESPACE + '.FopsParser.calculate'):
            check = FopsParser({})
            check.process()

    def test_calculate_computation_data(self):
        data_list = [DataList(0.07688797835760329, 'AtomicAddrClean', 1, 0, 0),
                     DataList(0.044279100905892856, 'Mul', 1, 0, 0),
                     DataList(0.23540968729221792, 'SquaredDifference', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD', 1, 0, 0),
                     DataList(0.04583021088185761, 'Mul', 1, 0, 0),
                     DataList(0.07688797835760329, 'AtomicAddrClean1', 1, 0, 0),
                     DataList(0.044279100905892856, 'Mul1', 1, 0, 0),
                     DataList(0.23540968729221792, 'SquaredDifference1', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD1', 1, 0, 0),
                     DataList(0.04583021088185761, 'Mul2', 1, 0, 0),
                     DataList(0.07688797835760329, 'AtomicAddrClean2', 1, 0, 0),
                     DataList(0.044279100905892856, 'Mul3', 1, 0, 0),
                     DataList(0.23540968729221792, 'SquaredDifference3', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD3', 1, 0, 0),
                     DataList(0.04583021088185761, 'Mul4', 1, 0, 0),
                     DataList(0.07688797835760329, 'AtomicAddrClean4', 1, 0, 0),
                     DataList(0.044279100905892856, 'Mul5', 1, 0, 0),
                     DataList(0.23540968729221792, 'SquaredDifference6', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD7', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD8', 1, 0, 0),
                     DataList(0.09965232461660241, 'ReduceSumD9', 1, 0, 0),
                     DataList(00.09965232461660241, 'ReduceSumD10', 1, 0, 0),
                     DataList(0.04583021088185761, 'Mul7', 1, 0, 0)
                     ]
        check = FopsParser(self.params)
        check.calculate_fops_data(data_list)

    def test_storage_data(self):
        with mock.patch(NAMESPACE + '.FopsParser.get_cluster_path', return_value='test'), \
                mock.patch('builtins.open', mock.mock_open(read_data='')), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch('os.fdopen', side_effect=OSError), \
                mock.patch('os.chmod'),\
                mock.patch('os.remove'):
            check = FopsParser(self.params)
            check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
            check.storage_data([])

    def test_query_computation_volume(self):
        with mock.patch(NAMESPACE + '.FopsParser.get_fops_data', return_value=[]), \
                mock.patch(NAMESPACE + '.FopsParser.calculate_fops_data', return_value=[]):
            check = FopsParser(self.params)
            check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
            check.query_fops_data()
        with mock.patch(NAMESPACE + '.FopsParser.get_fops_data', return_value=[]), \
                mock.patch(NAMESPACE + '.warn', return_value=[]):
            check = FopsParser(self.params)
            check.sample_config = {'ai_core_metrics': ''}
            check.query_fops_data()
        with mock.patch(NAMESPACE + '.FopsParser.get_fops_data', return_value=[]), \
                mock.patch(NAMESPACE + '.FopsParser.calculate_fops_data',
                           return_value=[(1, 2, 3, 4, 5, 6)]):
            check = FopsParser(self.params)
            check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
            check.query_fops_data()

    def test_get_cluster_path(self):
        with mock.patch('os.path.realpath', return_value='test\\query\\test\\test'),\
                mock.patch("os.path.exists", return_value=True):
            check = FopsParser(self.params)
            check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
            result = check.get_cluster_path('test\\test')
            self.assertEqual(result, 'test\\query\\test\\test')

    def test_check_id_valid(self):
        with DBOpen(DBNameConstant.DB_CLUSTER_RANK) as db_connect:
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect', return_value=[]):
                check = FopsParser(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                result = check.check_id_valid()
                self.assertFalse(result)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path',
                            return_value=(db_connect.db_conn, db_connect.db_curs)), \
                    mock.patch(NAMESPACE + '.DBManager.fetch_all_data',
                               return_value=[DataList(1, 2.3, 'test', 5, 'test')]), \
                    mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                check = FopsParser(self.params)
                check.sample_config = {'ai_core_metrics': 'ArithmeticUtilization'}
                result = check.check_id_valid()
                self.assertTrue(result)

    def test_get_cluster_path(self):
        with mock.patch('os.path.realpath', return_value=""), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch('os.makedirs'):
            check = FopsParser(self.params)
            check.get_cluster_path("test.bin")


if __name__ == '__main__':
    unittest.main()
