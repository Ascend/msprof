#!/usr/bin/env python
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
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import unittest

from constant.constant import clear_dt_project
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.iter_rec.iter_rec_parser'


class TestIterRecParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_IterRecParser')
    sample_config = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': 1,
        'job_id': 'job_default', 'model_id': -1
    }
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0']}
    dynamic_data = {1: ['1-1-0', '1-2-0', '1-3-0'], 2: ['2-1-0', '2-2-0', '2-3-0']}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)
