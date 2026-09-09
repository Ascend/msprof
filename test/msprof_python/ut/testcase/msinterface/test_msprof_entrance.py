#!/usr/bin/env python
# coding=utf-8
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import tempfile
import unittest
from argparse import Namespace, ArgumentTypeError
from contextlib import ExitStack
from pathlib import Path
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ExportMode
from common_func.profiling_scene import ProfilingScene
from msinterface.msprof_entrance import MsprofEntrance

NAMESPACE = 'msinterface.msprof_entrance'


class TestMsprofEntrance(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        if os.path.exists("test_entrance_dir"):
            import shutil
            shutil.rmtree("test_entrance_dir")
        os.makedirs("test_entrance_dir/subdir", exist_ok=True)

    @classmethod
    def tearDownClass(cls):
        import shutil
        shutil.rmtree("test_entrance_dir")

    def test_main(self):
        args = ["msprof_entrance.py", "query", "-dir", "test_entrance_dir"]
        collection_path = {"collection_path": "test_entrance_dir"}
        args_co = Namespace(**collection_path)
        with mock.patch('sys.argv', args), \
                mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
                mock.patch(NAMESPACE + '.QueryCommand.process'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()

    def test_main_should_dispatch_all_commands_when_child_dir_cannot_be_scanned(self):
        command_cases = (
            (["import"], 'msinterface.msprof_import.ImportCommand._start_parse'),
            (["export", "summary"], 'msinterface.msprof_export.ExportCommand._start_parse'),
            (["query"], 'msinterface.msprof_query.QueryCommand._do_get_query_data'),
            (["analyze", "--rule", "communication"],
             'analyzer.communication_analyzer.CommunicationAnalyzer._communication_analyze'),
        )
        real_listdir = os.listdir
        with tempfile.TemporaryDirectory() as root:
            device_path = os.path.join(root, 'device_0')
            invalid_path = os.path.join(root, 'invalid_dir')
            os.mkdir(device_path)
            os.mkdir(invalid_path)
            Path(device_path, 'info.json.0').write_text('{}', encoding='utf-8')

            def listdir_with_invalid_child(path):
                if os.path.realpath(path) == invalid_path:
                    raise PermissionError('invalid child directory')
                return real_listdir(path)

            for command_args, processing_path in command_cases:
                with self.subTest(command=command_args[0]), ExitStack() as stack:
                    stack.enter_context(mock.patch('sys.argv', ['msprof.py', *command_args, '-dir', root]))
                    scan = stack.enter_context(mock.patch('os.listdir', side_effect=listdir_with_invalid_child))
                    stack.enter_context(mock.patch('framework.load_info_manager.LoadInfoManager.load_info'))
                    stack.enter_context(mock.patch('msinterface.msprof_export.run_in_subprocess',
                                                   side_effect=lambda func, *args: func(*args)))
                    stack.enter_context(mock.patch('msinterface.msprof_export.ExportCommand._start_calculate'))
                    stack.enter_context(mock.patch('msinterface.msprof_export.ExportCommand._start_view'))
                    processing = stack.enter_context(mock.patch(processing_path, return_value=[]))
                    with self.assertRaises(SystemExit) as context:
                        MsprofEntrance().main()
                    self.assertEqual(context.exception.code, 0)
                    scan.assert_any_call(invalid_path)
                    processing.assert_called_once()
                    if command_args[0] in ('import', 'export'):
                        self.assertEqual(processing.call_args.args[0]['device'], [device_path])
                    else:
                        processing.assert_called_once_with(device_path)

    def test_main_should_set_all_export_false_when_set_model_id_and_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test_entrance_dir", "--model-id", "1", "--iteration-id", "2"]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()
            self.assertFalse(ProfilingScene().is_all_export())

    def test_main_should_print_error_when_set_model_id_but_no_iteration_id(self):
        args = ["msprof.py", "export", "summary", "-dir", "test_entrance_dir", "--model-id", "1"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()

    def test_main_should_raise_system_exit_when_no_argument(self):
        args = ["msprof.py"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_raise_system_exit_when_no_argument_with_export(self):
        args = ["msprof.py", "export"]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_raise_system_exit_when_dir_len_is_over_1024(self):
        args = ["msprof.py", "export", "summary", "-dir", "t" * 1025]
        with mock.patch('sys.argv', args), \
                mock.patch('common_func.common.error'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.check_path_char_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_main_should_success_when_analyze_args_is_valid(self):
        # 只能满足覆盖率要求，无法校验功能
        args = ["msprof.py", "analyze", "--rule", "communication", "--clear", "--type", "text", "-dir",
                "test_entrance_dir"]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

        args = [
            "msprof.py", "analyze", "--rule", "communication,communication_matrix",
            "--clear", "--type", "db", "-dir", "test_entrance_dir"
        ]
        with mock.patch('sys.argv', args), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand.process'):
            with self.assertRaises(SystemExit):
                MsprofEntrance().main()

    def test_set_export_mode(self):
        args_dic = {"collection_path": "test_entrance_dir", "iteration_id": 1, "model_id": 4294967295,
                    "iteration_count": None}
        InfoConfReader()._info_json = {"drvVersion": 0}
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        args = Namespace(**args_dic)
        with mock.patch('sys.argv', args):
            MsprofEntrance()._set_export_mode(args)

    def test_validate_analyze_rule_should_return_valid_str_when_str_is_valid(self):
        analyze_rule = "communication"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "communication_matrix"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "communication,communication_matrix"
        self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

    def test_validate_analyze_rule_should_raise_error_when_type_is_invalid(self):
        analyze_rule = "xxx,communication_matrix"
        with self.assertRaises(ArgumentTypeError):
            self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

        analyze_rule = "xxx"
        with self.assertRaises(ArgumentTypeError):
            self.assertEqual(analyze_rule, MsprofEntrance()._validate_analyze_rule(analyze_rule))

