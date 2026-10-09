#!/usr/bin/python3
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

from msmodel.npu_mem.npu_mem_model import NpuMemModel

NAMESPACE = 'msmodel.npu_mem.npu_mem_model'


class TestNpuMemModel(unittest.TestCase):

    def test_flush(self):
        with mock.patch(NAMESPACE + ".NpuMemModel.check_db"), \
                mock.patch(NAMESPACE + ".NpuMemModel.check_table"), \
                mock.patch(NAMESPACE + ".NpuMemModel.create_table"), \
                mock.patch(NAMESPACE + ".NpuMemModel.insert_data_to_db"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.flush([])

    def test_get_timeline_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.get_timeline_data()

    def test_get_summary_data(self):
        with mock.patch(NAMESPACE + ".DBManager.fetch_all_data"):
            check = NpuMemModel("test", "npu_mem.db", ["NpuMem"])
            check.get_summary_data()
