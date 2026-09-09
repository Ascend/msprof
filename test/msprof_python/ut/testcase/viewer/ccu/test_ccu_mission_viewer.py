# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

import os
import unittest
import shutil
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginMissionDto
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginTaskInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginGroupInfoDto
from profiling_bean.db_dto.ccu.ccu_add_info_dto import OriginWaitSignalInfoDto
from profiling_bean.db_dto.step_trace_dto import IterationRange
from viewer.ccu.ccu_mission_viewer import CCUMissionViewer
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.prof_enum.chip_model import ChipModel
from msparser.add_info.ccu_add_info_parser import CCUAddInfoParser
from msparser.hardware.ccu_mission_parser import CCUMissionParser
from msparser.hardware.ccu_channel_parser import CCUChannelParser

NAMESPACE = 'msmodel.hardware.ccu_mission_model'


class TestCCUMissionViewer(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), "ccu_add_info")
    HOST_PATH = os.path.join(DIR_PATH, "host")
    DEVICE_PATH = os.path.join(DIR_PATH, "device")
    HOST_SQLITE_PATH = os.path.join(HOST_PATH, "sqlite")
    DEVICE_SQLITE_PATH = os.path.join(DEVICE_PATH, "sqlite")
    file_list = {}
    HOST_CONFIG = {
        'result_dir': HOST_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }
    DEVICE_CONFIG = {
        'result_dir': DEVICE_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1, 'chip_model': str(ChipModel.CHIP_V6_1_0.value)
    }

    def setUp(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)
        os.makedirs(self.HOST_SQLITE_PATH)
        os.makedirs(self.DEVICE_SQLITE_PATH)
        if os.path.exists(self.HOST_SQLITE_PATH):
            add_info_parser = CCUAddInfoParser(self.file_list, self.HOST_CONFIG)
            add_info_parser._ccu_add_info_data = {
                DataTag.CCU_TASK: [[0, 1, '9581505218827963271', '7415574198778220483', 0, 2, 1, 0, 1, 1, 0]],
                DataTag.CCU_WAIT_SIGNAL:
                    [[0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 39, 1, 210, 2, 2, 1],
                     [0, '6823696491211891432', '7415574198778220483', 0, 2, 1, 1, 0, 1, 40, 1, 211, 2, 2, 1]],
                DataTag.CCU_GROUP:
                    [[0, '1362511707695072317', '7415574198778220483', 0, 2, 1, 1, 0, 1, 119, 255,
                      'SUM', 'INT32', 'INT32', 8192, 2, 1],
                     [0, '9129927687939955350', '7415574198778220483', 0, 2, 1, 1, 0, 1, 197, 255,
                      'RESERVED', 'RESERVED', 'RESERVED', 8192, 2, 1]]
            }
            add_info_parser.save()
        if os.path.exists(self.DEVICE_SQLITE_PATH):
            check = CCUMissionParser(self.file_list, self.DEVICE_CONFIG)
            check.mission_data = [
                [1, 0, 0, 0, 0, 74, 133158368126, 15, 133158368126],
                [1, 0, 0, 0, 0, 74, 133158368126, 14, 133158368126]
            ]
            check.save()

            check = CCUChannelParser(self.file_list, self.DEVICE_CONFIG)
            check.channel_data = [
                [0, 4919982785935, 25011, 25011, 25011],
                [1, 4919982785935, 4181298, 1435, 1068902],
                [2, 4919982785936, 13681, 13681, 13681]
            ]
            check.save()

    @classmethod
    def tearDownClass(cls):
        if os.path.exists(cls.DIR_PATH):
            shutil.rmtree(cls.DIR_PATH)

    def test_ccu_mission_viewer_should_return_true_when_run_get_timeline_data_success(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        configs = {'handler': '_get_ccu_mission_data',
                   'headers': ['Stream ID', 'Task Id', 'Instruction ID', 'Instruction Start Time(us)',
                               'Instruction Duration(us)', 'Notify Instruction ID', 'Notify Rank ID',
                               'Notify Duration(us)'], 'db': 'ccu.db', 'table': 'OriginMission', 'unused_cols': []}
        check = CCUMissionViewer(configs, params)
        check.get_timeline_data()

    def test_format_mission_summary_data_should_return_formatted_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        datas = [
            OriginMissionDto(stream_id=0, task_id=0, lp_instr_id=0, lp_start_time=0, lp_end_time=0,
                             setckebit_instr_id=74, setckebit_start_time=0, rel_id=15, rel_end_time=0),
            OriginMissionDto(stream_id=0, task_id=0, lp_instr_id=0, lp_start_time=0, lp_end_time=0,
                             setckebit_instr_id=74, setckebit_start_time=0, rel_id=14, rel_end_time=0)
        ]
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        with mock.patch(NAMESPACE + '.CCUViewerMissionModel.check_table', return_value=True):
            check = CCUMissionViewer({}, params)
            ret = check.format_mission_summary_data(datas)
            self.assertEqual(2, len(ret))
            self.assertEqual(74, ret[0][5])

    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        ccu_data_dict = {
            DataTag.CCU_MISSION: [
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=34, rel_id=15,
                                 start_time=1324138574522, end_time=1324138574522, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=34, rel_id=14,
                                 start_time=1324138574522, end_time=1324138574522, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=40, rel_id=1,
                                 start_time=1324138574641, end_time=1324138574641, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=0, setckebit_instr_id=40, rel_id=0,
                                 start_time=1324138574641, end_time=1324138574641, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=119, setckebit_instr_id=0, rel_id=0,
                                 start_time=1324138575375, end_time=1324138595272, time_type='LoopGroup',
                                 lp_start_time=None, lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=0, rel_id=0,
                                 start_time=1324138595993, end_time=1324138600670, time_type='LoopGroup',
                                 lp_start_time=None, lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=2,
                                 start_time=1324138600757, end_time=1324138600757, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=1,
                                 start_time=1324138600757, end_time=1324138601884, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None),
                OriginMissionDto(stream_id=1, task_id=0, lp_instr_id=197, setckebit_instr_id=205, rel_id=0,
                                 start_time=1324138600757, end_time=1324138600757, time_type='Wait', lp_start_time=None,
                                 lp_end_time=None, setckebit_start_time=None, rel_end_time=None)],
            DataTag.CCU_CHANNEL: [
                OriginChannelDto(channel_id=0, timestamp=1324138605307, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=1, timestamp=1324138605307, max_bw=14299535, min_bw=14299535,
                                 avg_bw=14299535),
                OriginChannelDto(channel_id=2, timestamp=1324138605308, max_bw=119258, min_bw=202,
                                 avg_bw=25831),
                OriginChannelDto(channel_id=3, timestamp=1324138605308, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=4, timestamp=1324138605309, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=5, timestamp=1324138605310, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=6, timestamp=1324138605310, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=124, timestamp=1324138605548, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=125, timestamp=1324138605549, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=126, timestamp=1324138605550, max_bw=0, min_bw=0, avg_bw=0),
                OriginChannelDto(channel_id=127, timestamp=1324138605550, max_bw=0, min_bw=0, avg_bw=0)],
            DataTag.CCU_TASK: [OriginTaskInfoDto(version=0, work_flow_mode=1, item_id='9581505218827963271',
                                                 group_name='7415574198778220483', rank_id=0, rank_size=2,
                                                 stream_id=1, task_id=0, die_id=1, mission_id=1, instr_id=0)],
            DataTag.CCU_WAIT_SIGNAL: [OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=38, mission_id=1,
                                                              cke_id=209, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=39, mission_id=1,
                                                              cke_id=210, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=40, mission_id=1,
                                                              cke_id=211, mask=2, channel_id=2,
                                                              remote_rank_id=1),
                                      OriginWaitSignalInfoDto(version=0, item_id='6823696491211891432',
                                                              group_name='7415574198778220483', rank_id=0,
                                                              work_flow_mode=1, rank_size=2, stream_id=1,
                                                              task_id=0, die_id=1, instr_id=205, mission_id=1,
                                                              cke_id=208, mask=2, channel_id=2,
                                                              remote_rank_id=1)],
            DataTag.CCU_GROUP: [OriginGroupInfoDto(version=0, item_id='1362511707695072317',
                                                   group_name='7415574198778220483', rank_id=0, work_flow_mode=1,
                                                   rank_size=2, stream_id=1, task_id=0, die_id=1, instr_id=119,
                                                   mission_id=255, reduce_op_type='SUM', input_data_type='INT32',
                                                   output_data_type='INT32', data_size=8192, channel_id=2,
                                                   remote_rank_id=1),
                                OriginGroupInfoDto(version=0, item_id='9129927687939955350',
                                                   group_name='7415574198778220483', rank_id=0, work_flow_mode=1,
                                                   rank_size=2, stream_id=1, task_id=0, die_id=1, instr_id=197,
                                                   mission_id=255, reduce_op_type='RESERVED',
                                                   input_data_type='RESERVED', output_data_type='RESERVED',
                                                   data_size=8192, channel_id=2, remote_rank_id=1)]}
        params = {'data_type': 'ccu_mission',
                  'project': 'prof_114514',
                  'device_id': 0,
                  'export_type': 'timeline',
                  'iter_id': 1,
                  'export_format': None,
                  'model_id': 4294967295,
                  'export_dump_folder': 'timeline'}
        with mock.patch(NAMESPACE + '.CCUViewerMissionModel.check_table', return_value=True):
            check = CCUMissionViewer({}, params)
            ret = check.get_trace_timeline(ccu_data_dict)
            self.assertEqual(7, len(ret))

    def test_host_add_info_association_should_include_stream_id(self):
        viewer = CCUMissionViewer.__new__(CCUMissionViewer)
        viewer.pid = 1
        viewer.tid = 1
        loop_data = [
            OriginMissionDto(stream_id=1, task_id=10, lp_instr_id=100, start_time=1000, end_time=4000,
                             time_type='LoopGroup'),
            OriginMissionDto(stream_id=2, task_id=10, lp_instr_id=100, start_time=1000, end_time=4000,
                             time_type='LoopGroup')
        ]
        group_data = [
            OriginGroupInfoDto(stream_id=1, task_id=10, instr_id=100, die_id=0, data_size=1024,
                               reduce_op_type='SUM', input_data_type='FP16', output_data_type='FP16')
        ]
        wait_data = [
            OriginMissionDto(stream_id=1, task_id=10, setckebit_instr_id=200, rel_id=5, start_time=2000,
                             end_time=6000, time_type='Wait'),
            OriginMissionDto(stream_id=2, task_id=10, setckebit_instr_id=200, rel_id=6, start_time=2000,
                             end_time=6000, time_type='Wait')
        ]
        wait_signal_data = [
            OriginWaitSignalInfoDto(stream_id=1, task_id=10, instr_id=200, die_id=0, mask=255, channel_id=3)
        ]

        with mock.patch.object(InfoConfReader(), 'trans_syscnt_into_local_time', return_value=1), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=3):
            loop_result = viewer.get_formatted_loop_data(loop_data, group_data)
            wait_result = viewer.get_formatted_wait_data(wait_data, wait_signal_data, [])

        loop_args = {event[5]['Physic Stream Id']: event[5] for event in loop_result}
        self.assertIn('Data Size', loop_args[1])
        self.assertNotIn('Data Size', loop_args[2])
        wait_args = {event[5]['Physic Stream Id']: event[5] for event in wait_result}
        self.assertIn('Mask', wait_args[1])
        self.assertNotIn('Mask', wait_args[2])

    def test_channel_delay_should_use_strict_boundary_prefix_max_and_smallest_channel_tie_break(self):
        viewer = CCUMissionViewer.__new__(CCUMissionViewer)
        viewer.pid = 1
        viewer.tid = 1
        wait_data = [
            OriginMissionDto(stream_id=1, task_id=10, setckebit_instr_id=200, rel_id=5, start_time=2000,
                             end_time=8000, time_type='Wait')
        ]
        wait_signal_data = [
            OriginWaitSignalInfoDto(stream_id=1, task_id=10, instr_id=200, die_id=0, mask=255, channel_id=4),
            OriginWaitSignalInfoDto(stream_id=1, task_id=10, instr_id=200, die_id=0, mask=255, channel_id=0),
            OriginWaitSignalInfoDto(stream_id=1, task_id=10, instr_id=200, die_id=0, mask=255, channel_id=0)
        ]
        channel_data = [
            OriginChannelDto(channel_id=0, timestamp=7900, avg_bw=77),
            OriginChannelDto(channel_id=4, timestamp=7500, avg_bw=150),
            OriginChannelDto(channel_id=0, timestamp=7000, avg_bw=75),
            OriginChannelDto(channel_id=0, timestamp=7000, avg_bw=150),
            OriginChannelDto(channel_id=0, timestamp=8000, avg_bw=999),
            OriginChannelDto(channel_id=0, timestamp=8000, avg_bw=500),
            OriginChannelDto(channel_id=0, timestamp=9000, avg_bw=1000)
        ]

        with mock.patch.object(InfoConfReader(), 'trans_syscnt_into_local_time', return_value=2), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=6):
            result = viewer.get_formatted_wait_data(wait_data, wait_signal_data, channel_data)

        self.assertEqual(1, len(result))
        self.assertEqual(0, result[0][5]['Maximum Delay Channel'])
        self.assertEqual(150, result[0][5]['Maximum Channel Delay'])

    def test_loop_bandwidth_should_be_omitted_for_nonpositive_duration(self):
        viewer = CCUMissionViewer.__new__(CCUMissionViewer)
        viewer.pid = 1
        viewer.tid = 1
        loop_data = [
            OriginMissionDto(stream_id=1, task_id=10, lp_instr_id=100, start_time=4000, end_time=1000,
                             time_type='LoopGroup')
        ]
        group_data = [
            OriginGroupInfoDto(stream_id=1, task_id=10, instr_id=100, die_id=0, data_size=1024,
                               reduce_op_type='SUM', input_data_type='FP16', output_data_type='FP16')
        ]

        with mock.patch.object(InfoConfReader(), 'trans_syscnt_into_local_time', return_value=4), \
                mock.patch.object(InfoConfReader(), 'duration_from_syscnt', return_value=-3):
            result = viewer.get_formatted_loop_data(loop_data, group_data)

        self.assertEqual(0, result[0][4])
        self.assertIn('Data Size', result[0][5])
        self.assertNotIn('Bandwidth (MB/s)', result[0][5])
