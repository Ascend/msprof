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
from collections import OrderedDict
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.dpu_track_dto import DPUTrackDto
from viewer.dpu_viewer import DPUViewer

NAMESPACE = 'viewer.dpu_viewer'
MODEL_NAMESPACE = 'msmodel.dpu.dpu_task_model'
TRACE_MANAGER_NAMESPACE = 'common_func.trace_view_manager'


class TestDPUViewer(unittest.TestCase):

    def setUp(self):
        """初始化测试环境"""
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        InfoConfReader()._host_freq = 100
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        self.config = {"headers": []}
        self.params = {"project": "test_dpu_view", "result_dir": "/test/path"}

    def tearDown(self):
        InfoConfReader()._info_json.clear()
        InfoConfReader()._start_info.clear()

    def _create_dpu_track_dto(self, **kwargs):
        """创建 DPUTrackDto 测试数据"""
        defaults = {
            'npu_device_id': 0,
            'dpu_device_id': 1,
            'start_time': 1000,
            'end_time': 2000,
            'stream_id': 1,
            'aicpu_task_id': 100,
            'task_id': 10,
            'task_type': 'KERNEL_AICPU',
            'ccl_tag': 'tag1',
            'data_type': 'FP16',
            'dst_addr': '0x1000',
            'duration_estimated': 500.0,
            'group_name': 'group1',
            'group_name_id': 'group_id1',
            'link_type': 'HCCS',
            'local_rank': 0,
            'notify_id': 1,
            'op_name': 'AllReduce',
            'op_type': 'ALLREDUCE',
            'plane_id': 0,
            'rank_size': 8,
            'rdma_type': 'RDMA',
            'remote_rank': 1,
            'role': 'SRC',
            'data_size': 1024,
            'src_addr': '0x2000',
            'stage': 'stage1',
            'thread_id': 123,
            'transport_type': 'TCP'
        }
        defaults.update(kwargs)
        return DPUTrackDto(**defaults)

    def test_get_column_trace_data_empty(self):
        """测试空数据"""
        viewer = DPUViewer(self.config, self.params)
        result = viewer.get_column_trace_data([], [])
        self.assertEqual(result, [])

    def test_get_column_trace_data_mixed_tracks(self):
        """测试混合数据"""
        viewer = DPUViewer(self.config, self.params)

        task_track = self._create_dpu_track_dto(
            dpu_device_id=1,
            stream_id=1,
            task_id=10,
            thread_id=123,
            start_time=1000,
            end_time=2000,
            task_type='KERNEL_AICPU',
            op_name='Conv2D'
        )

        hccl_track = self._create_dpu_track_dto(
            dpu_device_id=1,
            npu_device_id=0,
            stream_id=2,
            task_id=20,
            thread_id=456,
            start_time=2000,
            end_time=4000,
            op_type='ALLGATHER',
            data_size=4096,
            op_name='HCCL_AllGather'
        )

        result = viewer.get_column_trace_data([task_track], [hccl_track])

        self.assertEqual(len(result), 2)
        self.assertEqual(result[0][0], 'Conv2D')
        self.assertEqual(result[1][0], 'HCCL_AllGather')

    def test_get_time_timeline_header_empty(self):
        """测试空数据的 header"""
        viewer = DPUViewer(self.config, self.params)
        result = viewer.get_time_timeline_header([], [])
        self.assertEqual(result, [])

    def test_get_time_timeline_header_with_tracks(self):
        """测试有数据时的 header"""
        viewer = DPUViewer(self.config, self.params)

        task_track = [
            self._create_dpu_track_dto(dpu_device_id=1, stream_id=1, thread_id=123),
            self._create_dpu_track_dto(dpu_device_id=1, stream_id=2, thread_id=124),
        ]

        hccl_track = [
            self._create_dpu_track_dto(dpu_device_id=1, stream_id=1, thread_id=125),
            self._create_dpu_track_dto(dpu_device_id=2, stream_id=1, thread_id=126),
        ]

        result = viewer.get_time_timeline_header(task_track, hccl_track)

        # 应该有 2 个 device，device 1 有 2 个 stream，device 2 有 1 个 stream
        # 每个 stream 有 3 条记录：process_name, thread_name, thread_sort_index
        expected_count = 2 + (2 + 1) * 2  # 2 process + 3 streams * 2 (thread_name + sort_index)
        self.assertEqual(len(result), expected_count)

        # 检查 process_name
        process_names = [item for item in result if item[0] == 'process_name']
        self.assertEqual(len(process_names), 2)

        # 检查 thread_name
        thread_names = [item for item in result if item[0] == 'thread_name']
        self.assertEqual(len(thread_names), 3)

    def test_get_trace_timeline(self):
        """测试完整的 timeline 生成"""
        with mock.patch(TRACE_MANAGER_NAMESPACE + '.TraceViewManager.metadata_event',
                           return_value=['metadata']), \
                mock.patch(TRACE_MANAGER_NAMESPACE + '.TraceViewManager.column_graph_trace',
                           return_value=['graph_data']):
            viewer = DPUViewer(self.config, self.params)

            task_track = [self._create_dpu_track_dto(dpu_device_id=1, stream_id=1)]
            hccl_track = [self._create_dpu_track_dto(dpu_device_id=1, stream_id=2)]

            result = viewer.get_trace_timeline((task_track, hccl_track))
            expect_data = ['metadata',
                            OrderedDict([('name', 'AllReduce'),
                                      ('pid', 1),
                                      ('tid', 1),
                                      ('ts', '10000010.000'),
                                      ('dur', 10000000.0),
                                      ('args',
                                       OrderedDict([('Thread Id', 123),
                                                    ('Physic Stream Id', 1),
                                                    ('Task Id', 10),
                                                    ('Task Type', 'KERNEL_AICPU')])),
                                      ('ph', 'X')]),
                            OrderedDict([('name', 'AllReduce'),
                                      ('pid', 1),
                                      ('tid', 2),
                                      ('ts', '10000010.000'),
                                      ('dur', 10000000.0),
                                      ('args',
                                       OrderedDict([('Thread Id', 123),
                                                    ('Physic Stream Id', 2),
                                                    ('Task Id', 10),
                                                    ('OP Type', 'ALLREDUCE'),
                                                    ('AI CPU Device Id', 0),
                                                    ('AI CPU Task Id', 100),
                                                    ('Group Name', 'group1(group_id1)'),
                                                    ('Plane Id', 0),
                                                    ('Notify Id', 1),
                                                    ('Duration Estimated(us)', 500.0),
                                                    ('Rank Size', 8),
                                                    ('Src Rank', 0),
                                                    ('Dst Rank', 1),
                                                    ('Transport Type', 'TCP'),
                                                    ('Size(Byte)', 1024),
                                                    ('Bandwidth(GB/s)', 1.024e-7),
                                                    ('Data Type', 'FP16'),
                                                    ('Link Type', 'HCCS'),
                                                    ('Rdma Type', 'RDMA'),
                                                    ('Role', 'SRC'),
                                                    ('Ccl Tag', 'tag1'),
                                                    ('Work Flow Mode', 'N/A'),
                                                    ('Stage', 'stage1')])),
                                      ('ph', 'X')])]
            self.assertEqual(result, expect_data)

    def test_build_trace_data_zero_duration(self):
        """测试 duration 为 0 的情况"""
        viewer = DPUViewer(self.config, self.params)
        track = self._create_dpu_track_dto(
            data_size=1024,
            start_time=1000,
            end_time=1000
        )
        # 应该不抛出异常
        result = viewer._build_trace_data(track, is_hccl=True)
        self.assertIsNotNone(result)
