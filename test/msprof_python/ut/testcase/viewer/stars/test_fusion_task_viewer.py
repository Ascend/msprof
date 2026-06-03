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
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.fusion_task_viewer import FusionTaskViewer

NAMESPACE = 'viewer.stars.fusion_task_viewer'


class TestFusionTaskViewer(unittest.TestCase):

    def test_get_timeline_data_empty_db(self):
        config = {'result_dir': 'test'}
        with mock.patch("os.path.exists", return_value=False):
            viewer = FusionTaskViewer(config)
            ret = viewer.get_timeline_data()
        self.assertEqual(ret, [])

    def test_get_trace_timeline(self):
        """10-field raw data: [stream_id, task_id, acc_id, task_type, start, end, dur, type_name, mission_id, ccu_die_id]"""
        fusion_data = [
            [0, 100, 3, 'AI_CORE', 56, 60, 4, 'AICORE', 0, None],
            [0, 200, 7, 'CCU_SQE', 80, 90, 10, 'CCU', 1, 0],
        ]
        config = {}
        with mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', return_value=[]):
            viewer = FusionTaskViewer(config)
            InfoConfReader()._info_json = {
                'pid': 1, 'tid': 2,
                "DeviceInfo": [{'hwts_frequency': 1000, 'device_id': 0}],
            }
            ret = viewer.get_trace_timeline(fusion_data)
        # metadata_event (1 'M' event) + time_graph_trace response
        self.assertEqual(len(ret), 1)

    def test_timeline_non_ccu_args(self):
        """non-CCU tasks: no mission_id or ccu_die_id in args"""
        fusion_data = [[0, 100, 3, 'AI_CORE', 56, 60, 4, 'CPU', 5, None]]
        config = {}
        mock_trace = mock.MagicMock(return_value=[])
        with mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', mock_trace):
            viewer = FusionTaskViewer(config)
            InfoConfReader()._info_json = {
                'pid': 1, 'tid': 2,
                "DeviceInfo": [{'hwts_frequency': 1000, 'device_id': 0}],
            }
            viewer.get_trace_timeline(fusion_data)
        args_dict = mock_trace.call_args[0][1][0][5]
        self.assertNotIn('MISSION ID', args_dict)
        self.assertNotIn('CCU DIE ID', args_dict)
        self.assertEqual(args_dict['FUSION TASK TYPE'], 'CPU')

    def test_timeline_ccu_args(self):
        """CCU tasks include mission_id and ccu_die_id in args"""
        fusion_data = [[0, 100, 3, 'CCU_SQE', 56, 60, 4, 'CCU', 3, 0]]
        config = {}
        mock_trace = mock.MagicMock(return_value=[])
        with mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace', mock_trace):
            viewer = FusionTaskViewer(config)
            InfoConfReader()._info_json = {
                'pid': 1, 'tid': 2,
                "DeviceInfo": [{'hwts_frequency': 1000, 'device_id': 0}],
            }
            viewer.get_trace_timeline(fusion_data)
        args_dict = mock_trace.call_args[0][1][0][5]
        self.assertEqual(args_dict['MISSION ID'], 3)
        self.assertEqual(args_dict['CCU DIE ID'], 0)
        self.assertEqual(args_dict['FUSION TASK TYPE'], 'CCU')

    def test_get_timeline_header(self):
        config = {}
        viewer = FusionTaskViewer(config)
        InfoConfReader()._info_json = {'pid': 1, 'tid': 2}
        ret = viewer.get_timeline_header()
        self.assertEqual(len(ret), 1)
        self.assertEqual(ret[0][0], 'process_name')
        self.assertEqual(ret[0][3], 'Fusion Task')

    def test_get_timeline_data(self):
        config = {'result_dir': 'test'}
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch('msmodel.stars.fusion_task_model.FusionTaskModel.get_timeline_data',
                           return_value=[[0, 100, 3, 'AI_CORE', 56, 60, 4, 'CPU', 0, None]]), \
                mock.patch(NAMESPACE + '.FusionTaskViewer.get_trace_timeline', return_value=['trace']):
            viewer = FusionTaskViewer(config)
            ret = viewer.get_timeline_data()
        self.assertEqual(ret, ['trace'])

    def test_get_timeline_data_empty(self):
        config = {'result_dir': 'test'}
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch('msmodel.stars.fusion_task_model.FusionTaskModel.get_timeline_data',
                           return_value=[]):
            viewer = FusionTaskViewer(config)
            ret = viewer.get_timeline_data()
        self.assertEqual(ret, [])


if __name__ == '__main__':
    unittest.main()
