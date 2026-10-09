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

from profiling_bean.stars.acsq_task import AcsqTask

NAMESPACE = 'profiling_bean.stars.acsq_task'


class TestAcsqTask(unittest.TestCase):

    def test_init(self):
        args = [1000, 2, 3, 4, 66, 7, 8, 9, 54]
        with mock.patch(NAMESPACE + '.Utils.get_func_type', return_value=1):
            check = AcsqTask(args)
            self.assertEqual(check.func_type, 1)


if __name__ == '__main__':
    unittest.main()
