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

from profiling_bean.stars.acsq_task_v6_bean import AcsqTaskV6


class TestAcsqTaskV6(unittest.TestCase):

    def test_instance_of_v6_task_bean(self):
        args = [1024, 27603, 3, 0, 2201803260082, 11, 65]
        acsq_task_v6 = AcsqTaskV6(args)
        ret = ['000000', 1, 3, 3, 1]
        bean_property = [
            acsq_task_v6.func_type,
            acsq_task_v6.task_type,
            acsq_task_v6.stream_id,
            acsq_task_v6.task_id,
            acsq_task_v6.acc_id
        ]
        self.assertEqual(ret, bean_property)
