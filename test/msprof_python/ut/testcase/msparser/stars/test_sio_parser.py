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

from common_func.platform.chip_manager import ChipManager
from msparser.stars.sio_parser import SioParser
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'msparser.stars.sio_parser'
sample_config = {
    "model_id": 1,
    'iter_id': 'dasfsd',
    'result_dir': 'jasdfjfjs',
    "ai_core_profiling_mode": "task-based",
    "aiv_profiling_mode": "sample-based"
}


class TestSioParser(unittest.TestCase):
    def test_decoder(self):
        check = SioParser(sample_config.get('result_dir'), 'test.db', [])
        sio_decoder = check.decoder.decode(
            struct.pack("HHLQHH11L", 25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
        self.assertEqual(sio_decoder._func_type, '011001')
        check.preprocess_data()
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        check_v6 = SioParser(sample_config.get('result_dir'), 'test.db', [])
        sio_decoder_v6 = check_v6.decoder.decode(
            struct.pack("HHLQHH11L", 25, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
        self.assertEqual(sio_decoder_v6._func_type, '011001')


