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
from common_func.memcpy_constant import MemoryCopyConstant
from common_func.ms_constant.str_constant import StrConstant
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer

NAMESPACE = 'msmodel.memory_copy.memcpy_model.MemcpyModel'


class TestMemoryCopyViewer(unittest.TestCase):
    def setUp(self) -> None:
        self.current_path = ""
        InfoConfReader()._info_json = {"pid": 0}
        self.memcopy_viewer = MemoryCopyViewer(self.current_path)

    def test_get_memory_copy_chip0_summary_1(self):
        expect_res = [(0, 20000, 1, 20000, 20000, 20000, 100, 20000, MemoryCopyConstant.DEFAULT_VIEWER_VALUE,
                       MemoryCopyConstant.TYPE, StrConstant.AYNC_MEMCPY, 1,
                       MemoryCopyConstant.DEFAULT_VIEWER_VALUE, 1)]

        export_data = [(20000, MemoryCopyConstant.TYPE, 1, 1, 100, 20000, 20000, 20000, 1)]
        with mock.patch(NAMESPACE + '.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.return_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_chip0_summary_2(self):
        expect_res = []

        export_data = []
        with mock.patch(NAMESPACE + '.check_db', return_value=False), \
                mock.patch(NAMESPACE + '.return_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_non_chip0_summary_1(self):
        expect_res = [("MemcopyAsync", "other", 11, 12, 100, '12.200\t', '12.300\t')]

        export_data = [(MemoryCopyConstant.ASYNC_MEMCPY_NAME, MemoryCopyConstant.TYPE, 11,
                        12, 100, 2.2, 2.3)]
        InfoConfReader()._local_time_offset = 10.0
        with mock.patch(NAMESPACE + '.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.return_not_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_non_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_memory_copy_non_chip0_summary_2(self):
        expect_res = []
        export_data = []
        with mock.patch(NAMESPACE + '.check_db', return_value=False), \
                mock.patch(NAMESPACE + '.return_not_chip0_summary', return_value=export_data):
            res = self.memcopy_viewer.get_memory_copy_non_chip0_summary()

        self.assertEqual(expect_res, res)

    def test_get_direction_should_return_value(self):
        self.assertEqual(MemoryCopyConstant.get_direction(MemoryCopyConstant.H2D_TAG), MemoryCopyConstant.H2D_NAME)
