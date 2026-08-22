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
from common_func.db_name_constant import DBNameConstant
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

# 生产代码中 DBManager.fetch_all_data 会将 dataclass 转为增强型 namedtuple（含 replace 方法）
HcclTask = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclTask, [])
HcclOps = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclOps, [])


class TestHcclCalculator(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_HcclCalculator')

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        InfoConfReader()._sample_json = {"profLevel": "level1"}
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)
        InfoConfReader()._sample_json = None
        InfoConfReader()._start_info.clear()

    # ============================================================
    # calculate / ms_run
    # ============================================================

    def test_calculate_should_return_none_when_table_not_in_db(self):
        InfoConfReader()._start_info = {"collectionTimeBegin": "0"}
        InfoConfReader()._end_info = {}
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            check = HcclCalculator([], CONFIG)
            ret = check.calculate()
            self.assertIsNone(ret)
        InfoConfReader()._start_info.clear()

    def test_ms_run(self):
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".HcclCalculator._judge_calculate_again", return_value=True), \
                mock.patch(NAMESPACE + ".HcclCalculator._drop_table"), \
                mock.patch(NAMESPACE + ".HcclCalculator.calculate"), \
                mock.patch(NAMESPACE + ".HcclCalculator.save"):
            check = HcclCalculator([], CONFIG)
            check.ms_run()

    # ============================================================
    # _judge_calculate_again
    # ============================================================

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

    # ============================================================
    # save
    # ============================================================

    def test_save_should_return_none_when_hccl_data_empty(self):
        check = HcclCalculator([], CONFIG)
        check._hccl_task_data = []
        self.assertIsNone(check.save())

    def test_save_should_skip_op_table_when_op_data_empty(self):
        """op_data 为空时只写 task 表，不写 op 和 report"""
        with mock.patch.object(check := HcclCalculator([], CONFIG), '_model') as mock_model:
            mock_model.__enter__.return_value = mock_model
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = []
            check.save()
            mock_model.rebuild_hccl_task_table.assert_called_once()
            mock_model.insert_data_to_db.assert_any_call(
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, check._hccl_task_data
            )
            # op_data 为空，不调用第二次 insert，也不调用 rebuild_hccl_op_table
            self.assertEqual(1, mock_model.insert_data_to_db.call_count)
            mock_model.rebuild_hccl_op_table.assert_not_called()

    def test_save_should_skip_report_table_when_report_data_empty(self):
        """report_data 为空时写 task 和 op，不写 report"""
        with mock.patch.object(check := HcclCalculator([], CONFIG), '_model') as mock_model:
            mock_model.__enter__.return_value = mock_model
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = [HcclOps(model_id=4294967295, data_type="INT32")]
            check._hccl_op_report_data = []
            check.save()
            mock_model.rebuild_hccl_task_table.assert_called_once()
            mock_model.rebuild_hccl_op_table.assert_called_once()
            mock_model.rebuild_hccl_op_report_table.assert_not_called()
            self.assertEqual(2, mock_model.insert_data_to_db.call_count)
            mock_model.insert_data_to_db.assert_any_call(
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, check._hccl_task_data
            )
            mock_model.insert_data_to_db.assert_any_call(
                DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE, check._hccl_op_data
            )

    def test_save_should_write_all_tables_when_all_data_present(self):
        """三类数据都有时写全部三张表，且每张表写入前调用对应的 rebuild"""
        with mock.patch.object(check := HcclCalculator([], CONFIG), '_model') as mock_model:
            mock_model.__enter__.return_value = mock_model
            check._hccl_task_data = [HcclOps(model_id=4294967295)]
            check._hccl_op_data = [HcclOps(model_id=4294967841, data_type="INT32")]
            check._hccl_op_report_data = [("all_reduce", 1.0, 1.0, 1.0, 1.0, 1.0, 100.0)]
            check.save()
            mock_model.rebuild_hccl_task_table.assert_called_once()
            mock_model.rebuild_hccl_op_table.assert_called_once()
            mock_model.rebuild_hccl_op_report_table.assert_called_once()
            self.assertEqual(3, mock_model.insert_data_to_db.call_count)
            mock_model.insert_data_to_db.assert_any_call(
                DBNameConstant.TABLE_HCCL_OP_REPORT, check._hccl_op_report_data
            )

    # ============================================================
    # _merge_hccl_ops_and_tasks
    # ============================================================

    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hcclops_empty(self):
        hccl_ops = []
        hccl_tasks = [HcclTask(model_id=4294967295), HcclTask(model_id=4294967296)]
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])

    def test_merge_hccl_ops_and_tasks_should_return_empty_list_when_input_hccltasks_empty(self):
        hccl_ops = [HcclOps(model_id=4294967295), HcclOps(model_id=4294967296)]
        hccl_tasks = []
        check = HcclCalculator([], CONFIG)
        communication_data = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)
        self.assertEqual(communication_data, [])

    def test_merge_hccl_ops_and_tasks_should_match_by_thread_and_op_id(self):
        """同 thread_id 下按 task.op_id ↔ op.connection_id 将 master task 时间窗合并到 op 的 start/end"""
        hccl_ops = [
            HcclOps(thread_id=100, connection_id=1),
            HcclOps(thread_id=200, connection_id=2),
        ]
        hccl_tasks = [
            # 同一 op 内的两个 task 四元组不同（task_id 不同），不触发 iter_id 递增
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=100, duration=10, rank_size=8),
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=2, context_id=1, batch_id=1,
                     is_master=1, timestamp=120, duration=5, rank_size=8),
            HcclTask(thread_id=200, op_id=2, stream_id=2, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=300, duration=20, rank_size=4),
        ]
        check = HcclCalculator([], CONFIG)
        result = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

        self.assertEqual(2, len(result))
        # op1(thread=100): start=100, end=125 (从两个 master task 合并)
        self.assertEqual(100, result[0].start)
        self.assertEqual(125, result[0].end)
        self.assertEqual(8, result[0].rank_size)
        self.assertEqual(1, result[0].connection_id)
        # op2(thread=200): start=300, end=320
        self.assertEqual(300, result[1].start)
        self.assertEqual(320, result[1].end)
        self.assertEqual(4, result[1].rank_size)

    def test_merge_hccl_ops_and_tasks_should_assign_iter_id_for_same_thread_op_pair(self):
        """同一 op 反复执行时，靠 task 四元组 (stream_id, task_id, context_id, batch_id) 重复识别 iter_id 递增（从 1 开始）"""
        hccl_ops = [
            HcclOps(thread_id=100, connection_id=1),
        ]
        hccl_tasks = [
            # 第一轮执行：四元组 (1, 1, 1, 1)
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=100, duration=10, rank_size=8),
            # 第二轮执行：同一四元组重复 → iter_id 递增为 2
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=300, duration=10, rank_size=8),
        ]
        check = HcclCalculator([], CONFIG)
        result = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

        # 同一 op 反复执行 → 2 个结果，iter_id 依次 1、2
        self.assertEqual(2, len(result))
        self.assertEqual(1, result[0].connection_id)
        self.assertEqual(1, result[0].iter_id)
        self.assertEqual(100, result[0].start)
        self.assertEqual(1, result[1].connection_id)
        self.assertEqual(2, result[1].iter_id)
        self.assertEqual(300, result[1].start)

    def test_merge_hccl_ops_and_tasks_should_skip_non_master_tasks(self):
        """is_master=0 的 task 不参与合并"""
        hccl_ops = [
            HcclOps(thread_id=100, connection_id=1),
        ]
        hccl_tasks = [
            # non-master 与 master 是同一 op 内的两个不同 task，四元组不同，不触发 iter_id 递增
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=1, context_id=1, batch_id=1,
                     is_master=0, timestamp=100, duration=10, rank_size=8),
            HcclTask(thread_id=100, op_id=1, stream_id=1, task_id=2, context_id=1, batch_id=1,
                     is_master=1, timestamp=200, duration=20, rank_size=8),
        ]
        check = HcclCalculator([], CONFIG)
        result = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

        self.assertEqual(1, len(result))
        self.assertEqual(200, result[0].start)

    def test_merge_hccl_ops_and_tasks_should_group_by_op_id_and_refresh_group_name(self):
        """同一线程多个 op 的 task 时间戳交错时，应按 op_id 聚拢避免单个 op 被拆成多次迭代；
        并把所属 op 的 group_name 回填给 task（task 侧 group_name 与 op 侧不同源，以 op 侧为准）"""
        hccl_ops = [
            HcclOps(thread_id=100, connection_id=1, group_name="domain_a"),
            HcclOps(thread_id=100, connection_id=2, group_name="domain_b"),
        ]
        hccl_tasks = [
            # op1 的两个 master task 被 op2 从时间戳上交错开；若只按 timestamp 排序，op1 会被 op2 打断、错误拆成两次迭代
            # op1 的两个 task 四元组不同（task_id 不同），是同一 op 单次执行的两个 task，不触发 iter_id 递增
            # task 的 group_name 故意设为与所属 op 不一致的旧值，验证会被 op 的 group_name 回填
            HcclTask(thread_id=100, op_id=1, group_name="stale", stream_id=1, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=100, duration=10, rank_size=8),
            HcclTask(thread_id=100, op_id=2, group_name="stale", stream_id=2, task_id=1, context_id=1, batch_id=1,
                     is_master=1, timestamp=200, duration=10, rank_size=4),
            HcclTask(thread_id=100, op_id=1, group_name="stale", stream_id=1, task_id=2, context_id=1, batch_id=1,
                     is_master=1, timestamp=300, duration=10, rank_size=8),
        ]
        check = HcclCalculator([], CONFIG)
        result = check._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

        # 按 op_id 聚拢后：op1 两个 task 连续合并为一个窗口，op2 独立
        self.assertEqual(2, len(result))
        self.assertEqual(1, result[0].connection_id)
        self.assertEqual(1, result[0].iter_id)
        self.assertEqual(100, result[0].start)
        self.assertEqual(310, result[0].end)
        self.assertEqual(2, result[1].connection_id)
        self.assertEqual(1, result[1].iter_id)
        self.assertEqual(200, result[1].start)
        self.assertEqual(210, result[1].end)

        # group_name 回填：task 侧 group_name 应被刷新为所属 op 的 group_name
        self.assertEqual("domain_a", hccl_tasks[0].group_name)
        self.assertEqual("domain_b", hccl_tasks[1].group_name)
        self.assertEqual("domain_a", hccl_tasks[2].group_name)

    # ============================================================
    # _generate_hccl_data
    # ============================================================

    def test_generate_hccl_data_should_serialize_task_to_24_columns(self):
        """_generate_hccl_data 将 HcclTask 序列化为 HCCLTaskSingleDeviceMap 的 24 列"""
        check = HcclCalculator([], CONFIG)
        hccl_ops = []
        hccl_tasks = [
            HcclTask(
                model_id=1, index_id=2, hccl_name="Memcpy", group_name="grp",
                plane_id=3, timestamp=100, duration=50,
                op_id=5, is_master=1, stream_id=6, task_id=7, context_id=8,
                batch_id=9, size=1024, bandwidth=10.5, local_rank=0,
                remote_rank=1, rank_size=8, transport_type="SDMA",
                data_type="INT8", link_type="LINK", rdma_type="RDMA",
                notify_id=99,
            ),
        ]
        check._generate_hccl_data(hccl_ops, hccl_tasks)

        self.assertEqual(1, len(check._hccl_task_data))
        row = check._hccl_task_data[0]
        self.assertEqual(24, len(row))
        self.assertEqual(
            [1, 2, "Memcpy", "grp", 3, 100, 50, 5, 1, 6, 7, 8, 9,
             1024, 10.5, 0, 1, 8, "SDMA", "INT8", "LINK", "RDMA", 99, 0],
            row,
        )

    def test_generate_hccl_data_should_serialize_op_to_16_columns(self):
        """_generate_hccl_data 将 HcclOps 序列化为 HCCLOpSingleDeviceMap 的 16 列"""
        check = HcclCalculator([], CONFIG)
        hccl_tasks = []
        hccl_ops = [
            HcclOps(
                model_id=1, index_id=2, op_name="allreduce", task_type="HCCL",
                op_type="all_reduce", start=100, end=200,
                relay=1, retry=0, data_type="FP16", alg_type="HD",
                count=8, group_name="group_a", connection_id=42, rank_size=4,
            ),
        ]
        check._generate_hccl_data(hccl_ops, hccl_tasks)

        self.assertEqual(1, len(check._hccl_op_data))
        row = check._hccl_op_data[0]
        self.assertEqual(16, len(row))
        self.assertEqual(
            [1, 2, "allreduce", "HCCL", "all_reduce", 100, 200,
             1, 0, "FP16", "HD", 8, "group_a", 42, 4, 0],
            row,
        )

    # ============================================================
    # 带宽计算
    # ============================================================

    def test_calculate_bandwidth_gb_s_should_return_correct_bandwidth_in_GB_S(self):
        ret = HcclCalculator._calculate_bandwidth_gb_s(duration=319959.1875, size=209715200)
        ret_0_duration = HcclCalculator._calculate_bandwidth_gb_s(duration=0, size=666666)
        self.assertEqual(ret, 655.4435946615847)
        self.assertEqual(ret_0_duration, 0)

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

    def test_find_consecutive_payload_tasks_should_count(self):
        """find_consecutive_payload_tasks 统计连续 RDMA_SEND_PAYLOAD 任务数"""
        events = [
            [0, HcclTask(rdma_type='RDMA_SEND_PAYLOAD')],
            [1, HcclTask(rdma_type='RDMA_SEND_PAYLOAD')],
            [2, HcclTask(rdma_type='RDMA_SEND_PAYLOAD')],
            [3, HcclTask(rdma_type='OTHER')],
        ]
        count = HcclCalculator.find_consecutive_payload_tasks(events, 0)
        self.assertEqual(3, count)
        # 从非 PAYLOAD 位置开始应返回 0
        count_from_mid = HcclCalculator.find_consecutive_payload_tasks(events, 3)
        self.assertEqual(0, count_from_mid)

    # ============================================================
    # update_op_name_by_group_name
    # ============================================================

    def test_update_op_name_by_group_name_should_use_iter_id_suffix(self: any) -> None:
        """op_name 格式: {name}_{group[-3:]}_{index}_{iter_id}"""
        hccl_ops = [
            HcclOps(op_name="hcom_allreduce", group_name="12345", start=100, end=200, iter_id=1),
            HcclOps(op_name="hcom_allreduce", group_name="12345", start=300, end=400, iter_id=2),
        ]
        HcclCalculator.update_op_name_by_group_name(hccl_ops, start_time_raw_timestamp=50)
        self.assertEqual("hcom_allreduce_345_0_1", hccl_ops[0].op_name)
        self.assertEqual("hcom_allreduce_345_1_2", hccl_ops[1].op_name)

    def test_update_op_name_by_group_name_should_skip_ops_before_threshold(self: any) -> None:
        """end <= threshold 的 op 不递增 group 索引（保持 -1）"""
        hccl_ops = [
            HcclOps(op_name="op_a", group_name="12345", start=10, end=50, iter_id=1),
            HcclOps(op_name="op_a", group_name="12345", start=100, end=200, iter_id=1),
        ]
        HcclCalculator.update_op_name_by_group_name(hccl_ops, start_time_raw_timestamp=60)
        self.assertEqual("op_a_345_-1_1", hccl_ops[0].op_name)
        self.assertEqual("op_a_345_0_1", hccl_ops[1].op_name)

    # ============================================================
    # generate_op_report_data
    # ============================================================

    def test_generate_op_report_data_should_aggregate_by_op_type(self: any) -> None:
        """按 op_type 聚合 count / total / min / avg / max"""
        op1 = mock.Mock(op_type="all_reduce", start=0, end=10)
        op2 = mock.Mock(op_type="all_reduce", start=5, end=25)
        op3 = mock.Mock(op_type="all_gather", start=0, end=30)
        report_list = []
        HcclCalculator.generate_op_report_data([op1, op2, op3], report_list)
        self.assertEqual(2, len(report_list))
        self.assertEqual("all_reduce", report_list[0][0])
        self.assertEqual(2, report_list[0][1])
        self.assertEqual("all_gather", report_list[1][0])
        self.assertEqual(1, report_list[1][1])

    def test_generate_op_report_data_should_filter_ops_before_start_time(self: any) -> None:
        """end < start_time_raw_timestamp 的 op 不参与统计"""
        op_before = mock.Mock(op_type="old_op", start=0, end=50)
        op_after = mock.Mock(op_type="new_op", start=100, end=200)
        report_list = []
        HcclCalculator.generate_op_report_data([op_before, op_after], report_list, start_time_raw_timestamp=60)
        self.assertEqual(1, len(report_list))
        self.assertEqual("new_op", report_list[0][0])

    def test_generate_op_report_data_should_return_empty_when_all_filtered(self: any) -> None:
        """所有 op 都被过滤时 report_list 为空"""
        op1 = mock.Mock(op_type="old", start=0, end=10)
        op2 = mock.Mock(op_type="old2", start=0, end=0)
        report_list = []
        HcclCalculator.generate_op_report_data([op1, op2], report_list, start_time_raw_timestamp=100)
        self.assertEqual([], report_list)
