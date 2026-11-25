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

import os
import unittest
import shutil
from unittest import mock
from common_func.platform.chip_manager import ChipManager
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginSetCKBV62Dto
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginLoopGroupV62Dto
from msmodel.hardware.ccu_mission_model import CCUViewerMissionV62Model

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

    def test_format_v62_summary_data_should_return_formatted_data(self):
        chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        try:
            InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
            loopgroup_data = [
                OriginLoopGroupV62Dto(task_id=2, loopgroup_instr_id=3,
                                      loopgroup_start_syscnt=100, loopgroup_stop_syscnt=200)
            ]
            setckb_data = [
                OriginSetCKBV62Dto(task_id=2, setckb_instr_id=8, loop_id=9, loop_cnt=10,
                                   setckb_ckbinfo=11,
                                   setckb_syscnt=150, udf_profiling_data='hash_value', udf_flag=1,
                                   syscnt_type=2, lp_flg=0)
            ]
            check = CCUMissionViewer({}, {})
            loopgroup_ret = check.format_loopgroup_v62_summary_data(loopgroup_data)
            setckb_ret = check.format_setckb_v62_summary_data(setckb_data)
            self.assertEqual(1, len(loopgroup_ret))
            self.assertEqual((2, 3), loopgroup_ret[0][:2])
            self.assertEqual("4.000	", loopgroup_ret[0][2])
            self.assertEqual("4.0	", loopgroup_ret[0][3])
            self.assertEqual(1, len(setckb_ret))
            self.assertEqual((2, 8, 9, 10), setckb_ret[0][:4])
            self.assertEqual('0000000000001011', setckb_ret[0][4])
            self.assertEqual("6.000	", setckb_ret[0][5])
            self.assertEqual('hash_value', setckb_ret[0][6])
        finally:
            ChipManager().chip_id = chip_id

    def test_get_v62_summary_methods_should_return_tuple(self):
        chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        try:
            loop_config_headers = ["Task Id", "Instruction ID", "Configured Start", "Duration"]
            set_config_headers = [
                "Task Id", "Instruction ID", "Loop Id", "Loop Cnt", "CKB Info", "Configured Time",
                "UDF Profiling Data", "UDF Flag", "Syscnt Type", "LP Flag",
            ]
            check = CCUMissionViewer({'headers': loop_config_headers}, {'project': self.DEVICE_PATH})
            with mock.patch.object(CCUViewerMissionV62Model, 'get_loopgroup_summary_data',
                                   return_value=[OriginLoopGroupV62Dto(task_id=2,
                                                                       loopgroup_instr_id=3,
                                                                       loopgroup_start_syscnt=100,
                                                                       loopgroup_stop_syscnt=200)]), \
                    mock.patch.object(CCUViewerMissionV62Model, 'get_setckb_summary_data',
                                      return_value=[OriginSetCKBV62Dto(task_id=2, setckb_instr_id=8,
                                                                       loop_id=9, loop_cnt=10, setckb_ckbinfo=11,
                                                                       setckb_syscnt=150,
                                                                       udf_profiling_data='hash_value',
                                                                       udf_flag=1, syscnt_type=2, lp_flg=0)]):
                loop_headers, loop_data, loop_cnt = check.get_loopgroup_v62_summary_data()
                check.configs['headers'] = set_config_headers
                set_headers, set_data, set_cnt = check.get_setckb_v62_summary_data()
            self.assertEqual(loop_config_headers, loop_headers)
            self.assertEqual(1, loop_cnt)
            self.assertEqual((2, 3), loop_data[0][:2])
            self.assertEqual(set_config_headers, set_headers)
            self.assertEqual(1, set_cnt)
            self.assertEqual((2, 8, 9, 10), set_data[0][:4])
        finally:
            ChipManager().chip_id = chip_id

    # ==================== v6_2 timeline unit tests ====================

    def test_resolve_setckb_time_syscnt_type_1_should_return_zero_duration(self):
        records = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xFF, setckb_syscnt=1000,
                               udf_profiling_data='udf_val', udf_flag=1,
                               syscnt_type=1, lp_flg=0)
        ]
        result = CCUMissionViewer._resolve_setckb_time(records)
        self.assertIsNotNone(result)
        start_syscnt, end_syscnt, ckbinfo, udf_flag, udf_data = result
        self.assertEqual(1000, start_syscnt)
        self.assertEqual(1000, end_syscnt)
        self.assertEqual(0xFF, ckbinfo)
        self.assertEqual(1, udf_flag)
        self.assertEqual('udf_val', udf_data)

    def test_resolve_setckb_time_type_0_and_2_should_return_start_end(self):
        records = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xAA, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xBB, setckb_syscnt=500,
                               udf_profiling_data='end_udf', udf_flag=1,
                               syscnt_type=2, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xCC, setckb_syscnt=300,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=3, lp_flg=0),
        ]
        result = CCUMissionViewer._resolve_setckb_time(records)
        self.assertIsNotNone(result)
        start_syscnt, end_syscnt, ckbinfo, udf_flag, udf_data = result
        self.assertEqual(100, start_syscnt)
        self.assertEqual(500, end_syscnt)
        self.assertEqual(0xBB, ckbinfo)  # from end record
        self.assertEqual(1, udf_flag)     # from end record
        self.assertEqual('end_udf', udf_data)

    def test_resolve_setckb_time_no_valid_records_should_return_none(self):
        records = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=3, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=200,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=0),
        ]
        result = CCUMissionViewer._resolve_setckb_time(records)
        self.assertIsNone(result)

        reversed_records = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=200, udf_profiling_data='',
                               udf_flag=0, syscnt_type=0, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=100, udf_profiling_data='',
                               udf_flag=0, syscnt_type=2, lp_flg=0),
        ]
        self.assertIsNone(CCUMissionViewer._resolve_setckb_time(reversed_records))

    def test_resolve_setckb_time_type_1_takes_priority_over_0_and_2(self):
        records = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0x11, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0x22, setckb_syscnt=200,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=2, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0x33, setckb_syscnt=50,
                               udf_profiling_data='immediate', udf_flag=0,
                               syscnt_type=1, lp_flg=0),
        ]
        result = CCUMissionViewer._resolve_setckb_time(records)
        self.assertIsNotNone(result)
        start_syscnt, end_syscnt, ckbinfo, udf_flag, udf_data = result
        self.assertEqual(50, start_syscnt)
        self.assertEqual(50, end_syscnt)
        self.assertEqual(0x33, ckbinfo)  # from type-1 record
        self.assertEqual('immediate', udf_data)

    def test_format_mission_setckb_should_group_by_task_and_instr_id(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        mission_data = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xAA, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=0),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xBB, setckb_syscnt=500,
                               udf_profiling_data='udf_data', udf_flag=1,
                               syscnt_type=2, lp_flg=0),
            OriginSetCKBV62Dto(task_id=2, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xCC, setckb_syscnt=1000,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=1, lp_flg=0),
        ]
        viewer = CCUMissionViewer({}, {})
        reader = InfoConfReader()
        result = viewer._format_mission_setckb(mission_data, reader)
        self.assertEqual(2, len(result))
        # instr_id=10: type 0→2
        self.assertIn("SetCKB", result[0][0])
        self.assertEqual("4.000", result[0][3])
        self.assertEqual(16.0, result[0][4])
        self.assertEqual(1, result[0][5]["Task Id"])
        self.assertEqual(10, result[0][5]["Instruction ID"])
        self.assertIn("CKB Info", result[0][5])
        self.assertEqual('udf_data', result[0][5]["UDF Profiling Data"])
        # same instr_id under task_id=2: type 1 → zero-duration
        self.assertIn("SetCKB", result[1][0])
        self.assertEqual("40.000", result[1][3])
        self.assertEqual(0, result[1][4])  # duration=0
        self.assertEqual(2, result[1][5]["Task Id"])
        self.assertEqual(10, result[1][5]["Instruction ID"])

    def test_format_mission_setckb_should_skip_incomplete_records(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        mission_data = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=0),
        ]
        viewer = CCUMissionViewer({}, {})
        reader = InfoConfReader()
        result = viewer._format_mission_setckb(mission_data, reader)
        self.assertEqual(0, len(result))

    def test_format_loop_setckb_should_aggregate_by_instr_id(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        loop_data = [
            # instr_id=10, loop_cnt=0: type 0→2
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=5, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=1),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=5, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=300,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=2, lp_flg=1),
            # instr_id=10, loop_cnt=1: type 0→2, shorter duration
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=3, loop_cnt=1,
                               setckb_ckbinfo=0, setckb_syscnt=200,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=1),
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=3, loop_cnt=1,
                               setckb_ckbinfo=0, setckb_syscnt=250,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=2, lp_flg=1),
        ]
        viewer = CCUMissionViewer({}, {})
        reader = InfoConfReader()
        result = viewer._format_loop_setckb(loop_data, reader)
        self.assertEqual(1, len(result))  # one instr_id
        self.assertIn("SetCKB", result[0][0])
        args = result[0][5]
        self.assertEqual(1, args["Task Id"])
        self.assertEqual(10, args["Instruction ID"])
        self.assertEqual(5, args["Earliest Loop Id"])
        self.assertEqual(5, args["Latest Loop Id"])
        self.assertEqual(5, args["Longest Duration Loop Id"])
        self.assertEqual("4.000", result[0][3])
        self.assertEqual(8.0, result[0][4])
        self.assertEqual(8.0, args["Longest Duration(us)"])

    def test_format_loop_setckb_same_instr_id_across_tasks(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        loop_data = [
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=1, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=1, lp_flg=1),
            OriginSetCKBV62Dto(task_id=2, setckb_instr_id=10, loop_id=2, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=500,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=0, lp_flg=1),
            OriginSetCKBV62Dto(task_id=2, setckb_instr_id=10, loop_id=2, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=700,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=2, lp_flg=1),
        ]
        viewer = CCUMissionViewer({}, {})
        reader = InfoConfReader()
        result = viewer._format_loop_setckb(loop_data, reader)
        self.assertEqual(2, len(result))
        # instr 10: type 1, zero-duration
        self.assertEqual(0, result[0][4])
        self.assertEqual(10, result[0][5]["Instruction ID"])
        # task 2 reuses instr 10: type 0→2
        self.assertEqual(10, result[1][5]["Instruction ID"])
        self.assertEqual("20.000", result[1][3])
        self.assertEqual(8.0, result[1][4])

    def test_format_setckb_v62_timeline_should_split_mission_and_loop(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        setckb_data = [
            # mission
            OriginSetCKBV62Dto(task_id=1, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                               setckb_ckbinfo=0xFF, setckb_syscnt=100,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=1, lp_flg=0),
            # loop
            OriginSetCKBV62Dto(task_id=2, setckb_instr_id=20, loop_id=1, loop_cnt=0,
                               setckb_ckbinfo=0, setckb_syscnt=200,
                               udf_profiling_data='', udf_flag=0,
                               syscnt_type=1, lp_flg=1),
        ]
        viewer = CCUMissionViewer({}, {})
        result = viewer._format_setckb_v62_timeline(setckb_data)
        self.assertEqual(2, len(result))
        # mission: Task Id = 1
        self.assertEqual(1, result[0][5]["Task Id"])
        self.assertIn("CKB Info", result[0][5])
        # loop: Task Id = 2
        self.assertEqual(2, result[1][5]["Task Id"])

    def test_format_loopgroup_v62_timeline_should_return_correct_trace(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
        loopgroup_data = [
            OriginLoopGroupV62Dto(task_id=1, loopgroup_instr_id=100,
                                  loopgroup_start_syscnt=1000, loopgroup_stop_syscnt=5000),
        ]
        viewer = CCUMissionViewer({}, {})
        result = viewer._format_loopgroup_v62_timeline(loopgroup_data)
        self.assertEqual(1, len(result))
        self.assertEqual("LoopGroup", result[0][0])
        self.assertEqual(1, result[0][5]["Task Id"])
        self.assertEqual(100, result[0][5]["Instruction ID"])
        self.assertEqual("40.000", result[0][3])
        self.assertEqual(160.0, result[0][4])

    def test_get_v62_timeline_data_should_merge_loopgroup_and_setckb(self):
        chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        try:
            InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "25"}]}
            with mock.patch.object(CCUViewerMissionV62Model, 'get_loopgroup_summary_data',
                                   return_value=[
                                       OriginLoopGroupV62Dto(task_id=1, loopgroup_instr_id=100,
                                                             loopgroup_start_syscnt=1000,
                                                             loopgroup_stop_syscnt=3000)
                                   ]), \
                    mock.patch.object(CCUViewerMissionV62Model, 'get_setckb_summary_data',
                                      return_value=[
                                          OriginSetCKBV62Dto(task_id=2, setckb_instr_id=10, loop_id=0, loop_cnt=0,
                                                             setckb_ckbinfo=0xF, setckb_syscnt=500,
                                                             udf_profiling_data='', udf_flag=0,
                                                             syscnt_type=1, lp_flg=0)
                                      ]):
                viewer = CCUMissionViewer({}, {'project': self.DEVICE_PATH})
                result = viewer._get_v62_timeline_data()
            # header (2 meta events) + loopgroup (1) + setckb (1) = 4 entries before time_graph_trace
            self.assertGreater(len(result), 0)
            # Find LoopGroup event
            loop_events = [e for e in result if isinstance(e, dict) and e.get('name') == 'LoopGroup']
            setckb_events = [e for e in result if isinstance(e, dict) and 'SetCKB' in str(e.get('name', ''))]
            self.assertEqual(1, len(loop_events))
            self.assertEqual(1, len(setckb_events))
        finally:
            ChipManager().chip_id = chip_id

    def test_get_v62_timeline_data_empty_should_return_empty_list(self):
        chip_id = ChipManager().chip_id
        ChipManager().chip_id = ChipModel.CHIP_V6_2_0
        try:
            with mock.patch.object(CCUViewerMissionV62Model, 'get_loopgroup_summary_data',
                                   return_value=[]), \
                    mock.patch.object(CCUViewerMissionV62Model, 'get_setckb_summary_data',
                                      return_value=[]):
                viewer = CCUMissionViewer({}, {'project': self.DEVICE_PATH})
                result = viewer._get_v62_timeline_data()
            self.assertEqual([], result)
        finally:
            ChipManager().chip_id = chip_id
