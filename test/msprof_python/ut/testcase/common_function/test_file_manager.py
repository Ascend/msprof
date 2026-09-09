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
import shutil
import unittest
from collections import namedtuple
from unittest import mock
from unittest.mock import Mock

from common_func.common import is_linux
from common_func.constant import Constant
from common_func.file_manager import FileManager, is_root_user
from common_func.file_manager import check_db_path_valid
from common_func.file_manager import check_dir_readable
from common_func.file_manager import check_dir_writable
from common_func.file_manager import check_file_readable
from common_func.file_manager import check_file_writable
from common_func.file_manager import check_path_valid
from common_func.file_manager import check_so_valid
from common_func.file_manager import is_link
from common_func.file_name_manager import FileNameManagerConstant
from common_func.msprof_exception import ProfException

NAMESPACE = 'common_func.file_manager'


class TestFileManager(unittest.TestCase):

    def test_is_analyzed_data(self):
        os.mkdir("test_file_manager", 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        os.mkdir(os.path.join("test_file_manager", "data"), 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        with os.fdopen(os.open(os.path.join("test_file_manager/data", FileNameManagerConstant.ALL_FILE_TAG),
                               Constant.WRITE_FLAGS, Constant.WRITE_MODES), "w"):
            pass
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        os.mkdir(os.path.join("test_file_manager", "sqlite"), 0o777)
        self.assertEqual(False, FileManager.is_analyzed_data("test_file_manager"))
        os.mkdir(os.path.join("test_file_manager", "sqlite", "op_summary.db"), 0o777)
        self.assertEqual(True, FileManager.is_analyzed_data("test_file_manager"))
        shutil.rmtree("test_file_manager")

    def test_check_db_path_valid(self):
        with mock.patch("common_func.return_code_checker.ReturnCodeCheck.print_and_return_status"), \
                mock.patch("os.path.exists", return_value=True), \
                mock.patch("os.path.getsize", return_value=2):

            check_db_path_valid("test.txt", False, max_size=1)

    def test_check_path_valid_should_return_error_when_is_file_and_path_not_exists(self):
        path = 'test.cpp'
        is_file = True
        with mock.patch('os.path.exists', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"test.cpp\" does not exist. Please check that the path exists.")

    def test_check_path_valid_should_return_error_when_is_not_file_and_path_not_exists(self):
        path = '/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/host/host\" does not exist. Please check that the path exists.")

    def test_check_path_valid_should_return_error_when_dir_path_is_empty(self):
        path = ''
        is_file = False
        with mock.patch('os.path.exists', return_value=True):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PARAM_ERROR)
        self.assertEqual(str(context.exception),
                         "The path is empty. Please enter a valid path.")

    def test_check_path_valid_should_return_error_when_dir_path_is_too_long(self):
        def custom_len(obj):
            return 5000
        path = '/path/prof_0'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch(NAMESPACE + '.len', custom_len):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "Please ensure the length of input path \"/path/prof_0\" less than 4096")

    def test_check_path_valid_should_return_error_when_file_path_is_empty(self):
        path = ''
        is_file = True
        with mock.patch('os.path.exists', return_value=True):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PARAM_ERROR)
        self.assertEqual(str(context.exception),
                         "The path is empty. Please enter a valid path.")

    def test_check_path_valid_should_return_error_when_is_file_and_path_is_not_a_file(self):
        path = '/path/host/test.so'
        is_file = True
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/host/test.so\" is not a file. "
                                                 "Please check the path.")

    def test_check_path_valid_should_return_warning_when_is_file_and_path_is_link(self):
        path = '/path/a.so'
        is_file = True
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=True), \
             mock.patch('os.path.getsize', return_value=0), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=True), \
             self.assertLogs(level='WARNING') as context:
                check_path_valid(path, is_file)
        self.assertIn("The path /path/a.so is link. Please check the path.", context.output[0])

    def test_check_path_valid_should_return_error_when_not_is_file_and_path_is_not_directory(self):
        path = '/path/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception), "The path \"/path/host\" is not a directory. Please check the path.")

    def test_check_path_valid_should_return_warning_when_is_not_file_and_path_is_link(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=True), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=True), \
             self.assertLogs(level='WARNING') as context:
                check_path_valid(path, is_file)
        self.assertIn("The path /path/host/host is link. Please check the path.", context.output[0])

    def test_check_path_valid_should_return_none_when_is_root_user(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=True):
            self.assertIsNone(check_path_valid(path, is_file))

    def test_check_path_valid_should_return_error_when_is_not_file_and_path_has_no_permission_to_read(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=False), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_path_valid(path, is_file)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/host\" does not have "
                         "permission to read. Please check that the path is readable.")

    def test_check_path_valid_should_return_warning_when_others_writable(self):
        path = '/path/host/host'
        is_file = False
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=False), \
             mock.patch('os.access', return_value=True), \
             mock.patch('os.stat', return_value=Mock(st_mode=0o002, st_uid=1000)), \
             self.assertLogs(level='WARNING') as context:
                check_path_valid(path, is_file)
        self.assertIn("The path /path/host/host is writable by any other users.", context.output[0])

    def test_check_path_valid_should_return_warning_when_is_not_root_and_not_current_user(self):
        path = '/path/host/host'
        is_file = False
        # win平台无校验必要
        if not is_linux():
            return
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isdir', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=False), \
             mock.patch('os.access', return_value=True), \
             mock.patch('os.stat', return_value=Mock(st_mode=0o000, st_uid=9999)), \
             self.assertLogs(level='WARNING') as context:
                check_path_valid(path, is_file)
        self.assertIn("The path /path/host/host owner is not root or current user.", context.output[0])

    def test_check_file_readable_should_not_return_error(self):
        path = '/path/host/host/test.so'
        with mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch("os.path.getsize", return_value=2 * 1024 * 1024):
            check_file_readable(path)

    def test_check_file_readable_should_return_error_when_file_is_too_large(self):
        path = '/path/host/host/test.so'
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch("os.path.getsize", return_value=Constant.MAX_READ_FILE_BYTES + 1):
            with self.assertRaises(ProfException) as context:
                check_file_readable(path)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/host/test.so\" is too large to read. Please check the path.")

    def test_check_file_writable_should_not_return_error(self):
        path = '/path/host/host/test.so'
        with mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch('os.access', return_value=True):
            check_file_writable(path)

    def test_check_file_writable_should_return_error_when_path_has_no_permission_to_write(self):
        path = '/path/host/host'
        with mock.patch('os.path.exists', return_value=True), \
             mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch('os.access', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_file_writable(path)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/host\" does not have "
                         "permission to write. Please check that the path is writeable.")

    def test_check_dir_readable_should_not_return_error(self):
        path = '/path/host/host'
        with mock.patch(NAMESPACE + '.check_path_valid'):
            check_dir_readable(path)

    def test_check_dir_writable_should_not_return_error(self):
        path = '/path/host/host'
        with mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch('os.access', return_value=True):
            check_dir_writable(path)

    def test_check_dir_writable_should_return_none_when_is_root_user(self):
        path = '/path/host/writable'
        with mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=True):
            self.assertIsNone(check_dir_writable(path))

    def test_check_dir_writable_should_return_error_when_path_has_no_permission_to_write(self):
        path = '/path/host/writable'
        with mock.patch(NAMESPACE + '.check_path_valid'), \
             mock.patch('os.path.exists', return_value=True), \
             mock.patch('os.access', return_value=False), \
             mock.patch(NAMESPACE + '.is_root_user', return_value=False):
            with self.assertRaises(ProfException) as context:
                check_dir_writable(path)
        self.assertEqual(context.exception.code, ProfException.PROF_INVALID_PATH_ERROR)
        self.assertEqual(str(context.exception),
                         "The path \"/path/host/writable\" does not have "
                         "permission to write. Please check that the path is writeable.")

    def test_check_so_valid_should_return_false_when_so_is_not_file(self):
        path = "/path/host/msprof_analysis.so"
        with mock.patch('os.path.isfile', return_value=False):
            self.assertFalse(check_so_valid(path))

    def test_check_so_valid_should_return_false_when_so_is_soft_link(self):
        path = "/path/host/msprof_analysis.so"
        with mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=True):
            self.assertFalse(check_so_valid(path))

    def test_check_so_valid_should_return_false_when_so_has_no_permission_to_execute(self):
        path = "/path/host/msprof_analysis.so"
        with mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=False):
            self.assertFalse(check_so_valid(path))

    def test_check_so_valid_should_return_true_when_others_has_permission_to_write(self):
        path = "/path/host/msprof_analysis.so"
        StatInfo = namedtuple("StatInfo", ["st_mode"])
        with mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=True), \
             mock.patch('os.stat', return_value=StatInfo(0o022)):
            self.assertTrue(check_so_valid(path))

    def test_check_so_valid_should_return_true_when_not_linux(self):
        path = "/path/host/msprof_analysis.so"
        StatInfo = namedtuple("StatInfo", ["st_mode", "st_uid", "st_gid"])
        with mock.patch('os.path.isfile', return_value=True), \
             mock.patch('os.path.islink', return_value=False), \
             mock.patch('os.access', return_value=True), \
             mock.patch('os.stat', return_value=StatInfo(0o001, 3, 4)), \
             mock.patch(NAMESPACE + '.is_linux', return_value=False):
            self.assertTrue(check_so_valid(path))

    def test_is_link_should_return_false_when_path_is_empty_str(self):
        path = ""
        self.assertFalse(is_link(path))

    def test_is_root_user_should_return_true_when_is_root(self):
        if is_linux():
            with mock.patch('os.getuid', return_value=0):
                self.assertTrue(is_root_user())
        else:
            self.assertTrue(is_root_user())

    def test_is_root_user_should_return_false_when_is_not_root(self):
        if is_linux():
            with mock.patch('os.getuid', return_value=1):
                self.assertFalse(is_root_user())
        else:
            self.assertTrue(is_root_user())
