# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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


if __name__ == '__main__':
    unittest.main()
