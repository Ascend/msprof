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
import unittest

from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel


class TestChipManager(unittest.TestCase):

    def test_get_max_core_id_chip_v6_2_0(self):
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        self.assertEqual(ChipManager().get_max_core_id(), 69)

    def test_get_max_core_id_chip_v6_1_0(self):
        ChipManager().chip_id = ChipModel.CHIP_V6_1_0
        self.assertEqual(ChipManager().get_max_core_id(), 35)

    def test_is_mix_context_id_zero_chip_should_only_accept_v4_and_v1_1(self):
        # mix 算子只有 chip 5/7/8/11（V4_1_0 与 V1_1_x）把 context_id 记为 0
        zero_chips = (ChipModel.CHIP_V4_1_0, ChipModel.CHIP_V1_1_1, ChipModel.CHIP_V1_1_2, ChipModel.CHIP_V1_1_3)
        other_chips = (ChipModel.CHIP_V5_1_0, ChipModel.CHIP_V6_1_0, ChipModel.CHIP_V6_1_1, ChipModel.CHIP_V6_2_0)
        for chip_id in zero_chips:
            ChipManager().chip_id = chip_id
            self.assertTrue(ChipManager().is_mix_context_id_zero_chip(), f"chip {chip_id} should use context id 0")
        for chip_id in other_chips:
            ChipManager().chip_id = chip_id
            self.assertFalse(ChipManager().is_mix_context_id_zero_chip(),
                             f"chip {chip_id} should keep DEFAULT_CONTEXT_ID")


if __name__ == '__main__':
    unittest.main()
