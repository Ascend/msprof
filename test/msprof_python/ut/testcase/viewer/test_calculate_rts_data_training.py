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

import pytest

from viewer.calculate_rts_data import check_aicore_events
from viewer.calculate_rts_data_training import get_task_type

NAMESPACE = 'viewer.calculate_rts_data_training'


class TestCalculateRtsTrain(unittest.TestCase):
    def test_check_aicore_events(self):
        events = ["0x8", "0xa", "0x9", "0xb", "0xc", "0xd", "0x54", "0x55", 0x1]
        check_aicore_events(events)

        with pytest.raises(SystemExit) as error:
            check_aicore_events([])
        self.assertEqual(error.value.args[0], 1)

    def test_get_task_type(self):
        res = get_task_type("1")
        self.assertEqual(res, 'kernel AI cpu task')

        res = get_task_type("")
        self.assertEqual(res, "")


if __name__ == '__main__':
    unittest.main()
