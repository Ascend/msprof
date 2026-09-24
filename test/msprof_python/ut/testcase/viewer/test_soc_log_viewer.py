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
import json
import unittest
from collections import OrderedDict
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from mscalculate.ascend_task.ascend_task import TopDownTask
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.task_time_viewer import TaskTimeViewer
from constant.info_json_construct import InfoJson
from constant.info_json_construct import DeviceInfo

NAMESPACE = 'viewer.task_time_viewer'


class TestData:

    def __init__(self, *args):
        self.task_id = args[0]
        self.stream_id = args[1]
        self.subtask_id = args[2]
        self.context_id = args[3]
        self.op_name = args[4]
        self.task_type = args[5]
        self.ffts_type = args[6]
        self.subtask_type = args[7]
        self.task_time = args[8]
        self.dur_time = args[9]
        self.start_time = args[10]
        self.end_time = args[11]
        self.ffts_type = args[12]
        self.thread_id = args[13]
        self.batch_id = args[14]


class TestTaskTimeViewer(unittest.TestCase):

    def test_get_time_timeline_header(self):
        data = (['thread', 2, 3, 4, 5],)
        configs, params = {}, {}
        InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info]).apply()
        check = TaskTimeViewer(configs, params)
        ret = check.get_time_timeline_header(data)
        self.assertEqual(ret, [['process_name', 1000, 0, 'Task Scheduler'],
                               ['thread_name', 2, 3, 'Stream 3'],
                               ['thread_sort_index', 2, 3, 3]])

    def test_get_timeline_data(self):
        configs, params = {}, {"project": ""}
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ascend_task_data', return_value={}), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_trace_timeline', return_value=[]):
            check = TaskTimeViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(ret, [])

    def test_get_trace_timeline(self):
        configs, params = {}, {}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        setattr(data.get('task_data_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        setattr(data.get('task_data_list', [])[0], 'task_type', 'AI_CORE')
        setattr(data.get('subtask_data_list', [])[0], 'task_type', 'AI_CORE')
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_node_name_and_type', return_value=data), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_time_timeline_header', return_value=()):
            check = TaskTimeViewer(configs, params)
            InfoJson(devices='0').apply()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_format_task_type_data(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        setattr(data.get('task_data_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        setattr(data.get('task_data_list', [])[0], 'task_type', 'AI_CORE')
        setattr(data.get('subtask_data_list', [])[0], 'task_type', 'AI_CORE')
        setattr(data.get('subtask_data_list', [])[0], 'thread_id', 1)
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_node_name_and_type'), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_thread_id'), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_time_timeline_header', return_value=()):
            check = TaskTimeViewer(configs, params)
            InfoJson(devices='0').apply()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_get_device_task_type_should_return_device_type_when_device_is_not_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("AI_CPU")
        self.assertEqual(result, "AI_CPU")

    def test_get_device_task_type_should_return_other_when_v3_1_0_and_device_belongs_to_sqetype(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "AI_CORE")

    def test_get_device_task_type_should_return_other_when_v3_1_0_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("25")
        self.assertEqual(result, "Other")

    def test_get_device_task_type_should_return_other_when_v2_1_0_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "Other")

    def test_get_device_task_type_should_return_other_when_v2_1_0_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "Other")
        InfoConfReader()._info_json = {}

    def test_get_device_task_type_should_return_sdma_when_v3_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("9")
        self.assertEqual(result, "SDMA_SQE")
        InfoConfReader()._info_json = {}

    def test_get_device_task_type_should_return_sdma_when_stars_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        result = check.get_device_task_type("11")
        self.assertEqual(result, "SDMA_SQE")

        ChipManager().chip_id = ChipModel.CHIP_V1_1_1
        result = check.get_device_task_type("11")
        self.assertEqual(result, "SDMA_SQE")
        InfoConfReader()._info_json = {}

    def test_add_node_name_and_type(self):
        configs, params = {}, {}
        node_dict = {
            '2-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '2-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        InfoConfReader()._info_json = {'pid': '0'}
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'MatMul_1_lxslice1')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'Add')

    def test_add_node_name_should_return_device_when_host_is_ffts_plus(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AIV", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        InfoConfReader()._info_json = {"pid": "0"}
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'AIV')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AIV')

    def test_add_node_name_should_return_op_name_when_op_name_exists_and_host_is_unknown(self):
        configs, params = {}, {}
        node_dict = {
            '2-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '2-27-4294967295-0': {
                'op_name': 'MatMul/v2_MemSet',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "UNKNOWN", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'MatMul_1_lxslice1')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'MatMul/v2_MemSet')

    def test_add_node_name_should_return_other_when_host_is_unknown_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICORE", "AI_CORE", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V3_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICORE')

    def test_add_node_name_should_return_sqetype_when_host_is_unknown_and_device_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'EVENT_WAIT_SQE')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_v2_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'AIV')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_chip_v1_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "SDMA", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'SDMA')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_chip_v1_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'EVENT_WAIT_SQE')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICPU')

    def test_add_node_name_should_return_other_when_v2_1_0_and_host_is_unknown_and_device_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "15", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICPU')

    def test_add_node_name_should_return_other_when_v2_1_0_and_host_is_unknown_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': {
                'op_name': 'MatMul_1_lxslice1',
                'task_type': 'AI_CORE'
            },
            '3-27-4294967295-0': {
                'op_name': 'Add',
                'task_type': 'AI_CORE'
            },
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "EVENT_WAIT", "0", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1),
            ],
        }
        with mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.init', return_value=True), \
                mock.patch('msmodel.stars.op_summary_model.OpSummaryModel.get_op_name_from_ge_by_id',
                           return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name_and_type(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'EVENT_WAIT')

    def test_get_ascend_task_data_should_split_acsq_task_and_subtask_by_context_id(self):
        InfoConfReader()._info_json = {"pid": 0}
        top_down_task_type = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TopDownTask, [])
        configs = {}
        params = {"project": './'}
        data = [
            top_down_task_type(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            top_down_task_type(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
        ]
        with mock.patch(NAMESPACE + '.AscendTaskModel.get_ascend_task_data_without_unknown', return_value=data), \
                mock.patch("common_func.db_manager.DBManager.check_connect_db", return_value=(True, True)):
            check = TaskTimeViewer(configs, params)
            ret = check.get_ascend_task_data()
            self.assertEqual(len(ret), 2)
            self.assertEqual(len(ret.get('task_data_list', [])), 1)
            self.assertEqual(ret.get('task_data_list', [])[0].context_id, 4294967295)
            self.assertEqual(len(ret.get('subtask_data_list', [])), 1)
            self.assertEqual(ret.get('subtask_data_list', [])[0].context_id, 47)
        InfoConfReader()._info_json = {}

    def test_add_thread_id_should_add_thread_id_when_matched(self):
        configs = {}
        params = {"project": './'}
        data = {
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 0),
            ],
        }
        subtask_data = [TaskTimeDto()]
        subtask_data[0].stream_id = 36
        subtask_data[0].task_id = 2
        subtask_data[0].subtask_id = 47
        subtask_data[0].start_time = 38140480645103
        subtask_data[0].thread_id = 10
        with mock.patch(NAMESPACE + '.SubTaskTimeModel.get_all_data', return_value=subtask_data), \
                mock.patch('msmodel.interface.parser_model.ParserModel.init'):
            check = TaskTimeViewer(configs, params)
            check.add_thread_id(data)
            self.assertEqual(data.get("subtask_data_list")[0].thread_id, 10)

    def test_get_time_timeline_header_should_have_thread_name_when_thread_task_time(self):
        data = (['thread', 2, 1, 4, 5],)
        configs, params = {}, {}
        InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info]).apply()
        check = TaskTimeViewer(configs, params)
        ret = check.get_time_timeline_header(data, TraceViewHeaderConstant.PROCESS_THREAD_TASK)
        self.assertEqual(ret, [['process_name', 2, 0, 'Thread Task Time'],
                               ['thread_name', 2, 1, 'Thread 0(AIV)'],
                               ['thread_sort_index', 2, 1, 1]])

if __name__ == '__main__':
    unittest.main()
