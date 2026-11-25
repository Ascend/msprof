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
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelV62Dto
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.ccu.ccu_channel_viewer import CCUChannelViewer

NAMESPACE = 'msmodel.hardware.ccu_channel_model'


class TestCCUChannelViewer(unittest.TestCase):
    def setUp(self):
        reader = InfoConfReader()
        reader._dev_cnt = 0
        reader._host_mon = 0
        reader._local_time_offset = 0

    def test_format_channel_summary_data_should_return_formatted_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        datas = [
            OriginChannelDto(1, 4919982785935, 25011, 25011, 25011),
            OriginChannelDto(2, 4919982785938, 13185, 13185, 13185),
            OriginChannelDto(3, 4919982785938, 0, 0, 0),
        ]
        with mock.patch(NAMESPACE + '.CCUViewerChannelModel.check_table', return_value=True):
            check = CCUChannelViewer({}, {})
            ret = check.format_channel_summary_data(datas)
            self.assertEqual(3, len(ret))
            self.assertEqual(1, ret[0][0])
            self.assertEqual(0, ret[2][2])

    def test_format_channel_v62_summary_data_should_return_formatted_data(self):
        chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        try:
            InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
            datas = [OriginChannelV62Dto(1, "100", 25011, 12345, 1), OriginChannelV62Dto(2, "200", 0, 0, 0)]
            check = CCUChannelViewer({}, {})
            ret = check.format_channel_v62_summary_data(datas)
            self.assertEqual([(1, 1, "4.000\t", 12345, 25011), (2, 0, "8.000\t", 0, 0)], ret)
        finally:
            ChipManager().chip_id = chip_id
