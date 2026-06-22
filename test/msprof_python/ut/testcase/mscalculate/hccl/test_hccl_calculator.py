#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from constant.constant import CONFIG
from constant.constant import clear_dt_project
from mscalculate.hccl.hccl_calculator import HcclCalculator
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask

NAMESPACE = 'mscalculate.hccl.hccl_calculator'

HcclTask = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclTask, [])
HcclOps = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclOps, [])


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    @staticmethod
    def construct_hccl_dto(op_name, is_master, timestamp=123456, duration=0, op_type="HCCL_OP_TYPE"):
        hccl_data = HcclTask()
        hccl_data.op_name, hccl_data.iteration, hccl_data.duration, hccl_data.first_timestamp, hccl_data.is_dynamic, \
            hccl_data.model_id, hccl_data.index_id, hccl_data.task_type, hccl_data.op_type, hccl_data.is_master = \
            (op_name, 1, duration, timestamp, 0, 1, 1, "HCCL", op_type, is_master)
        return hccl_data

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_calculate_should_return_none_when_table_not_in_db(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            check = HcclCalculator([], CONFIG)
            ret = check.calculate()
            self.assertIsNone(ret)
        InfoConfReader()._start_info.clear()

    def test_calculate_should_update_both_hccl_data_and_hccl_op_report_data_when_op_type_valid(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.HcclCalculator._merge_hccl_ops_and_tasks', return_value=[
                    HcclTask(op_name="hccl_op", timestamp=1, duration=1, op_type="all_reduce")]):
            InfoConfReader()._info_json = {"devices": "0"}
            InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
            InfoConfReader()._end_info = {}
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_task_data = check._hccl_task_data
            hccl_op_report_data = check._hccl_op_report_data
            hccl_op_data = check._hccl_op_data
            self.assertEqual(30, len(hccl_task_data[0]))
            self.assertEqual([("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)], hccl_op_report_data)
            self.assertEqual([], hccl_op_data)

    def test_calculate_should_update_both_hccl_data_and_hccl_op_report_data_when_op_type_invalid(self):
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True), \
                mock.patch(NAMESPACE + '.HcclCalculator._merge_hccl_ops_and_tasks', return_value=[
                    HcclTask(op_name="hccl_op", timestamp=1, duration=1, op_type=Constant.NA)]):
            InfoConfReader()._info_json = {"devices": "0"}
            InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
            InfoConfReader()._end_info = {}
            check = HcclCalculator([], CONFIG)
            check.calculate()
            hccl_task_data = check._hccl_task_data
            hccl_op_report_data = check._hccl_op_report_data
            hccl_op_data = check._hccl_op_data
            self.assertEqual(30, len(hccl_task_data[0]))
            self.assertEqual([], hccl_op_report_data)
            self.assertEqual([], hccl_op_data)

    def test_generate_hccl_op_info_should_return_three_data_when_the_input_len_is_three(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        hccl_data = [
            HcclTask(op_name="hccl_op1", is_master=1),
            HcclTask(op_name="hccl_op1", timestamp=123457, is_master=1),
            HcclTask(op_name="hccl_op2", timestamp=123458, is_master=1)
        ]
        check = HcclCalculator([], CONFIG)
        check._generate_hccl_op_info(hccl_data)
        self.assertEqual(3, len(check._hccl_task_data))
        InfoConfReader()._start_info.clear()

    def test_cal_total_should_return_5_when_input_is_the_following_task_time(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        task_time = {
            '1': {'count': 2, 'total_time': 4, 'min': 1, 'max': 3, 'avg': 2},
            '2': {'count': 1, 'total_time': 1, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        _cal_total = getattr(check, "_cal_total")
        result = _cal_total(task_time)
        self.assertEqual(result, 5)
        InfoConfReader()._start_info.clear()

    def test_create_report_should_return_list_data_when_input_is_not_empty(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        task_data = {
            '1': {'count': 2, 'total_time': 4, 'min': 1, 'max': 3, 'avg': 2},
            '2': {'count': 1, 'total_time': 1, 'min': 1, 'max': 1, 'avg': 1}
        }
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [("1", 2, 4.0, 1.0, 2.0, 3.0, 80.0),
                                               ("2", 1, 1.0, 1.0, 1.0, 1.0, 20.0)])
        InfoConfReader()._start_info.clear()

    def test_get_hccl_op_report_data_should_return_dict_data_when_input_is_hccldto_list(self):
        args = {'stream id': 2, 'task id': 0, 'context id': 4294967295, 'is_master': '1'}
        hccl_data = [
            HcclTask(op_name="hccl_op1", is_master=1, timestamp=-1, duration=2, op_type="all_reduce"),
            HcclTask(op_name="hccl_op1", is_master=1, timestamp=1, duration=2, op_type="all_reduce"),
            HcclTask(op_name="hccl_op2", is_master=1, timestamp=5, duration=3, op_type="all_reduce"),
            HcclTask(op_name="hccl_op3", is_master=1, timestamp=10, duration=2, op_type="all_gather"),
            HcclTask(op_name="hccl_op4", is_master=1, timestamp=15, duration=3, op_type="all_gather")
        ]
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data,
                         {"all_reduce": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5},
                          "all_gather": {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5}
                          })

    def test_merge_hccl_ops_and_tasks_should_return_five_matched_res_when_input_not_empty(self):
        hccl_ops = [
            HcclOps(timestamp=1, duration=2),
            HcclOps(timestamp=2, duration=2),
            HcclOps(timestamp=3, duration=2),
            HcclOps(timestamp=4, duration=2),
            HcclOps(timestamp=5, duration=2),
            HcclOps(timestamp=6, duration=2),
            HcclOps(timestamp=7, duration=2),
            HcclOps(timestamp=8, duration=2),
            HcclOps(timestamp=9, duration=2, thread_id=111)
        ]

        hccl_tasks = [
            # corner case 1
            HcclTask(host_timestamp=1),
            HcclTask(host_timestamp=3),
            HcclTask(host_timestamp=4),
            HcclTask(host_timestamp=5),
            # corner case 2
            HcclTask(host_timestamp=10),
            # mismatch case 3
            HcclTask(host_timestamp=20)
        ]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(len(communication_data), 5)
        self.assertEqual(len(check._hccl_op_data), 8)

    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hcclops_empty(self):
        hccl_ops = []
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])
        self.assertEqual(check._hccl_op_data, [])

    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hccltasks_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = []
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])
        self.assertEqual(check._hccl_op_data, [])

    def test_merge_hccl_ops_and_tasks_should_return_2_data_when_step_export_and_ops_queue_not_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        ProfilingScene().set_mode(ExportMode.STEP_EXPORT)
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(2, len(communication_data))
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        self.assertEqual(len(check._hccl_op_data), 1)

    def test_merge_hccl_ops_and_tasks_should_return_2_data_when_all_export_and_ops_queue_not_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(2, len(communication_data))
        self.assertEqual(len(check._hccl_op_data), 1)

    def test_get_hccl_op_report_data_should_return_empty_data_when_input_empty(self):
        hccl_data = []
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data, {})

    def test_create_report_should_return_empty_when_input_is_empty(self):
        task_data = []
        check = HcclCalculator([], CONFIG)
        check._create_report(task_data)
        hccl_op_report_data = check._hccl_op_report_data
        self.assertEqual(hccl_op_report_data, [])

    def test_judge_calculate_again_should_return_true_when_not_all_export(self):
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        check = HcclCalculator([], CONFIG)
        self.assertTrue(check._judge_calculate_again())
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_judge_calculate_again_should_return_false_when_tables_in_db(self):
        scene = ProfilingScene()
        scene._scene = Constant.SINGLE_OP
        check = HcclCalculator([], CONFIG)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            self.assertFalse(check._judge_calculate_again())
        scene._scene = None

    def test_judge_calculate_again_should_return_true_when_tables_not_in_db(self):
        scene = ProfilingScene()
        scene._scene = Constant.STEP_INFO
        check = HcclCalculator([], CONFIG)
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            self.assertTrue(check._judge_calculate_again())
        scene._scene = None
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_ms_run(self):
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".HcclCalculator._judge_calculate_again", return_value=True), \
                mock.patch(NAMESPACE + ".HcclCalculator._drop_table"), \
                mock.patch(NAMESPACE + ".HcclCalculator.calculate"), \
                mock.patch(NAMESPACE + ".HcclCalculator.save"):
            check = HcclCalculator([], CONFIG)
            check.ms_run()

    def test_save_should_return_none_when_hccl_data_empty(self):
        check = HcclCalculator([], CONFIG)
        check._hccl_task_data = []
        self.assertIsNone(check.save())

    def test_save_should_return_none_when_hccl_data_not_empty_and_hccl_op_data_empty(self):
        with mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_task_table"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"):
            check = HcclCalculator([], CONFIG)
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = []
            self.assertIsNone(check.save())

    def test_save_should_return_none_when_hccl_data_and_hccl_op_data_not_empty_and_hccl_op_report_data_empty(self):
        with mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_task_table"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_op_table"):
            check = HcclCalculator([], CONFIG)
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = [HcclOps(model_id=4294967295, data_type="INT32")]
            check._hccl_op_report_data = []
            self.assertIsNone(check.save())

    def test_save_should_not_return_none_when_hccl_data_and_hccl_op_data_and_hccl_op_report_data_not_empty(self):
        with mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_task_table"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.insert_data_to_db"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_op_table"), \
                mock.patch("msmodel.hccl.hccl_model.HcclViewModel.rebuild_hccl_op_report_table"):
            check = HcclCalculator([], CONFIG)
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = [HcclOps(model_id=4294967841, data_type="INT32")]
            check._hccl_op_report_data = [("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)]
            check.save()

    def test_calculate_bandwidth_gb_s_should_return_correct_bandwidth_in_GB_S(self):
        ret = HcclCalculator._calculate_bandwidth_gb_s(duration=319959.1875, size=209715200)
        ret_0_duration = HcclCalculator._calculate_bandwidth_gb_s(duration=0, size=666666)
        self.assertEqual(ret, 655.4435946615847)
        self.assertEqual(ret_0_duration, 0)

    def test_calculate_bandwidth_gb_s_should_return_0_when_size_overflow(self):
        ret = HcclCalculator._calculate_bandwidth_gb_s(duration=100, size=Constant.UINT32_MAX)
        self.assertEqual(ret, 0)

    def test_update_bandwidth_should_update_correct_bandwidth(self):
        RDMA = 'RDMA'
        OP_NAME = 'hcom_allReduce__721_0_1'
        RDMA_SEND_NOTIFY = 'RDMA_SEND_NOTIFY'
        Memcpy = 'Memcpy'
        LOCAL = 'LOCAL'
        NOTIFY_WAIT = 'Notify_Wait'
        RDMASend = 'RDMASend'
        INVALID_TYPE = 'INVALID_TYPE'
        SDMA = 'SDMA'
        RDMA_SEND_PAYLOAD = 'RDMA_SEND_PAYLOAD'
        event = [
            HcclTask(op_name=OP_NAME, hccl_name=Memcpy, rdma_type=INVALID_TYPE,
                     timestamp=63888072593921.055, duration=319959.1875, transport_type=SDMA, task_id=1,
                     size=209715200, bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888072915640.34, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_PREPARE',
                     timestamp=63888072917700.47, duration=20, transport_type=LOCAL, task_id=1, size=0, bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888072921720.71, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_CHECK',
                     timestamp=63888072923480.82, duration=4310758.46875, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888077234799.32, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_ACK',
                     timestamp=63888077236859.445, duration=20, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=Memcpy, rdma_type=INVALID_TYPE,
                     timestamp=63888077238999.58, duration=160429.6171875, transport_type=SDMA, task_id=1,
                     size=104857600, bandwidth=-1),
        ]

        HcclCalculator.update_bandwidth(event)

        ans = [
            HcclTask(op_name=OP_NAME, hccl_name=Memcpy, rdma_type=INVALID_TYPE,
                     timestamp=63888072593921.055, duration=319959.1875, transport_type=SDMA, task_id=1,
                     size=209715200, bandwidth=655.4435946615847),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888072915640.34, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=0.01249908453971),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_PREPARE',
                     timestamp=63888072917700.47, duration=20, transport_type=LOCAL, task_id=1, size=0, bandwidth=0),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                     bandwidth=24.28991694888519),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888072921720.71, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=0.01249908453971),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_CHECK',
                     timestamp=63888072923480.82, duration=4310758.46875, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=0),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_NOTIFY,
                     timestamp=63888077234799.32, duration=320.0234375, transport_type=RDMA, task_id=1, size=4,
                     bandwidth=0.01249908453971),
            HcclTask(op_name=OP_NAME, hccl_name=NOTIFY_WAIT, rdma_type='RDMA_PAYLOAD_ACK',
                     timestamp=63888077236859.445, duration=20, transport_type=LOCAL, task_id=1, size=0,
                     bandwidth=0),
            HcclTask(op_name=OP_NAME, hccl_name=Memcpy, rdma_type=INVALID_TYPE,
                     timestamp=63888077238999.58, duration=160429.6171875, transport_type=SDMA, task_id=1,
                     size=104857600, bandwidth=653.6050003625519),
        ]

        for idx, _ in enumerate(event):
            self.assertAlmostEqual(ans[idx].bandwidth, event[idx].bandwidth)

        event2 = [
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=-1),
        ]
        HcclCalculator.update_bandwidth(event2)
        ans2 = [
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=327664.0007812118),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=327664.0007812118),
            HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                     timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1,
                     size=104857600,
                     bandwidth=327664.0007812118),
        ]

        for idx, _ in enumerate(event2):
            self.assertAlmostEqual(ans2[idx].bandwidth, event2[idx].bandwidth)

    def test_update_unclosed_rdma_task_bandwidth(self):
        RDMA = 'RDMA'
        OP_NAME = 'hcom_allReduce__721_0_1'
        RDMASend = 'RDMASend'
        RDMA_SEND_PAYLOAD = 'RDMA_SEND_PAYLOAD'

        events = [
            [0, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=-1)],
            [1, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=-1)],
            [2, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=-1)],
        ]
        idx = 0
        payload_cnt = 3
        HcclCalculator.update_unclosed_rdma_task_bandwidth(idx, payload_cnt, events)
        ans = [
            [0, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=327664.0007812118)],
            [1, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=327664.0007812118)],
            [2, HcclTask(op_name=OP_NAME, hccl_name=RDMASend, rdma_type=RDMA_SEND_PAYLOAD,
                         timestamp=63888072919960.61, duration=320.015625, transport_type=RDMA, task_id=1, size=104857600,
                         bandwidth=327664.0007812118)],
        ]
        for idx, _ in enumerate(events):
            self.assertAlmostEqual(ans[idx][1].bandwidth, events[idx][1].bandwidth)

    def test_get_hccl_op_report_data_should_return_filter_data_when_input_is_hccldto_list(self):
        args = {'stream id': 2, 'task id': 0, 'context id': 4294967295, 'is_master': '1'}
        all_gather = "all_gather"
        all_reduce = "all_reduce"
        hccl_data = [
            HcclTask(op_name="hccl_op1", is_master=1, timestamp=1, duration=2, op_type=all_reduce),
            HcclTask(op_name="hccl_op2", is_master=1, timestamp=5, duration=3, op_type=all_reduce),
            HcclTask(op_name="hccl_op3", is_master=1, timestamp=10, duration=2, op_type=all_gather),
            HcclTask(op_name="hccl_op4", is_master=1, timestamp=15, duration=3, op_type=all_gather),
            HcclTask(op_name="hccl_op5", is_master=0, timestamp=15, duration=3, op_type=all_gather)
        ]
        check = HcclCalculator([], CONFIG)
        hccl_op_report_data = check._get_hccl_op_report_data(hccl_data)
        self.assertEqual(hccl_op_report_data,
                         {all_reduce: {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5},
                          all_gather: {"count": 2, "total_time": 5, "max": 3, "min": 2, "avg": 2.5}
                          })

    def test_update_op_name_by_group_name_should_set_correct_op_name_by_timestamp(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        hcom_allreduce = "hcom_allreduce"
        hcom_allgather = "hcom_allgather"
        allreduce_aicpu = "hcom_allreduce_AicpuKernel"
        expect_op_name_list = ["hcom_allreduce_321_-1_0", "hcom_allreduce_321_-1_0", "hcom_allreduce_321_-1_0",
                               "hcom_allreduce_321_0_0", "hcom_allreduce_321_1_0", "hcom_allgather_321_2_0",
                               "hcom_allreduce_AicpuKernel_321_-1_0", "hcom_allreduce_AicpuKernel_321_0_0",
                               "hcom_allreduce_AicpuKernel_321_1_0"]
        hccl_data = [
            HcclTask(op_name=hcom_allreduce, group_name="45321", timestamp=100, duration=30, first_timestamp=50),
            HcclTask(op_name=hcom_allreduce, group_name="45321", timestamp=140, duration=10, first_timestamp=60),
            HcclTask(op_name=hcom_allreduce, group_name="45321", timestamp=180, duration=10, first_timestamp=70),
            HcclTask(op_name=hcom_allreduce, group_name="45321", timestamp=220, duration=70, first_timestamp=80),
            HcclTask(op_name=hcom_allreduce, group_name="45321", timestamp=330, duration=40, first_timestamp=90),
            HcclTask(op_name=hcom_allgather, group_name="45321", timestamp=380, duration=10, first_timestamp=95),
            HcclTask(op_name=allreduce_aicpu, group_name="45321", timestamp=150, duration=20, first_timestamp=65),
            HcclTask(op_name=allreduce_aicpu, group_name="45321", timestamp=400, duration=70, first_timestamp=100),
            HcclTask(op_name=allreduce_aicpu, group_name="45321", timestamp=500, duration=40, first_timestamp=110)
        ]
        check = HcclCalculator([], CONFIG)
        check.start_time_raw_timestamp = 200
        check.update_op_name_by_group_name(hccl_data)
        op_name_list = []
        for data in hccl_data:
            op_name_list.append(data.op_name)
        self.assertEqual(op_name_list, expect_op_name_list)
        InfoConfReader()._start_info.clear()


