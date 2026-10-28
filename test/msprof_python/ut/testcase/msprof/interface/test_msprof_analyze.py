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
from argparse import Namespace
from unittest import mock

from msinterface.msprof_analyze import AnalyzeCommand

NAMESPACE = 'msinterface.msprof_analyze'


class TestAnalyzeCommand(unittest.TestCase):

    def test_analyze_command_clear_dir_with_unset_clear_mode(self):
        args_dic = {"collection_path": "test", "rule": "communication"}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        test._clear_dir("")

    def test_analyze_command_clear_dir_with_set_clear_mode_false(self):
        args_dic = {"collection_path": "test", "rule": "communication", "clear_mode": False}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        test._clear_dir("")

    def test_analyze_command_clear_dir_with_set_clear_mode_True(self):
        args_dic = {"collection_path": "test", "rule": "communication", "clear_mode": True}
        args = Namespace(**args_dic)
        test = AnalyzeCommand(args)
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device_0', 'host', 'timeline']), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.check_dir_writable'), \
                mock.patch('shutil.rmtree'):
            test._clear_dir("")

    def test_process_should_run_communication_rule_when_prof_level_0(self):
        args_dic = {"collection_path": "test", "rule": "communication"}
        args = Namespace(**args_dic)
        sample_config = {
            "profLevel": "l0",
        }
        with mock.patch('common_func.data_check_manager.get_path_dir', return_value=['device_0', 'host']), \
                mock.patch(NAMESPACE + '.warn'), \
                mock.patch('common_func.config_mgr.ConfigMgr.read_sample_config', return_value=sample_config), \
                mock.patch('os.path.exists', side_effect=(False, True)), \
                mock.patch(NAMESPACE + '.CommunicationAnalyzer.process') as communication_process:
            AnalyzeCommand(args).process()

        communication_process.assert_called_once()

    def test_process_should_skip_matrix_rule_when_prof_level_0(self):
        args_dic = {"collection_path": "test", "rule": "communication_matrix"}
        args = Namespace(**args_dic)
        sample_config = {
            "profLevel": "l0",
        }
        with mock.patch('common_func.data_check_manager.get_path_dir', return_value=['device_0', 'host']), \
                mock.patch(NAMESPACE + '.warn') as warning, \
                mock.patch('common_func.config_mgr.ConfigMgr.read_sample_config', return_value=sample_config), \
                mock.patch('os.path.exists', side_effect=(False, True)), \
                mock.patch(NAMESPACE + '.CommunicationMatrixAnalyzer.process') as matrix_process:
            AnalyzeCommand(args).process()

        matrix_process.assert_not_called()
        warning.assert_called_once_with(
            'msprof_analyze.py', 'Analyze rule "communication_matrix" will do nothing in prof level 0.'
        )

    def test_process_should_dispatch_analyzer_with_db_export_type(self):
        args_dic = {"collection_path": "test", "rule": "communication", "export_type": "db"}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.DataCheckManager.check_prof_level0', return_value=False), \
                mock.patch(NAMESPACE + '.CommunicationAnalyzer') as analyzer_cls:
            AnalyzeCommand(args).process()

        analyzer_cls.assert_called_once_with(os.path.realpath("test"), "db")
        analyzer_cls.return_value.process.assert_called_once()

if __name__ == '__main__':
    unittest.main()
