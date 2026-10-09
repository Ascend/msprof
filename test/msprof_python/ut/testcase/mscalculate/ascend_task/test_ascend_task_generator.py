#!/usr/bin/python3
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
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from mscalculate.ascend_task.ascend_task import DeviceTask, TaskTypeDto
from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task_generator import AscendTaskGenerator

NAMESPACE = 'mscalculate.ascend_task.ascend_task_generator'


class TestAscendTaskGenerator(unittest.TestCase):
    def setUp(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5"}

    def tearDown(self) -> None:
        info_reader = InfoConfReader()
        info_reader._info_json = {}

    def test__sep_device_task_by_stream_should_return_3_stream(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._sep_task_by_stream_task_ctx(device_tasks)
        self.assertEqual(len(tasks), 3)

    def test__sep_host_task_by_stream_should_return_3_stream(self):
        host_tasks = [
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(1, 1, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(1, 1, 2, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._sep_task_by_stream_task_ctx(host_tasks)
        self.assertEqual(len(tasks), 3)

    def test__match_host_device_task_should_return_3_matched_1_host_mismatch(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        host_tasks = [
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 3),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._match_host_device_task(host_tasks, device_tasks)
        self.assertEqual(len(tasks[0]), 3)
        self.assertEqual(len(tasks[1]), 1)
        self.assertEqual(len(tasks[2]), 0)

    def test__match_host_device_task_should_return_3_matched_1_device_mismatch(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE"),
        ]
        host_tasks = [
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(1, 1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._match_host_device_task(host_tasks, device_tasks)
        self.assertEqual(len(tasks[0]), 3)
        self.assertEqual(len(tasks[1]), 0)
        self.assertEqual(len(tasks[2]), 1)

    def test__generate_top_down_tasks_when_pure_static_graph_scene(self):
        """
        model_id: int
        index_id: int
        stream_id: int
        task_id: int
        batch_id: int
        task_type: str
        host_timestamp: int
        """
        host_tasks = [
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(1, 0, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(1, 0, 1, 4, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
        ]

        """
        stream_id: int
        task_id: int
        subtask_id: int
        start_time: int
        duration: int
        task_type: str
        """
        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(all_), 5)

    def test__generate_top_down_tasks_when_static_graph_mix_with_dynamic_graph_scene(self):
        host_tasks = [
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(1, 0, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(1, 0, 1, 4, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
            HostTask(1, 1, 1, 6, 4294967295, 0, "AI_CORE", 'aclnn', 0, 5000, 3),
            HostTask(1, 1, 1, 7, 4294967295, 0, "AI_CORE", 'aclnn', 0, 6000, 4),
            HostTask(1, 1, 1, 8, 4294967295, 0, "AI_CORE", 'aclnn', 0, 7000, 5),
            HostTask(1, 1, 1, 65535, 4294967295, 0, "AI_CORE", 'aclnn', 0, 8000, 6),
            HostTask(1, 1, 1, 6, 4294967295, 1, "AI_CORE", 'aclnn', 0, 9000, 7),
        ]

        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE"),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE"),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE"),
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(all_), 11)

    def test__generate_top_down_tasks_operate_scene(self):
        host_tasks = [
            HostTask(4294967295, -1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 1),
            HostTask(4294967295, -1, 1, 4, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 2),
            HostTask(4294967295, -1, 1, 6, 4294967295, 0, "AI_CORE", 'aclnn', 0, 5000, 3),
            HostTask(4294967295, -1, 1, 7, 4294967295, 0, "AI_CORE", 'aclnn', 0, 6000, 4),
            HostTask(4294967295, -1, 1, 8, 4294967295, 0, "AI_CORE", 'aclnn', 0, 7000, 5),
            HostTask(4294967295, -1, 1, 65535, 4294967295, 0, "AI_CORE", 'aclnn', 0, 8000, 6),
            HostTask(4294967295, -1, 1, 6, 4294967295, 1, "AI_CORE", 'aclnn', 0, 9000, 7),
        ]

        device_tasks = [
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE"),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE"),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE"),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE"),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE"),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE"),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE"),
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE"),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE"),
        ]

        generator = AscendTaskGenerator("")
        all_ = generator._generate_top_down_tasks(host_tasks, device_tasks)
        self.assertEqual(len(all_), 11)

    def test__get_all_ascend_tasks(self):
        generator = AscendTaskGenerator("")
        tasks = generator._get_all_ascend_tasks()
        self.assertEqual(tasks, [])
        InfoConfReader()._info_json = {"devices": "0", "drvVersion": 0}
        with mock.patch(NAMESPACE + ".HostTaskCollector.get_host_tasks", return_value=[]), \
                mock.patch(NAMESPACE + ".DeviceTaskCollector.get_all_device_tasks", return_value=[]):
            tasks = generator._get_all_ascend_tasks()
            self.assertEqual(tasks, [])
        InfoConfReader()._info_json = {}

    def test__get_ascend_tasks_within_iter(self):
        generator = AscendTaskGenerator("")
        tasks = generator._get_ascend_tasks_within_iter(10, 1)
        self.assertEqual(tasks, [])
        InfoConfReader()._info_json = {"devices": "0"}
        with mock.patch(NAMESPACE + ".HostTaskCollector.get_host_tasks_by_model_and_iter", return_value=[]), \
                mock.patch(NAMESPACE + ".DeviceTaskCollector.get_device_tasks_by_model_and_iter", return_value=[]):
            tasks = generator._get_ascend_tasks_within_iter(10, 1)
            self.assertEqual(tasks, [])
        InfoConfReader()._info_json = {}

    def test_run_graph_scene(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".AscendTaskGenerator._get_ascend_tasks_within_iter",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE', 0)]):
            tasks = generator.run(10, 1, 4)
            self.assertEqual(len(tasks), 4)

    def test_run_operate_scene(self):
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".AscendTaskGenerator._get_all_ascend_tasks",
                        return_value=[TopDownTask(1, 1, 1, 1, 1, 1, 1000, 1000, 'AI_CORE', 'AI_CORE', 0)]):
            tasks = generator.run(4294967295, 1, 4)
            self.assertEqual(len(tasks), 1)

    def test__sep_task_by_stream_task_ctx_batch_should_return_3_stream(self):
        device_tasks = [
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE", 0),
            DeviceTask(1, 3, 0, 1000, 1000, "AI_CORE", 0),
            DeviceTask(1, 2, 0, 1000, 1000, "AI_CORE", 1),
        ]
        generator = AscendTaskGenerator("")
        tasks = generator._sep_task_by_stream_task_ctx_batch(device_tasks)
        self.assertEqual(len(tasks), 3)

    def test__generate_top_down_tasks_by_batch_id_should_return_11_tasks_when_unique_stream_task_cxt_batch_id(self):
        host_tasks = [
            # match with device
            HostTask(4294967295, -1, 1, 2, 4294967295, 0, "AI_CORE", 'alcnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 2, 4294967295, 1, "AI_CORE", 'alcnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 3, 4294967295, 0, "AI_CORE", 'alcnn', 0, 2000, 0),
            HostTask(4294967295, -1, 1, 4, 4294967295, 0, "AI_CORE", 'alcnn', 0, 3000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 0, "AI_CORE", 'alcnn', 0, 5000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 1, "AI_CORE", 'alcnn', 0, 9000, 0),
            HostTask(4294967295, -1, 1, 7, 4294967295, 0, "AI_CORE", 'alcnn', 0, 6000, 0),
            HostTask(4294967295, -1, 1, 8, 4294967295, 0, "AI_CORE", 'alcnn', 0, 7000, 0),
            # not match with device
            HostTask(4294967295, -1, 1, 65535, 4294967295, 0, "AI_CORE", 'aclnn', 0, 8000, 0),
        ]
        device_tasks = [
            # match with host
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE", 0),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE", 1),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE", 0),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE", 1),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE", 0),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE", 0),
            # not match with host
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE", 0),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE", 0),
        ]
        generator = AscendTaskGenerator("")
        top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        self.assertEqual(len(top_down_tasks), 11)

    def test__generate_top_down_tasks_by_batch_id_should_return_9_tasks_when_not_unique_id(self):
        host_tasks = [
            # not unique
            HostTask(4294967295, -1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            # match with device
            HostTask(4294967295, -1, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 0),
            HostTask(4294967295, -1, 1, 4, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 0, "AI_CORE", 'aclnn', 0, 5000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 1, "AI_CORE", 'aclnn', 0, 9000, 0),
            HostTask(4294967295, -1, 1, 7, 4294967295, 0, "AI_CORE", 'aclnn', 0, 6000, 0),
            HostTask(4294967295, -1, 1, 8, 4294967295, 0, "AI_CORE", 'aclnn', 0, 7000, 0),
            # not match with device
            HostTask(4294967295, -1, 1, 65535, 4294967295, 0, "AI_CORE", 'aclnn', 0, 8000, 0),
        ]
        device_tasks = [
            # not unique
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE", 0),
            # match with host
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE", 0),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE", 1),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE", 0),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE", 0),
            # not match with host
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE", 0),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE", 0),
        ]
        generator = AscendTaskGenerator("")
        top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        self.assertEqual(len(top_down_tasks), 9)

    def test__generate_top_down_tasks_by_batch_id_should_return_12_tasks_when_one_host_task_to_many_device_tasks(self):
        host_tasks = [
            # host task with static mode
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            # match with device
            HostTask(4294967295, -1, 1, 3, 4294967295, 0, "AI_CORE", 'alcnn', 0, 2000, 0),
            HostTask(4294967295, -1, 1, 4, 4294967295, 0, "AI_CORE", 'alcnn', 0, 3000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 0, "AI_CORE", 'alcnn', 0, 5000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 1, "AI_CORE", 'alcnn', 0, 9000, 0),
            HostTask(4294967295, -1, 1, 7, 4294967295, 0, "AI_CORE", 'alcnn', 0, 6000, 0),
            HostTask(4294967295, -1, 1, 8, 4294967295, 0, "AI_CORE", 'alcnn', 0, 7000, 0),
            # not match with device
            HostTask(4294967295, -1, 1, 65535, 4294967295, 0, "AI_CORE", 'alcnn', 0, 8000, 0),
        ]
        device_tasks = [
            # 3 device tasks with static mode
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE", 0),
            DeviceTask(1, 2, 4294967295, 1001, 1000, "AI_CORE", 0),
            DeviceTask(1, 2, 4294967295, 1002, 1000, "AI_CORE", 0),
            # match with host
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE", 0),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE", 1),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE", 0),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE", 0),
            # not match with host
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE", 0),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE", 0),
        ]
        generator = AscendTaskGenerator("")
        top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        self.assertEqual(len(top_down_tasks), 12)

    def test__generate_top_down_tasks_by_batch_id_should_return_host_tasks_when_devices_task_is_empty(self):
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "4", "hwts": 1560, "DeviceInfo": [{'hwts_frequency': 100}]}
        ChipManager().load_chip_info()
        host_tasks = [
            # host task with static mode
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
        ]
        device_tasks = []
        generator = AscendTaskGenerator("")
        with mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_all_data",
                        return_value=[TaskTypeDto(4294967295, 1, 2, '13', 0), TaskTypeDto(4294967296, 1, 2, '13', 2)]):
            top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
            self.assertEqual(len(top_down_tasks), 1)
            self.assertEqual(top_down_tasks[0].duration, 10)

    def test__generate_top_down_tasks_by_batch_id_should_return_host_tasks_when_step_trace_has_no_end(self):
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "4", "hwts": 1560, "DeviceInfo": [{'hwts_frequency': 100}]}
        ChipManager().load_chip_info()
        host_tasks = [
            # host task with static mode
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
        ]
        device_tasks = []
        generator = AscendTaskGenerator("")
        with mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_all_data",
                        return_value=[TaskTypeDto(4294967295, 1, 2, '13', 0)]):
            top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
            self.assertEqual(len(top_down_tasks), 1)
            self.assertEqual(top_down_tasks[0].duration, -1)

    def test__generate_top_down_tasks_by_batch_id_should_return_host_tasks_when_step_trace_has_no_start(self):
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "4", "hwts": 1560, "DeviceInfo": [{'hwts_frequency': 100}]}
        ChipManager().load_chip_info()
        host_tasks = [
            # host task with static mode
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
        ]
        device_tasks = []
        generator = AscendTaskGenerator("")
        with mock.patch("msmodel.step_trace.ts_track_model.TsTrackModel.get_all_data",
                        return_value=[TaskTypeDto(4294967295, 1, 2, '13', 2)]):
            top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
            self.assertEqual(len(top_down_tasks), 1)
            self.assertEqual(top_down_tasks[0].duration, -1)

    def test__generate_top_down_tasks_by_batch_id_should_return_host_tasks_when_no_step_trace(self):
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "4", "hwts": 1560, "DeviceInfo": [{'hwts_frequency': 100}]}
        ChipManager().load_chip_info()
        host_tasks = [
            # host task with static mode
            HostTask(1, 0, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
        ]
        device_tasks = []
        generator = AscendTaskGenerator("")
        top_down_tasks = generator._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        self.assertEqual(len(top_down_tasks), 1)
        self.assertEqual(top_down_tasks[0].duration, -1)

    def test__get_all_ascend_tasks_should_return_11_tasks_when_all_export_version(self):
        host_tasks = [
            # match with device
            HostTask(4294967295, -1, 1, 2, 4294967295, 0, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 2, 4294967295, 1, "AI_CORE", 'aclnn', 0, 1000, 0),
            HostTask(4294967295, -1, 1, 3, 4294967295, 0, "AI_CORE", 'aclnn', 0, 2000, 0),
            HostTask(4294967295, -1, 1, 4, 4294967295, 0, "AI_CORE", 'aclnn', 0, 3000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 0, "AI_CORE", 'aclnn', 0, 5000, 0),
            HostTask(4294967295, -1, 1, 6, 4294967295, 1, "AI_CORE", 'aclnn', 0, 9000, 0),
            HostTask(4294967295, -1, 1, 7, 4294967295, 0, "AI_CORE", 'aclnn', 0, 6000, 0),
            HostTask(4294967295, -1, 1, 8, 4294967295, 0, "AI_CORE", 'aclnn', 0, 7000, 0),
            # not match with device
            HostTask(4294967295, -1, 1, 65535, 4294967295, 0, "AI_CORE", 'aclnn', 0, 8000, 0),
        ]
        device_tasks = [
            # match with host
            DeviceTask(1, 2, 4294967295, 1000, 1000, "AI_CORE", 0),
            DeviceTask(1, 2, 4294967295, 9000, 1000, "AI_CORE", 1),
            DeviceTask(1, 3, 4294967295, 3000, 1000, "AI_CORE", 0),
            DeviceTask(1, 4, 4294967295, 7000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 11000, 1000, "AI_CORE", 0),
            DeviceTask(1, 6, 4294967295, 19000, 1000, "AI_CORE", 1),
            DeviceTask(1, 7, 4294967295, 13000, 1000, "AI_CORE", 0),
            DeviceTask(1, 8, 4294967295, 15000, 1000, "AI_CORE", 0),
            # not match with host
            DeviceTask(1, 9, 4294967295, 17000, 1000, "AI_CORE", 0),
            DeviceTask(2, 2, 4294967295, 5000, 1000, "AI_CORE", 0),
        ]
        InfoConfReader()._info_json = {"devices": "0", "drvVersion": InfoConfReader().ALL_EXPORT_VERSION}
        generator = AscendTaskGenerator("")
        with mock.patch(NAMESPACE + ".HostTaskCollector.get_host_tasks", return_value=host_tasks), \
                mock.patch(NAMESPACE + ".DeviceTaskCollector.get_all_device_tasks", return_value=device_tasks):
            tasks = generator._get_all_ascend_tasks()
            self.assertEqual(len(tasks), 11)
        InfoConfReader()._info_json = {}


