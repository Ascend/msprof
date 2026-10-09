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
import shutil
import unittest
from unittest import mock

from common_func.path_manager import PathManager

NAMESPACE = 'common_func.path_manager'
PROF_DIR = "PROF_1_TEST"
DEVICE_DIR = "device_0"
HOST_DIR = "host"



class TestPathManager(unittest.TestCase):

    def test_get_device_count_success_when_result_is_not_empty(self):
        with mock.patch('os.listdir', return_value=[DEVICE_DIR, 'device_1']):
            count = PathManager.get_device_count("PROF_1")
            self.assertEqual(count, 2)

    def test_del_summary_and_timeline_dir_success_when_result_dir_is_exist(self):
        shutil.rmtree(PROF_DIR, ignore_errors=True)
        os.mkdir(PROF_DIR, 0o777)
        os.mkdir(os.path.join(PROF_DIR, DEVICE_DIR), 0o777)
        os.mkdir(os.path.join(PROF_DIR, HOST_DIR), 0o777)
        os.mkdir(os.path.join(PROF_DIR, DEVICE_DIR, "summary"), 0o777)
        os.mkdir(os.path.join(PROF_DIR, HOST_DIR, "timeline"), 0o777)
        os.path.join(PROF_DIR, DEVICE_DIR)
        PathManager.del_summary_and_timeline_dir([os.path.join(PROF_DIR, DEVICE_DIR), os.path.join(PROF_DIR, HOST_DIR)])
        shutil.rmtree(PROF_DIR, ignore_errors=True)

    def test_walk_with_depth_success_when_directory_exists(self):
        shutil.rmtree(PROF_DIR, ignore_errors=True)
        os.mkdir(PROF_DIR, 0o777)
        os.mkdir(os.path.join(PROF_DIR, "level1"), 0o777)
        os.mkdir(os.path.join(PROF_DIR, "level1", "level2"), 0o777)
        os.mkdir(os.path.join(PROF_DIR, "level1", "level2", "level3"), 0o777)
        open(os.path.join(PROF_DIR, "root_file.txt"), 'w').close()
        open(os.path.join(PROF_DIR, "level1", "level1_file.txt"), 'w').close()
        open(os.path.join(PROF_DIR, "level1", "level2", "level2_file.txt"), 'w').close()
        open(os.path.join(PROF_DIR, "level1", "level2", "level3", "level3_file.txt"), 'w').close()
        results = list(PathManager.safe_os_walk(PROF_DIR, max_depth=3))
        self.assertEqual(len(results), 3)
        paths = [result[0] for result in results]
        expected_paths = [
            PROF_DIR,
            os.path.join(PROF_DIR, "level1"),
            os.path.join(PROF_DIR, "level1", "level2")
        ]
        for expected_path in expected_paths:
            self.assertIn(expected_path, paths)
        level3_path = os.path.join(PROF_DIR, "level1", "level2", "level3")
        self.assertNotIn(level3_path, paths)
        shutil.rmtree(PROF_DIR, ignore_errors=True)



