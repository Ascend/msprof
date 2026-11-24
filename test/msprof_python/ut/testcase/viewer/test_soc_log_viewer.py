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
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1, 0.0),
            ],
        }
        setattr(data.get('task_data_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        setattr(data.get('task_data_list', [])[0], 'task_type', 'AI_CORE')
        setattr(data.get('subtask_data_list', [])[0], 'task_type', 'AI_CORE')
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_node_name_and_type', return_value=data), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_block_pmu_data', return_value={}), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_time_timeline_header', return_value=()):
            check = TaskTimeViewer(configs, params)
            InfoJson(devices='0').apply()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_format_task_type_data(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {'pid': 1, 'tid': 0}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1, 0.0),
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
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_block_pmu_data', return_value={}), \
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AIV", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "UNKNOWN", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICORE", "AI_CORE", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "SDMA", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "15", 1, 0.0),
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
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "EVENT_WAIT", "0", 0, 0.0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1, 0.0),
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
            top_down_task_type(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0, 0.0),
            top_down_task_type(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1, 0.0),
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
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 0, 0.0),
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

    def test_get_block_pmu_data_should_group_by_task_id_and_core_type(self):
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto
        InfoConfReader()._info_json = {"pid": "0"}
        pmu_data_list = [
            PmuBlockDto(task_id=0, core_type=0, start_time=100, fake_end_time=200, duration=100),
            PmuBlockDto(task_id=0, core_type=0, start_time=50, fake_end_time=180, duration=130),
            PmuBlockDto(task_id=0, core_type=1, start_time=80, fake_end_time=250, duration=170),
            PmuBlockDto(task_id=1, core_type=0, start_time=120, fake_end_time=300, duration=180),
        ]
        with mock.patch(NAMESPACE + '.V6PmuViewModel.get_all_data', return_value=pmu_data_list), \
                mock.patch(NAMESPACE + '.V6PmuViewModel.init', return_value=True):
            configs, params = {}, {}
            check = TaskTimeViewer(configs, params)
            result = check.get_block_pmu_data()
            self.assertEqual(len(result), 3)
            self.assertEqual(result[(None, 0, 0, None, 0)][0], [50, 100])
            self.assertEqual(result[(None, 0, 0, None, 1)][0], [80])
            self.assertEqual(result[(None, 1, 0, None, 0)][0], [120])

    def test_get_block_pmu_data_should_return_empty_when_no_data(self):
        InfoConfReader()._info_json = {"pid": "0"}
        with mock.patch(NAMESPACE + '.V6PmuViewModel.get_all_data', return_value=[]), \
                mock.patch(NAMESPACE + '.V6PmuViewModel.init', return_value=True):
            configs, params = {}, {}
            check = TaskTimeViewer(configs, params)
            result = check.get_block_pmu_data()
            self.assertEqual(len(result), 0)

    def test_format_task_pmu_data_should_calculate_preprocessing_and_compute_stage(self):
        from mscalculate.ascend_task.ascend_task import TopDownTask
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto
        InfoConfReader()._info_json = {"pid": "0"}
        pmu_block_data = {
            (27, 0, 0, 0, 0): ([100], [PmuBlockDto(start_time=100, fake_end_time=200)]),
            (27, 0, 0, 0, 1): ([100], [PmuBlockDto(start_time=100, fake_end_time=300)]),
        }
        task_data = TopDownTask(0, 1, 27, 2, 0, 0, 100, 100, "KERNEL_AICPU", "AI_CPU", 0, 0.0)
        task_data.task_id = 0
        task_data.start_time = 100
        task_data.duration = 200
        args = {}
        configs, params = {}, {}
        check = TaskTimeViewer(configs, params)
        check.format_task_pmu_data(args, task_data, pmu_block_data)
        self.assertEqual(args.get("AIC Preprocessing Stage(ns)"), str(round(100, 3)))
        self.assertEqual(args.get("AIC ComputeExecution Stage(ns)"), str(round(100, 3)))
        self.assertEqual(args.get("AIV Preprocessing Stage(ns)"), str(round(200, 3)))
        self.assertEqual(args.get("AIV ComputeExecution Stage(ns)"), str(round(0, 3)))

    def test_format_task_pmu_data_should_not_add_pmu_fields_when_key_not_in_pmu_block_data(self):
        from mscalculate.ascend_task.ascend_task import TopDownTask
        InfoConfReader()._info_json = {"pid": "0"}
        pmu_block_data = {}
        task_data = TopDownTask(0, 1, 27, 2, 0, 0, 100, 100, "KERNEL_AICPU", "AI_CPU", 0, 0.0)
        task_data.task_id = 0
        task_data.start_time = 100
        task_data.duration = 200
        args = {}
        configs, params = {}, {}
        check = TaskTimeViewer(configs, params)
        check.format_task_pmu_data(args, task_data, pmu_block_data)
        self.assertNotIn("AIC Preprocessing Stage(ns)", args)
        self.assertNotIn("AIV Preprocessing Stage(ns)", args)

    def test_format_task_pmu_data_should_not_add_pmu_fields_when_fake_end_time_is_zero(self):
        from mscalculate.ascend_task.ascend_task import TopDownTask
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto
        InfoConfReader()._info_json = {"pid": "0"}
        pmu_block_data = {
            (27, 0, 0, 0, 0): ([100], [PmuBlockDto(start_time=100, fake_end_time=0)]),
        }
        task_data = TopDownTask(0, 1, 27, 2, 0, 0, 100, 100, "KERNEL_AICPU", "AI_CPU", 0, 0.0)
        task_data.task_id = 0
        task_data.start_time = 100
        task_data.duration = 200
        args = {}
        configs, params = {}, {}
        check = TaskTimeViewer(configs, params)
        check.format_task_pmu_data(args, task_data, pmu_block_data)
        self.assertNotIn("AIC Preprocessing Stage(ns)", args)

    def test_pmu_stages_keep_stream_batch_and_context_separate(self):
        from types import SimpleNamespace
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto

        task = SimpleNamespace(stream_id=7, task_id=42, batch_id=3, context_id=5,
                               start_time=100, duration=200)
        rows = [PmuBlockDto(stream_id=7, task_id=42, batch_id=3, subtask_id=5,
                            core_type=0, start_time=120, fake_end_time=175)]
        for stream, batch, context in [(8, 3, 5), (7, 4, 5), (7, 3, 6)]:
            rows.append(PmuBlockDto(stream_id=stream, task_id=42, batch_id=batch, subtask_id=context,
                                    core_type=0, start_time=110, fake_end_time=290))
        viewer = TaskTimeViewer({}, {})
        with mock.patch(NAMESPACE + '.V6PmuViewModel.get_all_data', return_value=rows), \
                mock.patch(NAMESPACE + '.V6PmuViewModel.init', return_value=True):
            blocks = viewer.get_block_pmu_data()
        self.assertEqual(len(blocks), 4)
        args = {}
        viewer.format_task_pmu_data(args, task, blocks)
        self.assertEqual(args['AIC Preprocessing Stage(ns)'], '75')
        self.assertEqual(args['AIC ComputeExecution Stage(ns)'], '125')

    def test_pmu_stages_match_repeated_executions(self):
        from types import SimpleNamespace
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto

        viewer = TaskTimeViewer({}, {})
        rows = []
        for iteration in range(33):
            start = 1000 + iteration * 1000
            for core in (0, 1):
                rows.extend([
                    PmuBlockDto(stream_id=7, task_id=42, batch_id=0, subtask_id=5,
                                core_type=core, start_time=start, fake_end_time=start + 100 + iteration + core),
                    PmuBlockDto(stream_id=7, task_id=42, batch_id=0, subtask_id=5,
                                core_type=core, start_time=start + 10, fake_end_time=start + 400)])
        rows.append(PmuBlockDto(stream_id=7, task_id=42, batch_id=0, subtask_id=5,
                                core_type=0, start_time=999, fake_end_time=1499))
        with mock.patch(NAMESPACE + '.V6PmuViewModel.get_all_data', return_value=rows[::-1]), \
                mock.patch(NAMESPACE + '.V6PmuViewModel.init', return_value=True):
            blocks = viewer.get_block_pmu_data()
        for iteration in reversed(range(33)):
            task = SimpleNamespace(stream_id=7, task_id=42, batch_id=0, context_id=5,
                                   start_time=1000 + iteration * 1000, duration=500)
            args = {}
            viewer.format_task_pmu_data(args, task, blocks)
            for core, prefix in enumerate(('AIC', 'AIV')):
                with self.subTest(iteration=iteration, core=prefix):
                    self.assertEqual(args.get(prefix + ' Preprocessing Stage(ns)'), str(100 + iteration + core))
                    self.assertEqual(args.get(prefix + ' ComputeExecution Stage(ns)'), str(400 - iteration - core))
        task.start_time = 50000
        args = {}
        viewer.format_task_pmu_data(args, task, blocks)
        self.assertEqual(args, {})

    def test_pmu_stages_ignore_missing_and_out_of_task_timestamps(self):
        from types import SimpleNamespace
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto

        task = SimpleNamespace(stream_id=7, task_id=42, batch_id=3, context_id=5,
                               start_time=100, duration=200)
        viewer = TaskTimeViewer({}, {})
        for end in (None, 0, 99, 100, 301):
            with self.subTest(fake_end_time=end):
                args = {}
                viewer.format_task_pmu_data(args, task, {
                    (7, 42, 3, 5, 0): ([100], [PmuBlockDto(start_time=100, fake_end_time=end)])})
                self.assertEqual(args, {})

    def test_pmu_stages_preserve_earliest_block_and_handle_null_batch(self):
        from types import SimpleNamespace
        from profiling_bean.db_dto.pmu_block_dto import PmuBlockDto

        task = SimpleNamespace(stream_id=7, task_id=42, batch_id=0, context_id=5,
                               start_time=100, duration=200)
        viewer = TaskTimeViewer({}, {})
        rows = [PmuBlockDto(stream_id=7, task_id=42, batch_id=None, subtask_id=5,
                            core_type=0, start_time=100, fake_end_time=0),
                PmuBlockDto(stream_id=7, task_id=42, batch_id=None, subtask_id=5,
                            core_type=0, start_time=120, fake_end_time=200)]
        with mock.patch(NAMESPACE + '.V6PmuViewModel.get_all_data', return_value=rows), \
                mock.patch(NAMESPACE + '.V6PmuViewModel.init', return_value=True):
            blocks = viewer.get_block_pmu_data()
        self.assertIn((7, 42, 0, 5, 0), blocks)
        args = {}
        viewer.format_task_pmu_data(args, task, blocks)
        self.assertEqual(args, {})
        task.start_time = 120
        viewer.format_task_pmu_data(args, task, blocks)
        self.assertEqual(args['AIC Preprocessing Stage(ns)'], '80')

if __name__ == '__main__':
    unittest.main()
