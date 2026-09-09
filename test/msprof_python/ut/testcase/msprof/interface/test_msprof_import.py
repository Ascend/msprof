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
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ExportMode, ProfilingScene
from msinterface.msprof_import import ImportCommand

NAMESPACE = 'msinterface.msprof_import'


class TestImportCommand(unittest.TestCase):
    def test_prepare_for_cluster_parse_should_prepare_root_without_loading_device_info(self):
        command = ImportCommand(Namespace(collection_path="test", cluster_flag=True))
        with mock.patch(NAMESPACE + '.os.path.exists', return_value=False), \
                mock.patch(NAMESPACE + '.prepare_for_parse') as prepare, \
                mock.patch('framework.load_info_manager.LoadInfoManager.load_info') as load_info:
            command._prepare_for_cluster_parse()
        prepare.assert_called_once_with(command.collection_path)
        load_info.assert_not_called()

    def test_process_should_raise_path_error_when_all_children_are_invalid(self):
        command = ImportCommand(Namespace(collection_path="test", cluster_flag=False))
        with mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch.object(command, '_process_sub_dirs'), \
                self.assertRaises(ProfException) as context:
            command.process()

        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)

    def test_process_sub_dirs_should_skip_invalid_child_and_process_valid_child(self):
        args = Namespace(collection_path="test", cluster_flag=False)
        command = ImportCommand(args)
        with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths',
                        return_value=[('device_0', 'valid/device_0', True)]), \
                mock.patch.object(command, '_start_parse') as start_parse:
            command._process_sub_dirs()

        self.assertEqual(command.valid_data_count, 1)
        start_parse.assert_called_once()

    def test_process_sub_dirs_should_propagate_profiling_data_parse_error(self):
        command = ImportCommand(Namespace(collection_path="test", cluster_flag=False))
        expected_error = ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths',
                        return_value=[('device_0', 'valid/device_0', True)]), \
                mock.patch.object(command, '_start_parse', side_effect=expected_error), \
                self.assertRaises(ProfException) as context:
            command._process_sub_dirs()

        self.assertIs(context.exception, expected_error)

    def test_do_import(self):
        result_dir = '123'
        args_dic = {"collection_path": "test", "cluster_flag": False}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=True), \
                mock.patch(NAMESPACE + '.analyze_collect_data'):
            key = ImportCommand(args)
            key.do_import(result_dir)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=True), \
                mock.patch(NAMESPACE + '.analyze_collect_data'):
            key = ImportCommand(args)
            key.do_import(result_dir)

    def test_process(self):
        with mock.patch(NAMESPACE + '.check_path_valid'):
            args_dic = {"collection_path": "test", "cluster_flag": False}
            args = Namespace(**args_dic)
            with mock.patch(NAMESPACE + '.ImportCommand._parse_data'), \
                    mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths',
                               return_value=[('host', 'host', True)]), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('os.path.realpath', return_value='home\\process'), \
                    mock.patch('msinterface.msprof_c_interface.dump_device_data'), \
                    mock.patch('os.listdir', return_value=['123']):
                ChipManager().chip_id = 5
                key = ImportCommand(args)
                key.process()
                with mock.patch(NAMESPACE + '.DataCheckManager.iter_valid_profiling_sub_paths', return_value=[]), \
                        mock.patch(NAMESPACE + '.warn'), \
                        mock.patch('os.listdir', return_value=['123']):
                    key = ImportCommand(args)
                    with self.assertRaises(ProfException) as context:
                        key.process()
                    self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)

    def test_parse_unresolved_dirs(self):
        unresolved_dirs = {'pro_dir': ['result_dir']}
        args_dic = {"collection_path": "test", "cluster_flag": False}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.prepare_and_load_info'), \
                mock.patch('os.path.join', return_value='test\\path'), \
                mock.patch(NAMESPACE + '.get_path_dir', return_value=['host', 'device_1', 'device_2', 'device_3']), \
                mock.patch(NAMESPACE + '.get_valid_sub_path'), \
                mock.patch('common_func.msprof_common.prepare_log'), \
                mock.patch('common_func.config_mgr.ConfigMgr.is_ai_core_sample_based'), \
                mock.patch(NAMESPACE + '.ImportCommand.do_import'), \
                mock.patch('importlib.import_module'):
            InfoConfReader()._info_json = {"drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
            InfoConfReader()._sample_json = {"devices": "5"}
            ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
            key = ImportCommand(args)
            key._parse_unresolved_dirs(unresolved_dirs)
            InfoConfReader()._info_json = {}
            InfoConfReader()._sample_json = {}


if __name__ == '__main__':
    unittest.main()
