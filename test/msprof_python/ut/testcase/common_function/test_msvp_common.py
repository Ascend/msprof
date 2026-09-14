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
import tempfile
import unittest

from common_func.constant import Constant
from common_func.msvp_common import clear_project_dirs
from common_func.msvp_common import format_high_precision_for_csv


class TestMsvpCommon(unittest.TestCase):
    def test_format_high_precision_for_csv_and_should_add_tab(self):
        data = '12345678912345.2345324'
        res = format_high_precision_for_csv(data)
        self.assertEqual(res, '12345678912345.2345324\t')


    def test_clear_project_dirs_should_preserve_host_platform_thread_db(self):
        with tempfile.TemporaryDirectory() as project_dir:
            sqlite_dir = os.path.join(project_dir, "sqlite")
            data_dir = os.path.join(project_dir, "data")
            os.makedirs(sqlite_dir)
            os.makedirs(data_dir)
            thread_db = os.path.join(sqlite_dir, "thread.db")
            obsolete_db = os.path.join(sqlite_dir, "op_summary.db")
            complete_file = os.path.join(data_dir, "host" + Constant.COMPLETE_TAG)
            data_file = os.path.join(data_dir, "host_platform_core.bin")
            for file_path in (thread_db, obsolete_db, complete_file, data_file):
                open(file_path, "w").close()

            clear_project_dirs(project_dir)

            self.assertTrue(os.path.exists(thread_db))
            self.assertFalse(os.path.exists(obsolete_db))
            self.assertFalse(os.path.exists(complete_file))
            self.assertTrue(os.path.exists(data_file))


if __name__ == '__main__':
    unittest.main()
