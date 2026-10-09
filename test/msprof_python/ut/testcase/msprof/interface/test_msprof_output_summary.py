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
from io import StringIO
from unittest import mock
from unittest.mock import MagicMock

from common_func.file_slice_helper import FileSliceHelper
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from msinterface.msprof_output_summary import MsprofOutputSummary

NAMESPACE = 'msinterface.msprof_output_summary'
DEVICE_DIR_1 = "device_0"
PROF_DIR = "test"
HOST_DIR = 'host'
CSV_FORMAT = "csv"


class TestMsprofOutputSummary(unittest.TestCase):

    def test_export_when_not_in_prof_file_then_pass(self):
        with mock.patch('common_func.common.print_info'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=False):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT).export(MsProfCommonConstant.SUMMARY)

    def test_export_when_in_prof_file_but_can_not_clear_file_then_return(self):
        with mock.patch('common_func.common.print_info'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_suffix'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._make_output_folder', return_value=False), \
                mock.patch('logging.error'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT).export(MsProfCommonConstant.SUMMARY)

    def test_export_when_in_prof_file_and_export_summary_then_pass(self):
        with mock.patch('common_func.common.print_info'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_suffix'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._make_output_folder', return_value=True):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._export_msprof_summary = mock.Mock()
            check._export_readme_file = mock.Mock()
            check.export(MsProfCommonConstant.SUMMARY)
            check._export_msprof_summary.assert_called_once()
            check._export_readme_file.assert_called_once()

    def test_export_when_in_prof_file_and_export_timeline_then_pass(self):
        with mock.patch('common_func.common.print_info'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_suffix'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._make_output_folder', return_value=True):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._export_msprof_timeline = mock.Mock()
            check._export_readme_file = mock.Mock()
            check.export(MsProfCommonConstant.TIMELINE)
            check._export_msprof_timeline.assert_called_once()
            check._export_readme_file.assert_called_once()

    def test_get_file_suffix_when_summary_then_return_csv_suffix(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check._get_file_suffix(MsProfCommonConstant.SUMMARY), StrConstant.FILE_SUFFIX_CSV)

    def test_get_file_suffix_when_timeline_then_return_json_suffix(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check._get_file_suffix(MsProfCommonConstant.TIMELINE), StrConstant.FILE_SUFFIX_JSON)

    def test_get_file_suffix_when_invalid_then_return_csv_suffix(self):
        with mock.patch('logging.error'), \
                mock.patch('sys.exit'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertEqual(check._get_file_suffix("abc"), check.INVALID_SUFFIX)

    def test_make_output_folder_when_suffix_invalid_then_return_False(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertFalse(check._make_output_folder(check.INVALID_SUFFIX))

    def test_make_output_folder_when_path_not_exist_then_mkdir_return_True(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.exists', return_value=False), \
                mock.patch('os.makedirs'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertTrue(check._make_output_folder(StrConstant.FILE_SUFFIX_CSV))

    def test_make_output_folder_when_path_exist_then_move_file_succes_return_True(self):
        with mock.patch('os.path.join', return_value="readme.txt"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["readme.txt", "msprof.json", "op_summary.csv"]), \
                mock.patch('common_func.msvp_common.check_dir_writable', ), \
                mock.patch('common_func.msvp_common.check_path_valid'), \
                mock.patch('os.path.isfile', return_value=True), \
                mock.patch('os.path.getsize', return_value=1000), \
                mock.patch('os.access', return_value=True), \
                mock.patch('os.remove'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertTrue(check._make_output_folder(StrConstant.FILE_SUFFIX_CSV))

    def test_is_in_prof_file_when_in_prof_file_then_return_True(self):
        with mock.patch('os.listdir', return_value=["host", "123"]):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertTrue(check._is_in_prof_file())

        with mock.patch('os.listdir', return_value=["device_1", "123"]):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertTrue(check._is_in_prof_file())

    def test_is_in_prof_file_when_not_in_prof_file_then_return_False(self):
        with mock.patch('os.listdir', return_value=["123"]):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertFalse(check._is_in_prof_file())

    def test_export_msprof_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=[HOST_DIR, DEVICE_DIR_1, 'device_1']), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_summary_file_name',
                           return_value=set({"op_summary.csv"})), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._save_summary_data'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._copy_summary_data'), \
                mock.patch('multiprocessing.Pool'), \
                mock.patch('multiprocessing.pool.Pool.apply_async'), \
                mock.patch('multiprocessing.pool.Pool.close'), \
                mock.patch('multiprocessing.pool.Pool.join'):
            check = MsprofOutputSummary(PROF_DIR, "json")
            check._export_msprof_summary()
            check = MsprofOutputSummary(PROF_DIR, "timeline")
            check._export_msprof_summary()

    def test_export_msprof_summary_called_once_then_pass(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        check._merge_summary = mock.Mock()
        check._export_msprof_summary()
        check._merge_summary.assert_called_once()

    def test_copy_summary_data_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="device_0"), \
                mock.patch('os.path.realpath', return_value="device_0"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"), \
                mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch('shutil.copy'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._copy_summary_data("device_0", ".csv")

    def test_copy_summary_data_when_file_name_is_prof_rule_then_pass(self):
        with mock.patch('os.path.join', return_value="device_0"), \
                mock.patch('os.path.realpath', return_value="device_0"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary_0_1_1_20230905213000.csv", "prof_rule_0.json"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv", "prof_rule_0.json"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"), \
                mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name',
                           return_value=["op_summary_0_1_1_20230905213000.csv", "prof_rule_0.json"]), \
                mock.patch('shutil.copy'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._copy_summary_data("device_0", ".json")

    def test_copy_summary_data_when_export_format_is_json_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.json"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"), \
                mock.patch(NAMESPACE + '.FileSliceHelper.make_export_file_name',
                           return_value=["op_summary_0_1_1_20230905213000.json"]), \
                mock.patch('shutil.copy'):
            MsprofOutputSummary(PROF_DIR, "json")._copy_summary_data("test", ".json")

    def test_merge_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=[HOST_DIR, DEVICE_DIR_1, 'device_1']), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_summary_file_name',
                           return_value=set({"op_summary.csv"})), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._save_summary_data'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._copy_summary_data'), \
                mock.patch('multiprocessing.Pool'), \
                mock.patch('multiprocessing.pool.Pool.apply_async'), \
                mock.patch('multiprocessing.pool.Pool.close'), \
                mock.patch('multiprocessing.pool.Pool.join'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._merge_summary()

    def test_get_summary_file_name_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"):
            file_set = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._get_summary_file_name("test")
            self.assertEqual(len(file_set), 1)

    def test_save_summary_data_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv", "aicpu.csv"]), \
                mock.patch('os.path.basename', return_value="0"), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["aicpu_0_1_1_20230905213000.csv", "aicpu_mi_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._insert_summary_data'), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.dump_csv_data'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._save_summary_data("aicpu",
                                                                         [HOST_DIR, DEVICE_DIR_1, 'device_1'])

    def test_save_msproftx_summary_data_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "msprof_tx.csv", "aicpu.csv"]), \
                mock.patch('os.path.basename', return_value="0"), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["msprof_tx_0_0_1_20241107020650.csv", "msprof_tx_0_0_2_20241107020650.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._insert_msproftx_summary_data'), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.dump_csv_data'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._save_summary_data("msprof_tx",
                                                                         [HOST_DIR, DEVICE_DIR_1, 'device_1'])

    def test_insert_msproftx_summary_data_when_host_then_pass(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "msprof_tx",
            StrConstant.PARAM_EXPORT_TYPE: "summary",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "summary"
        }
        helper = FileSliceHelper(params, [], [])
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._update_msproftx_host_data'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._insert_msproftx_summary_data("msprof_tx", "host", helper, HOST_DIR)

    def test_insert_msproftx_summary_data_when_device_then_pass(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "msprof_tx",
            StrConstant.PARAM_EXPORT_TYPE: "summary",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "summary"
        }
        helper = FileSliceHelper(params, [], [])
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._update_msproftx_device_data'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._insert_msproftx_summary_data("msprof_tx", "device", helper, DEVICE_DIR_1)

    def test_update_msproftx_device_data_when_more_than_1000000_then_pass(self):
        mock_iterator = MagicMock()
        mock_iterator.__iter__.return_value = iter(['1,2,10'] * 1000001)
        with mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.read_file', return_value=mock_iterator), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.check_header_is_empty',
                           return_value=True), \
                mock.patch('common_func.msvp_common.create_normal_writer'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._update_msproftx_device_data("msprof_tx", '0')
            self.assertEqual(len(check._msproftx_device_data_dict), 0)

    def test_update_msproftx_host_data_when_more_than_1000000_then_pass(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "msprof_tx",
            StrConstant.PARAM_EXPORT_TYPE: "summary",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "summary"
        }
        helper = FileSliceHelper(params, [], [])
        mock_iterator = MagicMock()
        mock_iterator.__iter__.return_value = iter(['1'] * 1000001)
        with mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.read_file', return_value=mock_iterator), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.check_header_is_empty',
                           return_value=True), \
                mock.patch('common_func.msvp_common.create_normal_writer'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._update_msproftx_host_data("msprof_tx", helper)
            self.assertEqual(len(helper.data_list), 0)

    def test_insert_summary_data_when_more_than_1000000_then_insert_data(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "op_summary",
            StrConstant.PARAM_EXPORT_TYPE: "summary",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "summary"
        }
        helper = FileSliceHelper(params, [], [])
        mock_iterator = MagicMock()
        mock_iterator.__iter__.return_value = iter(['1,2,3'] * 1000001)
        with mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.read_file', return_value=mock_iterator), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.check_header_is_empty',
                           return_value=True), \
                mock.patch('common_func.msvp_common.create_normal_writer'):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            check._insert_summary_data("op_summary", "test", helper)
            self.assertEqual(len(helper.data_list), 0)

    def test_get_newest_file_list_when_csv_normal_filename_then_pass(self):
        _file_list = [
            "op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
            "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
            "msprof_20230905191336.json", "msprof_20230905171336.json",
            "step_trace_20230905191336.json", "step_trace_20230805191336.json"
        ]
        data_type = StrConstant.FILE_SUFFIX_CSV
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check.get_newest_file_list(_file_list, data_type),
                         ["op_summary_20230905191336.csv", "step_trace_20230905191336.csv"])

    def test_get_newest_file_list_when_json_normal_filename_then_pass(self):
        _file_list = [
            "op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
            "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
            "msprof_20230905191336.json", "msprof_20230905171336.json",
            "step_trace_20230905191336.json", "step_trace_20230805191336.json"
        ]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check.get_newest_file_list(_file_list, data_type),
                         ["msprof_20230905191336.json", "step_trace_20230905191336.json"])

    def test_get_newest_file_list_when_invalid_filename_then_return_empty(self):
        _file_list = [
            "op_summary_20230905191336", "op.csv", "step_trace_qwer.csv",
            "_.csv", "step_trace_qwer.csv_test", "step_trace_qwer_.csv"
        ]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check.get_newest_file_list(_file_list, data_type), [])

    def test_read_file_when_normal_then_pass(self):
        input_str = "data1"
        output_list = ["data1"]
        reader = StringIO(input_str)
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        res = list(check.read_file(reader))
        self.assertEqual(res, output_list)

    def test_get_file_name_when_normal_name_then_pass(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check._get_file_name("msprof_20230905191336.json"), "msprof")

    def test_get_file_name_when_invalid_name_then_pass(self):
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        self.assertEqual(check._get_file_name("msprof.json"), "invalid")

        self.assertEqual(check._get_file_name("2msprof.json"), "invalid")

    def test_export_msprof_timeline_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._export_all_timeline_data'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._export_msprof_timeline()

    def test_export_all_timeline_data_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._save_timeline_data'), \
                mock.patch('multiprocessing.Process.start'), \
                mock.patch('multiprocessing.Process.join'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._export_all_timeline_data()

    def test_save_timeline_data_when_normal_then_pass(self):
        with mock.patch('os.path.realpath', return_value=PROF_DIR), \
                mock.patch('os.path.join', return_value=PROF_DIR), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('common_func.data_check_manager.DataCheckManager.contain_info_json_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_timeline_file_with_slice',
                           return_value=({}, 2)), \
                mock.patch('common_func.utils.Utils.get_json_data'), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.dump_json_data'), \
                mock.patch('multiprocessing.Pool'), \
                mock.patch('multiprocessing.pool.Pool.apply_async'), \
                mock.patch('multiprocessing.pool.Pool.close'), \
                mock.patch('multiprocessing.pool.Pool.join'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._save_timeline_data("msprof", ["test"])

    def test_insert_json_data_when_device_is_1_then_copy(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "msprof",
            StrConstant.PARAM_EXPORT_TYPE: "timeline",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "timeline"
        }
        with mock.patch('shutil.copy'):
            helper = FileSliceHelper(params, [], [])
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            helper.insert_data = mock.Mock()
            helper.dump_json_data = mock.Mock()
            check._insert_json_data(["msprof_slice_0_20231216.json"], helper, True, 1)
            helper.insert_data.assert_not_called()
            helper.dump_json_data.assert_not_called()

    def test_insert_json_data_when_device_more_than_1_then_merge_json(self):
        params = {
            StrConstant.PARAM_DATA_TYPE: "msprof",
            StrConstant.PARAM_EXPORT_TYPE: "timeline",
            StrConstant.PARAM_EXPORT_FORMAT: CSV_FORMAT,
            StrConstant.PARAM_RESULT_DIR: "test",
            StrConstant.PARAM_EXPORT_DUMP_FOLDER: "timeline"
        }
        helper = FileSliceHelper(params, [], [])
        check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
        helper.insert_data = mock.Mock()
        helper.dump_json_data = mock.Mock()
        check._insert_json_data(["msprof_slice_0_20231216.json", "msprof_slice_0_20231216.json"], helper, True, 1)
        helper.insert_data.assert_called()
        helper.dump_json_data.assert_called_once()

    def test_get_timeline_file_with_slice_when_not_exist_path_then_return_empty(self):
        with mock.patch('os.path.realpath', return_value=PROF_DIR), \
                mock.patch('os.path.join', return_value=PROF_DIR), \
                mock.patch('os.path.exists', return_value=False):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertEqual(check._get_timeline_file_with_slice("qwer", "test_path", {}),
                             ({}, 0))

    def test_get_timeline_file_with_slice_when_normal_then_get_timeline_file_with_slice_count(self):
        with mock.patch('os.path.realpath', return_value=PROF_DIR), \
                mock.patch('os.path.join', return_value=PROF_DIR), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=[]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["msprof_slice_1_20230923193400.json",
                                         "step_trace_slice_1_20230923193400.json"]):
            check = MsprofOutputSummary(PROF_DIR, CSV_FORMAT)
            self.assertEqual(check._get_timeline_file_with_slice("msprof", "test_path", {}),
                             ({1: [PROF_DIR]}, 1))

    def test_export_readme_file_normal_then_pass(self):
        with mock.patch('msconfig.config_manager.ConfigManager.get',
                        DATA={"timeline": ["op_summary.json"], "summary": ["op_summary.csv"]}), \
                mock.patch('os.listdir'), \
                mock.patch('os.path.join'), \
                mock.patch('os.open'), \
                mock.patch('os.fdopen'), \
                mock.patch('os.fdopen.write'), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_readme_info'):
            MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._export_readme_file()

    def test_get_readme_info_when_normal_then_get_desc(self):
        file_dict = {"begin": "hello", "step_trace": "qwer", "123": "zxc"}
        file_set = set()
        file_set.add("step_trace")
        self.assertEqual(MsprofOutputSummary(PROF_DIR, CSV_FORMAT)._get_readme_info(file_set, file_dict, ".json"),
                         "hello1.step_trace.json:qwer\n")
