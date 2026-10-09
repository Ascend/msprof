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

from msparser.stars import StarsChipTransParser
from profiling_bean.stars.stars_chip_trans_bean import StarsChipTransBean
from profiling_bean.stars.stars_chip_trans_v6_bean import StarsChipTransV6Bean
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'msparser.stars.stars_chip_trans_bean'
sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}


class TestStarsChipTransParser(unittest.TestCase):

    def test_decoder(self):
        check = StarsChipTransParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = StarsChipTransBean(struct.pack("2HLQ2H3L2Q4L", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        res = check.decoder
        self.assertEqual(res.sys_cnt, 0)

    def test_parser_chip_v6_should_return_correct_bean_when_parse_success(self):
        ori_info_json = InfoConfReader().get_info_json()
        InfoConfReader()._info_json = {'devices': '0', "DeviceInfo": [{'hwts_frequency': 1000}]}
        ori_chip_id = ChipManager().get_chip_id()
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        check = StarsChipTransParser(sample_config.get('result_dir'), 'test.db', [])
        check._decoder = StarsChipTransV6Bean(struct.pack("2HI3Q8L", 1, 1, 4, 5, 1, 4, 1, 9, 1, 9, 8, 1, 0, 0))
        check._data_list = [check.decoder]
        check.preprocess_data()
        self.assertEqual(check._data_dict, {'000001': [[0, 0.0, 4, 0]]})
        ChipManager().chip_id = ori_chip_id
        InfoConfReader()._info_json = ori_info_json
