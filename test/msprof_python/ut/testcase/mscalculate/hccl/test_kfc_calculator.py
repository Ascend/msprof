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
import unittest
from unittest import mock

from constant.constant import CONFIG
from common_func.hccl_info_common import DeviceHcclSource
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from common_func.info_conf_reader import InfoConfReader
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from mscalculate.hccl.hccl_task import KfcOps
from mscalculate.hccl.kfc_calculator import KfcCalculator
from common_func.msprof_object import CustomizedNamedtupleFactory
from msmodel.add_info.kfc_info_model import KfcInfoViewModel

NAMESPACE = 'mscalculate.hccl.kfc_calculator'

# KfcOps 在 get_kfc_op_data() 返回前被 DBManager.fetch_all_data 转换为增强型 namedtuple，
# 该 namedtuple 通过 enhance_namedtuple 将 _replace 重命名为 replace。
# 测试中需要模拟相同类型，否则 .replace() 调用会失败。
_KFC_OP_SQL_DESC = [
    ("model_id",), ("index_id",), ("stream_id",), ("task_id",),
    ("context_id",), ("batch_id",), ("start",), ("end",),
    ("kfc_connection_id",), ("op_name",),
]
_KFC_OP_NT = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(KfcOps, _KFC_OP_SQL_DESC)


def _make_kfc_op(**kwargs: any) -> any:
    """创建增强型 namedtuple KfcOps（模拟 get_kfc_op_data 的实际返回类型），具备 replace() 方法"""
    defaults = KfcOps()
    args = []
    for field_name in _KFC_OP_NT._fields:
        val = kwargs.get(field_name)
        if val is None:
            val = getattr(defaults, field_name)
        args.append(val)
    return _KFC_OP_NT(*args)


class TestKfcCalculator(unittest.TestCase):
    def test_ms_run_should_return_when_no_kfc_info_db(self: any) -> None:
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        check = KfcCalculator([], CONFIG)
        check.ms_run()
        InfoConfReader()._start_info.clear()

    def test_ms_run_should_drop_table_when_has_kfc_info_db_and_calculate_again(self: any) -> None:
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        with mock.patch("os.path.exists", return_value=True), \
                mock.patch(NAMESPACE + ".KfcCalculator._judge_calculate_again", return_value=True), \
                mock.patch(NAMESPACE + ".KfcCalculator.calculate"):
            check = KfcCalculator([], CONFIG)
            check.ms_run()
        InfoConfReader()._start_info.clear()

    def test_judge_calculate_again_should_return_true_when_not_all_export(self: any) -> None:
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        check = KfcCalculator([], CONFIG)
        check._judge_calculate_again()
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        InfoConfReader()._start_info.clear()

    def test_judge_calculate_again_should_return_true_when_all_export_and_not_have_kfc_op_table(self: any) -> None:
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=False):
            check = KfcCalculator([], CONFIG)
            self.assertTrue(check._judge_calculate_again())
        InfoConfReader()._start_info.clear()

    def test_judge_calculate_again_should_return_false_when_all_export_and_have_kfc_op_table(self: any) -> None:
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)
        with mock.patch(NAMESPACE + ".DBManager.check_tables_in_db", return_value=True):
            check = KfcCalculator([], CONFIG)
            self.assertFalse(check._judge_calculate_again())
        InfoConfReader()._start_info.clear()

    # ============================================================
    # 新增 UT — 当前修改引入的方法
    # ============================================================

    def test_make_kernel_key_should_return_5_tuple_with_iter_id(self: any) -> None:
        """_make_kernel_key 返回含 iter_id 的5元组"""
        kernel = KfcOps(stream_id=1, task_id=2, context_id=3, batch_id=4, iter_id=5)
        result = KfcCalculator._make_kernel_key(kernel)
        self.assertEqual((1, 2, 3, 4, 5), result)

    def test_make_kernel_key_should_return_default_iter_id_for_new_kernel(self: any) -> None:
        """iter_id 使用 dataclass 默认值 0 时，key 包含 0"""
        kernel = KfcOps(stream_id=10, task_id=20, context_id=30, batch_id=40)
        result = KfcCalculator._make_kernel_key(kernel)
        self.assertEqual((10, 20, 30, 40, 0), result)

    def test_serialize_kfc_op_should_return_16_elements(self: any) -> None:
        """_serialize_kfc_op 序列化为16列，source 固定为 MC2"""
        kernel = KfcOps(
            model_id=1, index_id=2, op_name="test_op", start=100, end=200,
            group_name="group1", kfc_connection_id=42, op_type="all_reduce",
            relay=1, retry=0, data_type="FP16", alg_type="HD", count=8, rank_size=4,
        )
        result = KfcCalculator._serialize_kfc_op(kernel)
        self.assertEqual(16, len(result))
        self.assertEqual(
            [1, 2, "test_op", 100, 200, "group1", 42, "all_reduce", 1, 0, "FP16", "HD", 8, 4, 0, DeviceHcclSource.MC2.value],
            result,
        )

    def test_serialize_kfc_task_should_return_25_elements_with_source_last(self: any) -> None:
        """_serialize_kfc_task 序列化为25列，最后一列为 source"""
        task = HcclTask(
            model_id=0, index_id=1, hccl_name="Memcpy", group_name="g",
            plane_id=2, timestamp=100, duration=50, op_id=4,
            is_master=1, stream_id=5, task_id=6, context_id=7, batch_id=8,
            size=1024, bandwidth=10.5, local_rank=0, remote_rank=1, rank_size=8,
            transport_type="SDMA", data_type="INT8", link_type="LINK",
            rdma_type="RDMA", notify_id="99",
        )
        result = KfcCalculator._serialize_kfc_task(task, DeviceHcclSource.HCCL.value)
        self.assertEqual(25, len(result))
        self.assertEqual(DeviceHcclSource.HCCL.value, result[-1])

    def test_group_kernels_by_name_should_group_and_sort_by_start(self: any) -> None:
        """_group_kernels_by_name 按 group_name 分组，组内按 start 升序"""
        k1 = KfcOps(group_name="g1", start=300)
        k2 = KfcOps(group_name="g1", start=100)
        k3 = KfcOps(group_name="g2", start=200)
        result = KfcCalculator._group_kernels_by_name([k1, k2, k3])
        self.assertEqual({"g1", "g2"}, set(result.keys()))
        self.assertEqual([100, 300], [k.start for k in result["g1"]])
        self.assertEqual([200], [k.start for k in result["g2"]])

    def test_group_tasks_by_comm_stream_should_group_and_exclude_non_comm_stream(self: any) -> None:
        """_group_tasks_by_comm_stream 按 comm_stream_id → {group_name} 表分组，非 comm 流的 task 被排除"""
        t1 = HcclTask(stream_id=1)
        t2 = HcclTask(stream_id=1)
        t3 = HcclTask(stream_id=2)
        t4 = HcclTask(stream_id=99)  # 非 comm 流，应被排除
        comm_stream_id_group_table = {1: {"g1"}, 2: {"g2"}}
        result = KfcCalculator._group_tasks_by_comm_stream([t1, t2, t3, t4], comm_stream_id_group_table)
        self.assertEqual(2, len(result["g1"]))
        self.assertEqual(1, len(result["g2"]))
        self.assertEqual(["g1", "g2"], sorted(result.keys()))

    def test_group_tasks_by_comm_stream_should_append_task_to_each_group_of_stream(self: any) -> None:
        """一个 comm 流可归属多个 group_name，task 会被加入每个 group"""
        t1 = HcclTask(stream_id=1)
        comm_stream_id_group_table = {1: {"g1", "g2"}}
        result = KfcCalculator._group_tasks_by_comm_stream([t1], comm_stream_id_group_table)
        self.assertEqual(1, len(result["g1"]))
        self.assertEqual(1, len(result["g2"]))

    def test_get_hccl_and_mc2_op_should_assign_iter_id_from_1(self: any) -> None:
        """iter_id 从 1 开始，同一四元组首个 kernel 为 1"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        k1 = _make_kfc_op(stream_id=10, task_id=20, context_id=30, batch_id=40)
        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm, \
                mock.patch(NAMESPACE + ".KfcCalculator.get_kfc_host_hccl_op", return_value={}):
            mock_vm.return_value.__enter__.return_value.get_kfc_op_data.return_value = [k1]
            check = KfcCalculator([], CONFIG)
            hccl_kernels, mc2_kernels = check.get_hccl_and_mc2_op()
            self.assertEqual(0, len(hccl_kernels))
            self.assertEqual(1, len(mc2_kernels))
            self.assertEqual(1, mc2_kernels[0].iter_id)
        InfoConfReader()._start_info.clear()

    def test_get_hccl_and_mc2_op_should_increment_iter_id_for_duplicate_4tuple(self: any) -> None:
        """同一四元组多次出现，iter_id 依次为 1, 2, 3"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        key = (10, 20, 30, 40)
        k1 = _make_kfc_op(stream_id=key[0], task_id=key[1], context_id=key[2], batch_id=key[3])
        k2 = _make_kfc_op(stream_id=key[0], task_id=key[1], context_id=key[2], batch_id=key[3])
        k3 = _make_kfc_op(stream_id=key[0], task_id=key[1], context_id=key[2], batch_id=key[3])
        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm, \
                mock.patch(NAMESPACE + ".KfcCalculator.get_kfc_host_hccl_op", return_value={}):
            mock_vm.return_value.__enter__.return_value.get_kfc_op_data.return_value = [k1, k2, k3]
            check = KfcCalculator([], CONFIG)
            _, mc2_kernels = check.get_hccl_and_mc2_op()
            self.assertEqual([1, 2, 3], [k.iter_id for k in mc2_kernels])
        InfoConfReader()._start_info.clear()

    def test_get_hccl_and_mc2_op_should_assign_independent_iter_id_per_4tuple(self: any) -> None:
        """不同四元组各自独立计数"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        k1 = _make_kfc_op(stream_id=10, task_id=20, context_id=30, batch_id=40)
        k2 = _make_kfc_op(stream_id=11, task_id=21, context_id=31, batch_id=41)
        k3 = _make_kfc_op(stream_id=10, task_id=20, context_id=30, batch_id=40)
        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm, \
                mock.patch(NAMESPACE + ".KfcCalculator.get_kfc_host_hccl_op", return_value={}):
            mock_vm.return_value.__enter__.return_value.get_kfc_op_data.return_value = [k1, k2, k3]
            check = KfcCalculator([], CONFIG)
            _, mc2_kernels = check.get_hccl_and_mc2_op()
            self.assertEqual([1, 1, 2], [k.iter_id for k in mc2_kernels])
        InfoConfReader()._start_info.clear()

    def test_get_hccl_and_mc2_op_should_set_connection_id_from_matched_hccl_op(self: any) -> None:
        """匹配到 HCCL_OP 时，op 侧 connection_id 取 hccl_op.connection_id"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        k1 = _make_kfc_op(stream_id=10, task_id=20, context_id=30, batch_id=40, kfc_connection_id=42)
        hccl_op = HcclOps(connection_id=99, op_name="all_reduce", group_name="g1")
        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm, \
                mock.patch(NAMESPACE + ".KfcCalculator.get_kfc_host_hccl_op", return_value={42: hccl_op}):
            mock_vm.return_value.__enter__.return_value.get_kfc_op_data.return_value = [k1]
            check = KfcCalculator([], CONFIG)
            hccl_kernels, mc2_kernels = check.get_hccl_and_mc2_op()
            self.assertEqual(1, len(hccl_kernels))
            self.assertEqual(0, len(mc2_kernels))
            self.assertEqual(99, hccl_kernels[0].connection_id)
            self.assertEqual("all_reduce", hccl_kernels[0].op_name)
            self.assertEqual("g1", hccl_kernels[0].group_name)
        InfoConfReader()._start_info.clear()

    def test_assign_op_and_plane_should_set_task_op_id_from_kernel_connection_id(self: any) -> None:
        """task 侧 op_id 取 op 的 connection_id（关联语义），落在 op 时间范围内标记 is_master"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}
        hccl_task_tuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(HcclTask, [])
        kernel = KfcOps(group_name="g1", start=100, end=200, connection_id=42,
                        model_id=1, index_id=2, iter_id=3, op_name="all_reduce")
        task = hccl_task_tuple(group_name="g1", timestamp=150, duration=10, stream_id=7)
        grouped_tasks = {"g1": [task]}
        check = KfcCalculator([], CONFIG)
        check._assign_op_and_plane(grouped_tasks, {"g1": [kernel]})

        updated = grouped_tasks["g1"][0]
        self.assertEqual(42, updated.op_id)
        self.assertEqual("all_reduce", updated.op_name)
        self.assertEqual("g1", updated.group_name)
        self.assertEqual(1, updated.is_master)
        self.assertEqual(0, updated.plane_id)
        InfoConfReader()._start_info.clear()

    def test_refine_kernel_times_should_use_iter_id_to_distinguish_executions(self: any) -> None:
        """_refine_kernel_times_with_master_stream 用 iter_id 区分多次执行的 start/end"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}

        # 使用 _make_kfc_op 创建增强型 namedtuple（生产代码中 get_hccl_and_mc2_op 返回此类型）
        hccl_kernels = [
            _make_kfc_op(stream_id=19, task_id=0, context_id=4294967295, batch_id=0, iter_id=1),
            _make_kfc_op(stream_id=19, task_id=0, context_id=4294967295, batch_id=0, iter_id=2),
        ]

        # context_id 必须匹配 NumberConstant.DEFAULT_GE_CONTEXT_ID (4294967295),
        # 因为 _refine_kernel_times_with_master_stream 内部使用该常量构造 uid 进行 lookup
        kfc_task_data = [
            HcclTask(stream_id=52, task_id=0, duration=100, timestamp=1000, context_id=4294967295, batch_id=0),
            HcclTask(stream_id=52, task_id=2, duration=100, timestamp=2000, context_id=4294967295, batch_id=0),
        ]

        # master_stream: 两次执行，分别对应 iter_id=1 和 iter_id=2
        master_stream_data = [
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=1, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=0, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.FIRST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=2, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=2, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.LAST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=3, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=0, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.FIRST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=4, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=2, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.LAST_TASK_TYPE,
            ),
        ]

        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm:
            mock_vm.return_value.__enter__.return_value.get_aicpu_master_stream_hccl_task.return_value = master_stream_data
            check = KfcCalculator([], CONFIG)
            check._refine_kernel_times_with_master_stream(hccl_kernels, kfc_task_data)

        # 第一次执行 (iter_id=1): start 来自 task(52,0), end 来自 task(52,2)
        self.assertEqual(1000, hccl_kernels[0].start)
        self.assertEqual(2100, hccl_kernels[0].end)
        # 第二次执行 (iter_id=2): 本例 kfc_task_data 每个 uid 仅一条，两轮都命中同一条，时间一致
        self.assertEqual(1000, hccl_kernels[1].start)
        self.assertEqual(2100, hccl_kernels[1].end)

        InfoConfReader()._start_info.clear()

    def test_refine_kernel_times_should_use_own_round_when_same_uid_repeated(self: any) -> None:
        """同一 uid 多次执行（重复执行）时，各轮 kernel 取对应轮次的小 task 时间，不被最后一轮覆盖"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}

        # 两轮执行，同一 uid 重复出现
        hccl_kernels = [
            _make_kfc_op(stream_id=19, task_id=0, context_id=4294967295, batch_id=0, iter_id=1),
            _make_kfc_op(stream_id=19, task_id=0, context_id=4294967295, batch_id=0, iter_id=2),
        ]

        # FIRST uid(52,0) 两次执行、LAST uid(52,2) 两次执行，时间不同（乱序验证按 timestamp 排序）
        kfc_task_data = [
            HcclTask(stream_id=52, task_id=0, duration=100, timestamp=1000, context_id=4294967295, batch_id=0),
            HcclTask(stream_id=52, task_id=2, duration=100, timestamp=3000, context_id=4294967295, batch_id=0),
            HcclTask(stream_id=52, task_id=0, duration=100, timestamp=2000, context_id=4294967295, batch_id=0),
            HcclTask(stream_id=52, task_id=2, duration=100, timestamp=4000, context_id=4294967295, batch_id=0),
        ]

        # 两轮执行，按 timestamp 顺序：第一轮 F/L、第二轮 F/L
        master_stream_data = [
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=1, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=0, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.FIRST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=2, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=2, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.LAST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=3, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=0, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.FIRST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=4, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=2, aicpu_batch_id=0, batch_id=0,
                task_type=KfcCalculator.LAST_TASK_TYPE,
            ),
        ]

        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm:
            mock_vm.return_value.__enter__.return_value.get_aicpu_master_stream_hccl_task.return_value = master_stream_data
            check = KfcCalculator([], CONFIG)
            check._refine_kernel_times_with_master_stream(hccl_kernels, kfc_task_data)

        # 第一轮: start=FIRST uid 第 1 次(1000), end=LAST uid 第 1 次(3000+100=3100)
        self.assertEqual(1000, hccl_kernels[0].start)
        self.assertEqual(3100, hccl_kernels[0].end)
        # 第二轮: start=FIRST uid 第 2 次(2000), end=LAST uid 第 2 次(4000+100=4100)
        self.assertEqual(2000, hccl_kernels[1].start)
        self.assertEqual(4100, hccl_kernels[1].end)

        InfoConfReader()._start_info.clear()

    def test_refine_kernel_times_should_match_aicpu_kernel_by_aicpu_batch_id(self: any) -> None:
        """aicpu_key 使用 aicpu_batch_id 匹配 kernel，而非展开算子的 batch_id"""
        InfoConfReader()._start_info = {"collectionTimeBegin": "9"}
        InfoConfReader()._end_info = {}

        # aicpu kernel 的 batch_id=7（aicpu_batch_id），与展开算子 batch_id=3 不同
        hccl_kernels = [
            _make_kfc_op(stream_id=19, task_id=0, context_id=4294967295, batch_id=7, iter_id=1),
        ]

        # kfc task（展开算子）的 batch_id=3，用于 uid 匹配
        kfc_task_data = [
            HcclTask(stream_id=52, task_id=0, duration=100, timestamp=1000, context_id=4294967295, batch_id=3),
            HcclTask(stream_id=52, task_id=2, duration=100, timestamp=2000, context_id=4294967295, batch_id=3),
        ]

        master_stream_data = [
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=1, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=0, aicpu_batch_id=7, batch_id=3,
                task_type=KfcCalculator.FIRST_TASK_TYPE,
            ),
            KfcInfoViewModel.MASTER_STREAM_HCCL_TASK_TYPE(
                timestamp=2, aicpu_stream_id=19, aicpu_task_id=0,
                stream_id=52, task_id=2, aicpu_batch_id=7, batch_id=3,
                task_type=KfcCalculator.LAST_TASK_TYPE,
            ),
        ]

        with mock.patch(NAMESPACE + ".KfcInfoViewModel") as mock_vm:
            mock_vm.return_value.__enter__.return_value.get_aicpu_master_stream_hccl_task.return_value = master_stream_data
            check = KfcCalculator([], CONFIG)
            check._refine_kernel_times_with_master_stream(hccl_kernels, kfc_task_data)

        # 命中 aicpu_batch_id=7 的 kernel，start/end 被修正
        self.assertEqual(1000, hccl_kernels[0].start)
        self.assertEqual(2100, hccl_kernels[0].end)

        InfoConfReader()._start_info.clear()
