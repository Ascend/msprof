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

from profiling_bean.struct_info.l2_cache import L2CacheDataBean

NAMESPACE = 'profiling_bean.struct_info.l2_cache'


class TestL2CacheDataBean(unittest.TestCase):
    data = (0, 3, 26, 0, 991, 98, 0, 0, 505, 1089, 0, 106)

    def test_decode(self):
        data_bin = b'\x00'
        with mock.patch('struct.unpack', return_value=self.data):
            with mock.patch(NAMESPACE + '.L2CacheDataBean.construct_bean', return_value=False):
                check = L2CacheDataBean()
                check.decode(data_bin)
            with mock.patch(NAMESPACE + '.L2CacheDataBean.construct_bean', return_value=True):
                check = L2CacheDataBean()
                check.decode(data_bin)

    def test_construct_bean(self):
        check = L2CacheDataBean()
        self.assertEqual(check.construct_bean(self.data), True)
        self.assertEqual(check.construct_bean((1,)), False)

    def test_property(self):
        check = L2CacheDataBean()
        check.construct_bean(self.data)
        self.assertEqual(check.task_type, 0)
        self.assertEqual(check.stream_id, 3)
        self.assertEqual(check.task_id, 26)
        self.assertEqual(check.events_list, ['991', '98', '0', '0', '505', '1089', '0', '106'])
