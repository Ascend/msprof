# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

# coding=utf-8
"""
function: test soc_pmu_viewer
Copyright Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
"""
import unittest
from unittest import mock

from viewer.soc_pmu_viewer import SocPmuViewer

MODEL_NAMESPACE = 'msmodel.l2_cache.soc_pmu_model'
NAMESPACE = 'viewer.soc_pmu_viewer'


class TestSocPmuViewer(unittest.TestCase):
    def test_get_summary_data_when_op_dict_is_empty(self):
        configs = {}
        param = {'data_type': 'soc_pmu', 'export_type': 'summary'}
        datas = [(1, 1, 0.2, 0.8), (1, 2, 0.2, 0.8), (1, 4, 0.75, 0.25)]
        with mock.patch(MODEL_NAMESPACE + '.SocPmuViewerModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.DataManager.get_op_dict', return_value={}), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuViewerModel.get_summary_data', return_value=datas):
            check = SocPmuViewer(configs, param)
            ret = check.get_summary_data()
            self.assertEqual(ret[1], datas)

    def test_get_summary_data_when_op_dict_has_op_info(self):
        configs = {"headers": ['Stream Id', 'Task Id', 'TLB Miss Rate', 'TLB Hit Rate']}
        param = {'data_type': 'soc_pmu', 'export_type': 'summary'}
        datas = [(1, 1, 0.2, 0.8), (1, 2, 0.2, 0.8), (1, 4, 0.75, 0.25)]
        op_info = {
            "1-1": "name1",
            "4-1": "name2"
        }
        expect = [[1, 1, 0.2, 0.8, "name1"], [1, 2, 0.2, 0.8, "N/A"], [1, 4, 0.75, 0.25, "name2"]]
        with mock.patch(MODEL_NAMESPACE + '.SocPmuViewerModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.DataManager.get_op_dict', return_value=op_info), \
                mock.patch(MODEL_NAMESPACE + '.SocPmuViewerModel.get_summary_data', return_value=datas):
            check = SocPmuViewer(configs, param)
            ret = check.get_summary_data()
            self.assertEqual(ret[1], expect)
