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

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.sio_viewer import SioViewer


class TestSioViewer(unittest.TestCase):
    def test_get_trace_timeline(self):
        datas = [
            (0, 1, 2, 3, 4, 5, 6, 7, 8, 214),
            (1, 1, 2, 3, 4, 5, 6, 7, 8, 214),
            (0, 1, 2, 3, 4, 5, 6, 7, 8, 216),
            (1, 1, 2, 3, 4, 5, 6, 7, 8, 216)
        ]
        start_info = {"collectionTimeBegin": 0, "clockMonotonicRaw": 10}
        end_info = {"collectionTimeEnd": 10}
        InfoConfReader()._info_json = {"pid": '0'}
        InfoConfReader()._start_info = start_info
        InfoConfReader()._end_info = end_info
        check = SioViewer({}, {})
        ret = check.get_trace_timeline(datas)
        self.assertEqual(9, len(ret))
