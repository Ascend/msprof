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
from unittest import mock

from profiling_bean.stars.fusion_task_bean import FusionTaskBean

NAMESPACE = 'profiling_bean.stars.fusion_task_bean'


class TestFusionTaskBean(unittest.TestCase):

    def test_init(self):
        args = [2134, 0x6BD3, 100, 56, 1, 5, 0, 0, 0]
        with mock.patch(NAMESPACE + '.InfoConfReader') as mock_reader, \
                mock.patch(NAMESPACE + '.Utils.get_func_type', return_value='010110'):
            mock_reader.return_value.time_from_syscnt.return_value = 1120.0
            bean = FusionTaskBean(args)
            self.assertEqual(bean.func_type, '010110')
            self.assertEqual(bean.cnt, 1)
            self.assertEqual(bean.task_type, 2)
            self.assertEqual(bean.magic, 0x6BD3)
            self.assertEqual(bean.stream_id, 0)
            self.assertEqual(bean.task_id, 100)
            self.assertEqual(bean.sys_cnt, 1120.0)
            self.assertEqual(bean.fusion_task_type, '0000000000000001')
            self.assertEqual(bean.acc_id, 5)


if __name__ == '__main__':
    unittest.main()
