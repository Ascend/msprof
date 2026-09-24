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
import json
import unittest
from unittest import mock

from constant.constant import CONFIG
from viewer.stars.stars_chip_trans_view import StarsChipTransView
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import InfoJson

NAMESPACE = 'viewer.stars.stars_chip_trans_view'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsChipTransView(unittest.TestCase):

    def test_get_timeline_data(self):
        InfoJson(pid=123).apply()
        with mock.patch(NAMESPACE + '.ViewModel.init', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[[0, 1, 2, 3]]):
            res = StarsChipTransView(CONFIG, CONFIG).get_timeline_data()
        self.assertEqual(len(res), 7)

        ori_chip_id = ChipManager().get_chip_id()
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        with mock.patch(NAMESPACE + '.ViewModel.init', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=True), \
             mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[[0, 1, 2, 3]]):
            res = StarsChipTransView(CONFIG, CONFIG).get_timeline_data()
        self.assertEqual(len(res), 4)
        ChipManager().chip_id = ori_chip_id

