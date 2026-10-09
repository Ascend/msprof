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
from unittest.mock import MagicMock
from datetime import datetime
from datetime import timezone
from unittest import mock

from common_func.ms_constant.str_constant import StrConstant
from common_func.constant import Constant
from common_func.file_slice_helper import FileSliceHelper
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode

NAMESPACE = 'common_func.file_slice_helper'

PARAMS_SUMMARY = {
    StrConstant.PARAM_DATA_TYPE: "test",
    StrConstant.PARAM_EXPORT_TYPE: "summary",
    StrConstant.PARAM_EXPORT_FORMAT: "csv",
    StrConstant.PARAM_RESULT_DIR: "test",
    StrConstant.PARAM_EXPORT_DUMP_FOLDER: "summary"
}
PARAMS_TIMELINE = {
    StrConstant.PARAM_DATA_TYPE: "test",
    StrConstant.PARAM_EXPORT_TYPE: "timeline",
    StrConstant.PARAM_EXPORT_FORMAT: "csv",
    StrConstant.PARAM_RESULT_DIR: "test",
    StrConstant.PARAM_EXPORT_DUMP_FOLDER: "timeline"
}


class TestFileSliceHelper(unittest.TestCase):
    def test_set_header_when_normal_then_pass(self):
        check = FileSliceHelper(PARAMS_SUMMARY, [], [])
        check.set_header(["1"])

    def test_check_header_is_empty_when_empty_return_false(self):
        check = FileSliceHelper(PARAMS_SUMMARY, [], [])
        check.header = ["1"]
        self.assertFalse(check.check_header_is_empty())

    def test_check_header_is_empty_when_exist_return_true(self):
        check = FileSliceHelper(PARAMS_SUMMARY, [], [])
        self.assertTrue(check.check_header_is_empty())

    def test_insert_data_when_summary_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.dump_csv_data'):
            check = FileSliceHelper(PARAMS_SUMMARY, [], [])
            check.insert_data(["1"])

    def test_insert_data_when_timeline_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper._pretreat_json_data'):
            check = FileSliceHelper(PARAMS_TIMELINE, [], [])
            check.insert_data(["1"])

    def test_dump_csv_data_when_force_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name'), \
                mock.patch(NAMESPACE + '.create_csv'):
            check = FileSliceHelper(PARAMS_SUMMARY, [], [])
            check.data_list = ["1"]
            check.dump_csv_data(force=True)

    def test_dump_csv_data_when_not_force_but_exceeded_limit_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name'), \
                mock.patch(NAMESPACE + '.create_csv'):
            check = FileSliceHelper(PARAMS_SUMMARY, [], [])
            check.data_list = ["1"] * 1000001
            check.dump_csv_data()

    def test_data_deduplication_when_exist_header_data_then_get_header(self):
        json_data = [{"name": "acl@qwer", "ph": "M"}, {"name": "abcd"}, {"name": "efgh"}]
        check = FileSliceHelper(PARAMS_TIMELINE, [], [])
        check._pretreat_json_data(json_data)
        self.assertEqual(check.data_list, [{"name": "abcd"}, {"name": "efgh"}])
        self.assertEqual(check.header, [{"name": "acl@qwer", "ph": "M"}])

    def test_data_deduplication_when_first_data_with_cann_then_return_all_data(self):
        json_data = [
            {"name": "acl@qwer", "args": {"connection_id": 1}},
            {"name": "abcd"}, {"name": "efgh"}
        ]
        check = FileSliceHelper(PARAMS_TIMELINE, [], [])
        check._pretreat_json_data(json_data)
        self.assertEqual(check.data_list, json_data)
        self.assertEqual(check.header, [])

    def test_data_deduplication_when_exist_data_with_cann_then_return_all_data(self):
        json_data = [
            {"name": "acl@qwer", "args": {"connection_id": 1}},
            {"name": "acl@yuio", "args": {"connection_id": 2}},
            {"name": "abcd"}, {"name": "efgh"}
        ]
        check = FileSliceHelper(PARAMS_TIMELINE, [], [])
        check.data_list = [{"name": "acl@1234"}]
        check.connection_id_set.add(2)
        check._pretreat_json_data(json_data)
        self.assertEqual(check.data_list,
                         [{"name": "acl@1234"},
                          {'args': {'connection_id': 1}, 'name': 'acl@qwer'},
                          {'name': 'abcd'}, {'name': 'efgh'}])
        self.assertEqual(check.header, [])

    def test_dump_json_data_when_force_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name'), \
                mock.patch(NAMESPACE + '.FileSliceHelper._create_json'):
            check = FileSliceHelper(PARAMS_TIMELINE, [], [])
            check.data_list = [{"1": 0}]
            check.dump_csv_data(force=True)

    def test_dump_json_data_when_not_force_but_exceeded_limit_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name'), \
                mock.patch(NAMESPACE + '.FileSliceHelper._create_json'):
            check = FileSliceHelper(PARAMS_TIMELINE, [], [])
            check.data_list = [{"1": 0}] * 3000000
            check.dump_json_data(0, is_need_slice=False)

    def test_create_json_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'):
            check = FileSliceHelper(PARAMS_TIMELINE, [], [])
            check.data_list = [{"1": 0}]
            check._create_json("test")


    def test_make_export_file_name_should_return_csv_name_when_give_summary_json(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "summary", "project": "",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: ""
        }
        with mock.patch(NAMESPACE + '.FileSliceHelper.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            unittest.TestCase().assertEqual(FileSliceHelper.make_export_file_name(params),
                                            "abc_0_1_1_slice_0_20230905200300.csv")


    def test_make_export_file_name_should_return_json_name_when_give_timeline_json(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "timeline", "project": "",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: ""
        }
        with mock.patch(NAMESPACE + '.FileSliceHelper.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            unittest.TestCase().assertEqual(FileSliceHelper.make_export_file_name(params),
                                            "abc_0_1_1_slice_0_20230905200300.json")

    def test_get_current_time_str_should_return_success_when_input_data_is_valid(self):
        utc_time = datetime.now(tz=timezone.utc)
        current_time = utc_time.replace(tzinfo=timezone.utc).astimezone(tz=None)
        self.assertEqual(FileSliceHelper.get_current_time_str(), current_time.strftime('%Y%m%d%H%M%S'))

    def test_get_current_time_str_should_return_success_when_data_length_is_14(self):
        self.assertEqual(len(FileSliceHelper.get_current_time_str()), 14)

    def test_get_export_prefix_file_name_should_return_name_when_normal(self):
        params = {"data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1"}
        ProfilingScene().init(" ")
        ProfilingScene()._scene = Constant.STEP_INFO
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch(NAMESPACE + '.FileSliceHelper.get_current_time_str', return_value="20230905200300"):
            self.assertEqual(FileSliceHelper.get_export_prefix_file_name(params, 0, True),
                            "abc_0_1_1_slice_0_20230905200300")
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_make_export_file_name_should_return_json_name_when_give_summary_json(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "summary", "export_format": "json", "project": "",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: ""
        }
        with mock.patch(NAMESPACE + '.FileSliceHelper.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            self.assertEqual(FileSliceHelper.make_export_file_name(params),
                            "abc_0_1_1_slice_0_20230905200300.json")

    def test_make_export_file_name_should_return_csv_name_when_give_summary_csv(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "summary", "export_format": "csv", "project": "",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: ""
        }
        with mock.patch(NAMESPACE + '.FileSliceHelper.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            self.assertEqual(FileSliceHelper.make_export_file_name(params),
                            "abc_0_1_1_slice_0_20230905200300.csv")

if __name__ == '__main__':
    unittest.main()
