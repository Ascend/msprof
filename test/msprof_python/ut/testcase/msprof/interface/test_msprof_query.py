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
from argparse import Namespace
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from constant.constant import INFO_JSON
from msinterface.msprof_query import QueryCommand
from profiling_bean.basic_info.query_data_bean import QueryDataBean

NAMESPACE = 'msinterface.msprof_query'


class TestQueryCommand(unittest.TestCase):
    def test_process_should_raise_path_error_when_all_children_are_invalid(self):
        args = Namespace(collection_path="test", id=None, data_type=None, model_id=None, iteration_id=None)
        command = QueryCommand(args)
        with mock.patch.object(command, 'check_argument_valid'), \
                mock.patch.object(command, '_check_cluster_query', return_value=False), \
                mock.patch.object(command, '_get_query_data', return_value=[]), \
                self.assertRaises(ProfException) as context:
            command.process()

        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)

    def test_get_query_data_should_skip_invalid_child_and_process_valid_child(self):
        args = Namespace(collection_path="test")
        command = QueryCommand(args)
        expected_data = [[0, 1, 2, 3]]
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['invalid', 'valid']), \
                mock.patch(NAMESPACE + '.DataCheckManager.get_valid_profiling_sub_path',
                           side_effect=[('', False), ('valid', True)]), \
                mock.patch.object(command, '_do_get_query_data', return_value=expected_data):
            result = command._get_query_data_from_sub_dir('test')

        self.assertEqual(result, expected_data)
        self.assertEqual(command.valid_data_count, 1)

    def test_get_query_data_should_propagate_profiling_data_query_error(self):
        command = QueryCommand(Namespace(collection_path="test"))
        expected_error = ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device_0']), \
                mock.patch(NAMESPACE + '.DataCheckManager.get_valid_profiling_sub_path',
                           return_value=('valid/device_0', True)), \
                mock.patch.object(command, '_do_get_query_data', side_effect=expected_error), \
                self.assertRaises(ProfException) as context:
            command._get_query_data_from_sub_dir('test')

        self.assertIs(context.exception, expected_error)

    args_dic = {"collection_path": "123", "id": None, "data_type": None, "model_id": None, "iteration_id": None}
    args = Namespace(**args_dic)

    def test_check_argument_valid(self):
        with mock.patch('os.path.realpath', return_value=True), \
             mock.patch(NAMESPACE + '.check_path_valid'):
            key = QueryCommand(TestQueryCommand.args)
            key.check_argument_valid()

    def test_calculate_str_length(self):
        headers = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
        data = [[9, 8, 7], [6, 5, 4], [3, 2, 1]]
        key = QueryCommand(TestQueryCommand.args)
        result = key._calculate_str_length(headers, data)
        self.assertEqual(result, [3, 3, 3])

    def test_format_print_1(self):
        data = None
        headers = None
        key = QueryCommand(TestQueryCommand.args)
        key._format_print(data, headers)

    def test_format_print_2(self):
        data = 1
        headers = None
        with mock.patch(NAMESPACE + '.QueryCommand._calculate_str_length', return_value=[1, 2, 3]), \
             mock.patch(NAMESPACE + '.QueryCommand._show_query_header'), \
             mock.patch(NAMESPACE + '.QueryCommand._show_query_data'):
            key = QueryCommand(TestQueryCommand.args)
            key._format_print(data, headers)

    def test_show_query_data(self):
        data = [[1, 2], [3, 4], [5, 6]]
        max_column_list = [123, 456, 789]
        with mock.patch("builtins.print"):
            key = QueryCommand(TestQueryCommand.args)
            key._show_query_data(data, max_column_list)

    def test_show_query_header(self):
        headers = [[2, 1], [4, 3], [6, 5]]
        max_column_list = [321, 654, 987]
        with mock.patch("builtins.print"):
            key = QueryCommand(TestQueryCommand.args)
            key._show_query_header(headers, max_column_list)

    def test_do_get_query_data(self):
        result_dir = '123'
        query_data = {"job_info": '123', "job_name": 'ada', "model_id": 1, "device_id": 1,
                      "iteration_id": 123, "collection_time": 111, "parsed": False,
                      "top_time_iteration": "1,2", "rank_id":1}
        args_dic = {"collection_path": "123"}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.MsprofQueryData.query_data',
                        return_value=[QueryDataBean(**query_data)]), \
             mock.patch('os.listdir', return_value=['123']), \
             mock.patch(NAMESPACE + '.LoadInfoManager.load_info'):
            InfoConfReader()._info_json = INFO_JSON
            key = QueryCommand(args)
            result = key._do_get_query_data(result_dir)
            self.assertEqual(result, [['123', 1, 'ada', 111, 1, 123, "1,2", 1]])

    def test_get_query_data_without_sub_dir(self):
        args_dic = {"collection_path": "123"}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
             mock.patch(NAMESPACE + '.warn'), \
             mock.patch(NAMESPACE + '.QueryCommand._do_get_query_data', return_value=[1, 2]), \
             mock.patch('os.path.realpath', return_value=True), \
             mock.patch('os.listdir', return_value=['123']):
            key = QueryCommand(args)
            key._get_query_data()

    def test_get_query_data_with_sub_dir(self):
        args_dic = {"collection_path": "123"}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=False), \
             mock.patch(NAMESPACE + '.DataCheckManager.get_valid_profiling_sub_path',
                        side_effect=[('home\\process', True), ('home\\process', False)]), \
             mock.patch(NAMESPACE + '.warn'), \
             mock.patch(NAMESPACE + '.get_path_dir', return_value=[1, 2]), \
             mock.patch('os.path.join', return_value=True), \
             mock.patch('os.path.realpath', return_value='home\\process'), \
             mock.patch('os.listdir', return_value=['123']), \
             mock.patch(NAMESPACE + '.QueryCommand._do_get_query_data', return_value=[1, 2]):
            key = QueryCommand(args)
            key._get_query_data()

    def test_process_1(self):
        args_dic = {"collection_path": "123"}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.QueryCommand.check_argument_valid'), \
             mock.patch(NAMESPACE + '.QueryCommand._get_query_data', return_value=[[0, 1, 2, 3], ]), \
             mock.patch(NAMESPACE + '.QueryCommand._is_query_summary_data', return_value=False), \
             mock.patch(NAMESPACE + '.QueryCommand._format_print'):
            key = QueryCommand(args)
            key.process()

    def test_process_2(self):
        args_dic = {"collection_path": "123", "data_type": 0, "id": None, "model_id": None, "iteration_id": None}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.QueryCommand.check_argument_valid'), \
             mock.patch(NAMESPACE + '.QueryCommand._is_query_summary_data', return_value=True), \
             mock.patch('msinterface.msprof_query_summary_manager.MsprofQuerySummaryManager.check_rank_device_id',
                        return_value=True):
            key = QueryCommand(args)
            key.process()


if __name__ == '__main__':
    unittest.main()
