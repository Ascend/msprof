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
from constant.constant import CONFIG
from msparser.hardware.netdev_stats_parser import NetDevStatsParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = "msparser.hardware.netdev_stats_parser"


class TestNetDevStatsParser(unittest.TestCase):
    file_list = {DataTag.NETDEV_STATS: ["netdev_stats.data.0.slice_0"]}

    def test_parse(self):
        with mock.patch(NAMESPACE + ".NetDevStatsParser.parse_plaintext_data"):
            InfoConfReader()._info_json = {"devices": "0"}
            parser = NetDevStatsParser(self.file_list, CONFIG)
            parser.parse()

    def test_save(self):
        with mock.patch("msmodel.hardware.netdev_stats_model.NetDevStatsModel.init"), \
                mock.patch("msmodel.hardware.netdev_stats_model.NetDevStatsModel.create_table"), \
                mock.patch("msmodel.hardware.netdev_stats_model.NetDevStatsModel.flush"), \
                mock.patch("msmodel.hardware.netdev_stats_model.NetDevStatsModel.finalize"):
            InfoConfReader()._info_json = {"devices": "0"}
            parser = NetDevStatsParser(self.file_list, CONFIG)
            parser._origin_data = [123]
            parser.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".NetDevStatsParser.parse"), \
                mock.patch(NAMESPACE + ".NetDevStatsParser.save", side_effect=OSError), \
                mock.patch(NAMESPACE + ".logging.error"):
            InfoConfReader()._info_json = {"devices": "0"}
            NetDevStatsParser(self.file_list, CONFIG).ms_run()
