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

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from profiling_bean.stars.acsq_task_v6_bean import AcsqTaskV6


class TestAcsqTaskV6(unittest.TestCase):

    def test_instance_of_v6_task_bean(self):
        args = [1024, 27603, 3, 0, 2201803260082, 11, 65, 2201803, 22234553, 4332]
        acsq_task_v6 = AcsqTaskV6(args)
        ret = ['000000', 1, 3, 3, 1, 427187950540182.3]
        with mock.patch.object(ChipManager(), 'is_chip_v6_2_0', return_value=True), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt',
                           return_value=427187950540182.3):
            bean_property = [
                acsq_task_v6.func_type,
                acsq_task_v6.task_type,
                acsq_task_v6.stream_id,
                acsq_task_v6.task_id,
                acsq_task_v6.acc_id,
                acsq_task_v6.write_latency
            ]
        self.assertEqual(ret, bean_property)

    def test_write_latency_ignores_reserved_fields_on_old_chips_and_nonpositive_intervals(self):
        for v62, end in ((False, 250), (True, 0), (True, 199), (True, 200)):
            with self.subTest(v62=v62, end=end):
                task = AcsqTaskV6([1024, 0, 3, 0, 200, 0, 0, 0, end, 0])
                with mock.patch.object(ChipManager(), 'is_chip_v6_2_0', return_value=v62), \
                        mock.patch.object(InfoConfReader(), 'duration_from_syscnt') as convert:
                    self.assertEqual(0.0, task.write_latency)
                    convert.assert_not_called()
