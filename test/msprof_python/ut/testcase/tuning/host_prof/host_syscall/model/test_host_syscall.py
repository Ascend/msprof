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

from host_prof.host_syscall.model.host_syscall import HostSyscall

NAMESPACE = 'host_prof.host_syscall.model.host_syscall'


class TsetHostSyscall(unittest.TestCase):
    result_dir = 'test\\test.text'

    def test_flush_data(self):
        with mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            check = HostSyscall(self.result_dir)
            check.cache_data = [['MSVP_UploaderD', 18070, '18072', 'nanosleep', '191449517.503',
                                 1057.0, '191449518.56', 191454818891.872, 191454819948.872]]
            check.flush_data()

    def test_has_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            check = HostSyscall(self.result_dir)
            result = check.has_runtime_api_data()
        self.assertEqual(result, True)

    def test_get_summary_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_summary_runtime_api_data()
        self.assertEqual(result, (1, 2, 3, 4))

    def test_get_runtime_api_data(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_runtime_api_data()
        self.assertEqual(result, (1, 2, 3, 4))

    def test_get_all_tid(self):
        with mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=(1, 2, 3, 4)):
            check = HostSyscall(self.result_dir)
            result = check.get_all_tid()
        self.assertEqual(result, (1, 2, 3, 4))


if __name__ == '__main__':
    unittest.main()
