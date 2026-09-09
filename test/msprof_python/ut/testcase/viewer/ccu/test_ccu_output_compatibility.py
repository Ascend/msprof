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

import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginGroupInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginWaitSignalInfoDto
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginMissionDto
from profiling_bean.prof_enum.data_tag import DataTag
from viewer.ccu.ccu_mission_viewer import CCUMissionViewer


class TestCcuOutputCompatibility(unittest.TestCase):
    def test_v6_timeline_output_should_match_legacy_contract(self):
        viewer = CCUMissionViewer.__new__(CCUMissionViewer)
        viewer.pid = 101
        viewer.tid = 202
        ccu_data = {
            DataTag.CCU_MISSION: [
                OriginMissionDto(
                    stream_id=1, task_id=10, lp_instr_id=100, start_time=1000, end_time=2000,
                    time_type="LoopGroup"
                ),
                OriginMissionDto(
                    stream_id=1, task_id=11, setckebit_instr_id=200, rel_id=9,
                    start_time=3000, end_time=4000, time_type="Wait"
                ),
            ],
            DataTag.CCU_GROUP: [
                OriginGroupInfoDto(
                    stream_id=1, task_id=10, instr_id=100, die_id=0, data_size=1048576,
                    reduce_op_type="SUM", input_data_type="FP16", output_data_type="FP16"
                )
            ],
            DataTag.CCU_WAIT_SIGNAL: [
                OriginWaitSignalInfoDto(
                    stream_id=1, task_id=11, instr_id=200, die_id=0, mask=255, channel_id=3
                ),
                OriginWaitSignalInfoDto(
                    stream_id=1, task_id=11, instr_id=200, die_id=0, mask=255, channel_id=4
                ),
            ],
            DataTag.CCU_CHANNEL: [
                OriginChannelDto(channel_id=3, timestamp=3500, avg_bw=88),
                OriginChannelDto(channel_id=4, timestamp=3900, avg_bw=120),
                OriginChannelDto(channel_id=4, timestamp=4000, avg_bw=999),
            ],
        }
        expected = (
            '[{"name":"process_name","pid":101,"tid":202,"args":{"name":"CCU"},"ph":"M"},'
            '{"name":"thread_name","pid":101,"tid":202,"args":{"name":"Communication"},"ph":"M"},'
            '{"name":"LoopGroup","pid":101,"tid":202,"ts":1000,"dur":1000,'
            '"args":{"Physic Stream Id":1,"Task Id":10,"Instruction ID":100,"Die Id":0,'
            '"Data Size":1048576,"Bandwidth (MB/s)":1000.0,"Reduce Op Type":"SUM",'
            '"Input Data Type":"FP16","Output Data Type":"FP16"},"ph":"X"},'
            '{"name":"Wait","pid":101,"tid":202,"ts":3000,"dur":1000,'
            '"args":{"Physic Stream Id":1,"Task Id":11,"Notify Instruction ID":200,"Notify Rank ID":9,'
            '"Die Id":0,"Mask":255,"Maximum Delay Channel":4,"Maximum Channel Delay":120},"ph":"X"}]'
        )

        with mock.patch.object(
                InfoConfReader(), "trans_syscnt_into_local_time", side_effect=lambda value: value), \
                mock.patch.object(InfoConfReader(), "duration_from_syscnt", side_effect=lambda value: value):
            output = viewer.get_trace_timeline(ccu_data)

        self.assertEqual(expected, json.dumps(output, separators=(",", ":")))
