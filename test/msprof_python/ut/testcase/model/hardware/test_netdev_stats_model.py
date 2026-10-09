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
from msmodel.hardware.netdev_stats_model import NetDevStatsModel

NAMESPACE = "msmodel.hardware.netdev_stats_model"


class TestNetDevStatsModel(unittest.TestCase):

    def test_flush(self):
        data = [0, 980783386442350, 0, 0, 17794086897794, 17794908949726, 0, 0,
                273574917, 273636579, 0, 0, 0, 0, 1, 204255952, 973902252]
        with mock.patch(NAMESPACE + ".NetDevStatsModel.insert_data_to_db"):
            InfoConfReader()._info_json = {"devices": "0"}
            parser = NetDevStatsModel("test")
            parser.flush(data)
