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

from common_func.info_conf_reader import InfoConfReader
from viewer.association.host_connect_device import HostToDevice
from common_func.trace_view_header_constant import TraceViewHeaderConstant


NAMESPACE = "viewer.association.host_connect_device"


class TestHostToDevice(unittest.TestCase):
    def test_add_connect_line_api(self):
        api_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Node@launch",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588477.77002,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 390.26,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Thread Id": 2287318, "Mode": "launch", "level": "node",
                    TraceViewHeaderConstant.TRACE_HEADER_ID: "0",
                    "item_id": "hcom_broadcast_", "connection_id": 19,
                }
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Node@launch",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2290334,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368854.88,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 12.6,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Thread Id": 2290334, "Mode": "launch", "level": "node",
                    TraceViewHeaderConstant.TRACE_HEADER_ID: "0",
                    "item_id": "ApplyAdamW", "connection_id": 14983
                },
            },
        ]
        expected_data = api_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "HostToDevice64355789963263",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: "64355789963263",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731802,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2290334,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368854.88
            }
        ]
        InfoConfReader()._info_json = {"devices": 0}
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(4, 33573, 23): (2287318, 4294967295), (4, 33574, 23): (2287318, 4294967295)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_hccl_op_connection_ids", return_value={14983}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            InfoConfReader()._info_json = {"devices": 0}
            connection = HostToDevice("")
            connection.add_connect_line(api_data, "api")
            self.assertEqual(api_data, expected_data)

    def test_add_connect_line_task_time(self):
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        task_time_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "SDMA",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "SDMA", "Physic Stream Id": 11, "Task Id": 4043,
                    "Batch Id": 0, "Subtask Id": 0, "connection_id": 19,
                }
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "ApplyAdamW",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368863,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 31.92,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Physic Stream Id": 4, "Task Id": 33573,
                    "Batch Id": 23, "Subtask Id": 4294967295, "connection_id": 14983
                },
            },
        ]
        connection_id = (0 << 80) + (4 << 64) + (33573 << 48) + (23 << 32) + 4294967295
        expected_data = task_time_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "34789457752.030"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134525368863,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(0, 4, 33573, 23): (2287318, 4294967295)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_cann_pid", return_value=3), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            InfoConfReader()._info_json = {"devices": 0}
            InfoConfReader()._host_freq = 123456
            connection = HostToDevice("")
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, task_time_data)

            connection.exist_api = True
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, expected_data)
    
    def test_add_connect_line_should_add_one_line_when_data_is_acl_event(self):
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        acl_event_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "EVENT_RECORD",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "EVENT_RECORD", "Physic Stream Id": 11, "Task Id": 4043,
                    "Batch Id": 0, "Subtask Id": 4294967295, "connection_id": 19,
                }
            },
        ]
        connection_id = (0 << 80) + (11 << 64) + (4043 << 48) + (0 << 32) + 4294967295
        expected_data = acl_event_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134524588600"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{connection_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(connection_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]
        data = {
            TraceViewHeaderConstant.TRACE_HEADER_NAME: "AscendCL@aclrtRecordEvent",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 11,
                TraceViewHeaderConstant.TRACE_HEADER_TS: '134524588600',
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Thread Id": "11", "Mode": 'ACL_RTS', "level": 'acl',
                    "id": 'aclrtRecordEvent', "item_id": 0, "connection_id": 19,
                }
        }
        with mock.patch(NAMESPACE + ".HostToDevice.get_cann_pid", return_value=3):
            InfoConfReader()._info_json = {"devices": 0}
            InfoConfReader()._host_freq = 123456
            connection = HostToDevice("")
            connection._acl_event_apis[19] = data
            connection.add_connect_line(acl_event_data, "task_time")
            self.assertEqual(acl_event_data, acl_event_data)
 
            connection.exist_api = True
            connection.add_connect_line(acl_event_data, "task_time")
            self.assertEqual(acl_event_data, expected_data)

    def test_add_connect_line_hccl(self):
        hccl_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "hcom_broadcast__072_0",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731811,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.74,
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {"connection_id": 19},
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
            },
        ]
        expected_data = hccl_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "HostToDevice81604378623",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: "81604378623",
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588800.38,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 228731811,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]
        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks', return_value={(4, 33573, 23), (4, 33574, 23)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_connection_id_to_context_ids_mapping",
                           return_value={14983: [4294967295]}):
            InfoConfReader()._info_json = {"devices": 0}
            connection = HostToDevice("")
            connection.add_connect_line(hccl_data, "communication")
            self.assertEqual(hccl_data, expected_data)

    def test_add_connect_line_should_add_two_line_when_input_one_node_has_two_task(self):
        InfoConfReader()._local_time_offset = 10.0
        InfoConfReader()._host_local_time_offset = 10.0
        task_time_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Task1",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525368863",
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 10, 
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Physic Stream Id": 4, "Task Id": 33573,
                    "Batch Id": 23, "Subtask Id": 1, "connection_id": 14983
                },
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "Task2",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525369863",
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 20,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "Task Type": "AI_CORE", "Physic Stream Id": 4, "Task Id": 33574,
                    "Batch Id": 23, "Subtask Id": 2, "connection_id": 14983
                },
            },
        ]
        task1_conn_id = (0 << 80) + (4 << 64) + (33573 << 48) + (23 << 32) + 1
        task2_conn_id = (0 << 80) + (4 << 64) + (33574 << 48) + (23 << 32) + 2
        expected_data = task_time_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task1_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task1_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "81000528.403"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task1_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task1_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525368863",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task2_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "s",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task2_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 2287318,
                TraceViewHeaderConstant.TRACE_HEADER_TS: "162001046.807"
            },
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f"HostToDevice{task2_conn_id}",
                TraceViewHeaderConstant.TRACE_HEADER_PH: "f",
                TraceViewHeaderConstant.TRACE_HEADER_ID: str(task2_conn_id),
                TraceViewHeaderConstant.TRACE_HEADER_TS: "134525369863",
                TraceViewHeaderConstant.TRACE_HEADER_CAT: "HostToDevice",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 3,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 4,
                TraceViewHeaderConstant.TRACE_HEADER_BP: "e"
            },
        ]

        with mock.patch(NAMESPACE + '.HostToDevice.get_node_tasks',
                        return_value={(0, 4, 33573, 23): (2287318, 10000000),
                                      (0, 4, 33574, 23): (2287318, 20000000)}), \
                mock.patch(NAMESPACE + ".HostToDevice.get_cann_pid", return_value=3):
            InfoConfReader()._info_json = {"devices": 0}
            InfoConfReader()._host_freq = 123456
            connection = HostToDevice("")
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, task_time_data)

            connection.exist_api = True
            connection.add_connect_line(task_time_data, "task_time")
            self.assertEqual(task_time_data, expected_data)

    def test_add_msproftx_ex_start_points_with_valid_data_in_host_dir(self):
        msproftx_data = [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: "test",
                TraceViewHeaderConstant.TRACE_HEADER_PID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588477.77002,
                TraceViewHeaderConstant.TRACE_HEADER_DURATION: 0.0,
                TraceViewHeaderConstant.TRACE_HEADER_PH: "X",
                TraceViewHeaderConstant.TRACE_HEADER_ARGS: {
                    "mark_id": 0, "event_type": 'marker_ex'
                }
            }
        ]
        expected_data = msproftx_data + [
            {
                TraceViewHeaderConstant.TRACE_HEADER_NAME: f'MsTx_0',
                TraceViewHeaderConstant.TRACE_HEADER_PH: 's',
                TraceViewHeaderConstant.TRACE_HEADER_ID: '0',
                TraceViewHeaderConstant.TRACE_HEADER_TS: 134524588477.77002,
                TraceViewHeaderConstant.TRACE_HEADER_CAT: 'MsTx',
                TraceViewHeaderConstant.TRACE_HEADER_PID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_TID: 0,
                TraceViewHeaderConstant.TRACE_HEADER_BP: 'e',
            }
        ]
        connection = HostToDevice("")
        connection.add_connect_line(msproftx_data, "msprof_tx")
        self.assertEqual(msproftx_data, expected_data)

    def test_add_msproftx_ex_start_points_with_invalid_data_in_host_dir(self):
        msproftx_data = []
        connection = HostToDevice("PROF_XXX/host")
        connection.add_connect_line(msproftx_data, "msprof_tx")
        self.assertEqual(msproftx_data, [])

    def test_get_cann_pid(self):
        InfoConfReader()._info_json = {TraceViewHeaderConstant.TRACE_HEADER_PID: 123}
        cann_pid = HostToDevice.get_cann_pid()
        self.assertEqual(cann_pid, 126207)

    def test_get_hccl_op_connection_ids(self):
        with mock.patch("common_func.msvp_common.path_check", return_value=False):
            connection = HostToDevice("")
            ret = connection.get_hccl_op_connection_ids()
            self.assertEqual(ret, set())
