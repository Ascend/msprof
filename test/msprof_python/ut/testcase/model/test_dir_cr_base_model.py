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
import os
import unittest

from common_func.info_conf_reader import InfoConfReader
from constant.constant import clear_dt_project


class TestDirCRBaseModel(unittest.TestCase):
    """
    this class provide a standard sqlite dir create and delete process
    """
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_BaseModel")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    PROF_HOST_DIR = os.path.join(PROF_DIR, 'host')

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        path = os.path.join(self.PROF_DEVICE_DIR, 'sqlite')
        os.makedirs(path)
        path = os.path.join(self.PROF_HOST_DIR, 'sqlite')
        os.makedirs(path)
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5"}

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)
        info_reader = InfoConfReader()
        info_reader._info_json = {}
