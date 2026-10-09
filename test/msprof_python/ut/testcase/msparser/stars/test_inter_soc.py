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
import struct
import unittest

from common_func.info_conf_reader import InfoConfReader
from msparser.stars.inter_soc_parser import InterSocParser
from profiling_bean.stars.inter_soc import InterSoc

NAMESPACE = 'msparser.stars.inter_soc_parser'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestInterSocParser(unittest.TestCase):

    def test_decoder(self):
        check = InterSocParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = InterSoc(struct.pack("4HQ2H3L4Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        res = check.decoder
        self.assertEqual(res.sys_cnt, 0)

    def test_preprocess_data(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        check = InterSocParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = InterSoc(struct.pack("4HQ2H3L4Q", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        res = check.decoder
        check._data_list.append(res)
        check.preprocess_data()
        self.assertEqual(check._data_list[0][0], 0)
