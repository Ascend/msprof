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
import logging
from collections import defaultdict

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager
from common_func.hccl_info_common import DeviceHcclSource
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.hccl.hccl_calculator import HcclCalculator
from mscalculate.interface.icalculator import ICalculator
from msmodel.add_info.kfc_info_model import KfcInfoViewModel
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel
from msmodel.hccl.hccl_model import HCCLModel, HcclViewModel


class KfcCalculator(ICalculator, MsMultiProcess):
    FIRST_TASK_TYPE = 0
    LAST_TASK_TYPE = 1

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._kfc_op_data = []
        self._kfc_task_data = []
        self._mc2_op_report_data = []
        start_ts, _ = InfoConfReader().get_collect_time()
        self.start_time_raw_timestamp = InfoConfReader().trans_from_local_time_into_dev_raw_time(start_ts)

    def get_hccl_and_mc2_op(self: any) -> tuple:
        # 获取符合条件的aicpuKernel（SQL已按start_time排序）
        with KfcInfoViewModel(self._project_path, [DBNameConstant.TABLE_KFC_INFO]) as kfc_info_model:
            aicpu_kernel_list = kfc_info_model.get_kfc_op_data()

        # 按四元组出现次数标记iter_id（从1开始），区分同一kernel的多次执行（如task flip/重执行）
        counter = defaultdict(lambda: 0)
        for i, kfc_op in enumerate(aicpu_kernel_list):
            key = (kfc_op.stream_id, kfc_op.task_id, kfc_op.context_id, kfc_op.batch_id)
            counter[key] += 1
            aicpu_kernel_list[i] = kfc_op.replace(iter_id=counter[key])

        # 从HCCL_OP里取数据，以kfc_connectionId进行分组
        kfc_hccl_op_map = self.get_kfc_host_hccl_op()

        # 依照kfc_connection_id来判定是hccl还是mc2
        hccl_kernel_list = []
        mc2_kernel_list = []
        for kfc_op in aicpu_kernel_list:
            hccl_op = kfc_hccl_op_map.get(kfc_op.kfc_connection_id)
            if hccl_op:
                # 匹配到HCCL_OP，说明是HCCL
                hccl_kernel_list.append(
                    kfc_op.replace(
                        connection_id=hccl_op.connection_id, op_name=hccl_op.op_name, group_name=hccl_op.group_name
                    )
                )
            else:
                # 未匹配到，说明是MC2
                mc2_kernel_list.append(kfc_op.replace(connection_id=kfc_op.kfc_connection_id))
        return hccl_kernel_list, mc2_kernel_list

    def _refine_kernel_times_with_master_stream(self: any, hccl_kernels: list, kfc_task_data: list) -> None:
        """用mainStreamTask修正aicpu kernel的start/end，依赖kfcTask数据做匹配
        batchId已在parser阶段预计算并持久化到DB，无需重复计算
        iter_id用于区分同一aicpu kernel的多次执行（如task flip），确保每个副本的start/end独立
        """
        with KfcInfoViewModel(
            self._project_path, [DBNameConstant.TABLE_AICPU_MASTER_STREAM_HCCL_TASK]
        ) as kfc_info_model:
            master_stream_hccl_task = kfc_info_model.get_aicpu_master_stream_hccl_task()
        if not master_stream_hccl_task:
            return

        # 用kfcTask数据构建小task索引 (unique_id → task)
        hccl_small_task = {}
        for data in kfc_task_data:
            uid = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            hccl_small_task[uid] = data

        # 构建 kernel_key → [first_start, last_end]，key中加入iter_id区分多次执行
        kernel_times = {}
        aicpu_iter = defaultdict(lambda: 0)  # (aicpu_stream_id, aicpu_task_id, context_id, batch_id) → 当前执行次数
        mismatch = set()
        missing_first = set()  # 有 LAST 但无对应 FIRST 的异常 key
        for data in master_stream_hccl_task:
            if data.task_type not in (self.FIRST_TASK_TYPE, self.LAST_TASK_TYPE):
                continue
            uid = "{0}-{1}-{2}-{3}".format(
                data.stream_id, data.task_id, NumberConstant.DEFAULT_GE_CONTEXT_ID, data.batch_id
            )
            small_task = hccl_small_task.get(uid)
            if not small_task:
                mismatch.add(uid)
                continue
            aicpu_key = (
                data.aicpu_stream_id,
                data.aicpu_task_id,
                NumberConstant.DEFAULT_GE_CONTEXT_ID,
                data.aicpu_batch_id,
            )
            # FIRST表示新一轮执行开始，递增iter_id（从1开始）
            if data.task_type == self.FIRST_TASK_TYPE:
                aicpu_iter[aicpu_key] += 1
            iter_id = aicpu_iter[aicpu_key]
            key = (*aicpu_key, iter_id)
            task_end = small_task.timestamp + small_task.duration
            if data.task_type == self.FIRST_TASK_TYPE:
                kernel_times[key] = (small_task.timestamp, task_end)
            elif key not in kernel_times:
                # LAST 无对应 FIRST（如 FIRST 因 uid mismatch 被跳过），记录异常 key，不做时间修正
                missing_first.add(key)
            else:
                kernel_times[key] = (kernel_times[key][0], task_end)
        if mismatch:
            logging.error("Can not match any master task for these unique id: %s", mismatch)
        if missing_first:
            logging.error("LAST task has no matching FIRST task, abnormal keys: %s", missing_first)

        # 修正hccl_kernels的start/end，key中加入iter_id匹配对应次数的执行
        for i, kernel in enumerate(hccl_kernels):
            key = self._make_kernel_key(kernel)
            if key not in kernel_times:
                continue
            start, end = kernel_times[key]
            hccl_kernels[i] = kernel.replace(start=start, end=end)

    @staticmethod
    def _group_kernels_by_name(kernels: list) -> dict:
        """按group_name分组并排序kernels"""
        kb = defaultdict(list)
        for k in kernels:
            kb[k.group_name].append(k)
        for v in kb.values():
            v.sort(key=lambda k: k.start)
        return kb

    @staticmethod
    def _make_kernel_key(kernel) -> tuple:
        """构造 kernel 唯一标识(含iter_id以区分同一kernel的多次执行):
        (stream_id, task_id, context_id, batch_id, iter_id)
        """
        return (kernel.stream_id, kernel.task_id, kernel.context_id, kernel.batch_id, kernel.iter_id)

    def _assign_op_and_plane(self: any, grouped_tasks: dict, kernel_by_group: dict, plane_id_base: int = 0) -> dict:
        """双指针匹配op_id并分配plane_id，标记is_master（op时间范围内=1，外=0），原地修改tasks，返回kernel_times"""
        kernel_times: dict = {}  # _make_kernel_key → (min_start, max_end)
        for gname, tasks in grouped_tasks.items():
            kernels = kernel_by_group.get(gname, [])
            if not kernels:
                continue
            stream_plane: dict = {}
            op_index = 0
            op_len = len(kernels)

            for i, data in enumerate(tasks):
                while op_index < op_len and kernels[op_index].end < data.timestamp:
                    op_index += 1
                if op_index < op_len and kernels[op_index].start <= data.timestamp:
                    key = self._make_kernel_key(kernels[op_index])
                    # 用op的opId和groupName，并同步kernel的iter_id给task；
                    # 落在 op 时间范围内的 task 标记为主流
                    data = data.replace(
                        op_id=kernels[op_index].connection_id,
                        group_name=kernels[op_index].group_name,
                        model_id=kernels[op_index].model_id,
                        index_id=kernels[op_index].index_id,
                        iter_id=kernels[op_index].iter_id,
                        op_name=kernels[op_index].op_name,
                        is_master=1,
                    )
                    start = data.timestamp
                    end = data.timestamp + data.duration
                    prev = kernel_times.get(key)
                    if prev:
                        kernel_times[key] = (min(start, prev[0]), max(end, prev[1]))
                    else:
                        kernel_times[key] = (start, end)
                else:
                    # 时间范围外的 task 不算主流
                    data = data.replace(is_master=0)

                sid = data.stream_id
                if sid not in stream_plane:
                    stream_plane[sid] = len(stream_plane) + plane_id_base
                data = data.replace(plane_id=stream_plane[sid])
                tasks[i] = data

        # 仅用于mc2算子 op算子时间刷新（mc2无mainStreamTask）
        return kernel_times

    def generate_hccl_kernels(self, hccl_kernels: list, is_level0: bool) -> None:
        if is_level0 or not hccl_kernels:
            return

        # task 数据与 mc2 流程一致：按 mc2Info 展开流(comm_stream_ids)过滤，再按 stream→group 分组
        _, comm_stream_id_group_table = self.get_mc2_comm_info_data()
        comm_stream_ids = tuple(comm_stream_id_group_table.keys())
        if not comm_stream_ids:
            return

        with KfcInfoViewModel(self._project_path, [DBNameConstant.TABLE_KFC_INFO]) as model:
            kfc_task_data = model.get_kfc_info_with_task_by_stream_ids(comm_stream_ids)
        if not kfc_task_data:
            return

        self._refine_kernel_times_with_master_stream(hccl_kernels, kfc_task_data)

        kernel_by_group = self._group_kernels_by_name(hccl_kernels)
        grouped_tasks = self._group_tasks_by_comm_stream(kfc_task_data, comm_stream_id_group_table)
        self._assign_op_and_plane(grouped_tasks, kernel_by_group, plane_id_base=1)

        # op_id/iter_id 已在 _assign_op_and_plane 中赋值，逐组按 (op_id, iter_id) 计算带宽；
        # 主从流 task 都落库，仅按 op 时间剔除归属其他 op 的 task
        for gname, tasks in grouped_tasks.items():
            if gname in kernel_by_group:
                matched_tasks = self._filter_matched_tasks(tasks)
                HcclCalculator.update_bandwidth(matched_tasks)
                self._kfc_task_data.extend(
                    self._serialize_kfc_task(t, DeviceHcclSource.HCCL.value) for t in matched_tasks
                )

    @staticmethod
    def _group_tasks_by_comm_stream(task_data: list, comm_stream_id_group_table: dict) -> dict:
        """按 mc2Info 展开流信息(stream_id → {group_name}) 分组 task，非 comm 流的 task 自然被排除"""
        gt = defaultdict(list)
        for data in task_data:
            for gname in comm_stream_id_group_table.get(data.stream_id, set()):
                gt[gname].append(data)
        return gt

    @staticmethod
    def _filter_matched_tasks(tasks: list) -> list:
        """按 op 时间筛选：只保留匹配到某个 op 时间范围内的 task（op_id 已赋值）；
        op_id 仍为默认 -1 的 task 落在所有 op 时间窗之外（归属其他 op），不落库
        """
        return [t for t in tasks if t.op_id != Constant.DEFAULT_INVALID_VALUE]

    @staticmethod
    def _serialize_kfc_task(data: any, source: int) -> list:
        """序列化HcclTask namedtuple为KfcTaskMap的25列格式（对齐HCCLTaskSingleDeviceMap + source）"""
        return [
            data.model_id,
            data.index_id,
            data.hccl_name,
            data.group_name,
            data.plane_id,
            data.timestamp,
            data.duration,
            data.op_id,
            data.is_master,
            data.stream_id,
            data.task_id,
            data.context_id,
            data.batch_id,
            data.size,
            data.bandwidth,
            data.local_rank,
            data.remote_rank,
            data.rank_size,
            data.transport_type,
            data.data_type,
            data.link_type,
            data.rdma_type,
            data.notify_id,
            data.iter_id,
            source,
        ]

    @staticmethod
    def _serialize_kfc_op(kernel) -> list:
        """序列化MC2 kernel为KfcOPMap的15列格式:
        model_id, index_id, op_name, start, end, group_name,
        connection_id(kfc_connection_id), op_type, relay, retry, data_type, alg_type, count, rank_size, source
        """
        return [
            kernel.model_id,
            kernel.index_id,
            kernel.op_name,
            kernel.start,
            kernel.end,
            kernel.group_name,
            kernel.kfc_connection_id,
            kernel.op_type,
            kernel.relay,
            kernel.retry,
            kernel.data_type,
            kernel.alg_type,
            kernel.count,
            kernel.rank_size,
            kernel.iter_id,
            DeviceHcclSource.MC2.value,
        ]

    def generate_mc2_kernels(self, mc2_kernels: list, is_level0: bool) -> None:
        if not mc2_kernels:
            return

        aicpu_info, comm_stream_id_group_table = self.get_mc2_comm_info_data()

        for i, kernel in enumerate(mc2_kernels):
            info = aicpu_info.get(kernel.stream_id)
            if info:
                mc2_kernels[i] = kernel.replace(group_name=info[0], rank_size=info[1], op_type=kernel.op_name)
            else:
                mc2_kernels[i] = kernel.replace(op_type=kernel.op_name)

        comm_stream_ids = tuple(comm_stream_id_group_table.keys())
        if not comm_stream_ids:
            return

        with KfcInfoViewModel(self._project_path, [DBNameConstant.TABLE_KFC_INFO]) as model:
            if is_level0:
                comm_data = model.get_ascend_task_with_kfc_defaults(comm_stream_ids)
            else:
                comm_data = model.get_kfc_info_with_task_by_stream_ids(comm_stream_ids)
        if not comm_data:
            return

        grouped_tasks = self._group_tasks_by_comm_stream(comm_data, comm_stream_id_group_table)

        kernel_by_group = self._group_kernels_by_name(mc2_kernels)
        kernel_times = self._assign_op_and_plane(grouped_tasks, kernel_by_group, plane_id_base=0)

        # op_id/iter_id 已在 _assign_op_and_plane 中赋值，逐组按 (op_id, iter_id) 计算带宽；
        # 主从流 task 都落库，仅按 op 时间剔除归属其他 op 的 task
        for gname, tasks in grouped_tasks.items():
            if gname in kernel_by_group:
                matched_tasks = self._filter_matched_tasks(tasks)
                HcclCalculator.update_bandwidth(matched_tasks)
                self._kfc_task_data.extend(
                    self._serialize_kfc_task(t, DeviceHcclSource.MC2.value) for t in matched_tasks
                )

        # 用task首尾时间刷新kernel的start/end，更新op_name后序列化写入KfcOPMap
        for i, kernel in enumerate(mc2_kernels):
            times = kernel_times.get(self._make_kernel_key(kernel))
            if times:
                mc2_kernels[i] = kernel.replace(start=times[0], end=times[1])

        HcclCalculator.update_op_name_by_group_name(mc2_kernels, self.start_time_raw_timestamp)
        self._kfc_op_data = [self._serialize_kfc_op(k) for k in mc2_kernels]

    def calculate(self: any) -> None:
        is_level0 = InfoConfReader().is_level0()
        hccl_kernels, mc2_kernels = self.get_hccl_and_mc2_op()
        self.generate_hccl_kernels(hccl_kernels, is_level0)
        self.generate_mc2_kernels(mc2_kernels, is_level0)
        if is_level0:
            logging.warning("Profiling level is level0, no need to export statistics data.")
            return
        HcclCalculator.generate_op_report_data(mc2_kernels, self._mc2_op_report_data, self.start_time_raw_timestamp)

    def save(self: any) -> None:
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP]) as model:
            if self._kfc_op_data:
                model.flush(self._kfc_op_data, DBNameConstant.TABLE_KFC_OP)
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_TASK]) as model:
            if self._kfc_task_data:
                model.flush(self._kfc_task_data, DBNameConstant.TABLE_KFC_TASK)
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP_REPORT]) as model:
            if self._mc2_op_report_data:
                model.flush(self._mc2_op_report_data, DBNameConstant.TABLE_KFC_OP_REPORT)

    def get_mc2_comm_info_data(self: any) -> tuple:
        """返回:
        aicpu_info: {aicpu_stream_id: (group_name, rank_size)}
        comm_stream_id_group_table: {comm_stream_id: {group_names}}
        """
        with Mc2CommInfoViewModel(self._project_path, [DBNameConstant.TABLE_MC2_COMM_INFO]) as model:
            comm_info = model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        aicpu_info = {}  # aicpu_stream_id → (group_name, rank_size)
        comm_stream_id_group_table = {}
        for info in comm_info:
            aicpu_info[info.aicpu_kfc_stream_id] = (info.group_name, info.rank_size)
            try:
                comm_stream_list = list(map(int, info.comm_stream_ids.split(",")))
            except Exception:
                logging.error("The comm_stream_ids is not number, str is %s", info.comm_stream_ids)
                continue
            for stream_id in comm_stream_list:
                comm_stream_id_group_table.setdefault(stream_id, set()).add(info.group_name)
        return aicpu_info, comm_stream_id_group_table

    def get_kfc_host_hccl_op(self: any) -> dict:
        iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        with HcclViewModel(self._project_path, DBNameConstant.DB_HCCL, [DBNameConstant.TABLE_HCCL_OP]) as model:
            if not model.check_table():
                return dict()
            hccl_ops = model.get_hccl_ops(iter_range.model_id, iter_range.iteration_id)
        kfc_hccl_op_map = {}
        for data in hccl_ops:
            for kfc_connection_id in data.kfc_connection_ids.split(","):
                kfc_hccl_op_map[int(kfc_connection_id)] = data
        return kfc_hccl_op_map

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if (
            not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_MC2_COMM_INFO))
            or not self._judge_calculate_again()
        ):
            return
        self._drop_table()
        self.calculate()
        self.save()

    def _drop_table(self):
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP, DBNameConstant.TABLE_KFC_TASK]) as model:
            model.drop_table(DBNameConstant.TABLE_KFC_OP)
            model.drop_table(DBNameConstant.TABLE_KFC_TASK)
            model.drop_table(DBNameConstant.TABLE_KFC_OP_REPORT)

    def _judge_calculate_again(self):
        if not ProfilingScene().is_all_export():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_KFC_OP)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_KFC_OP) or DBManager.check_tables_in_db(
                hccl_db_path, DBNameConstant.TABLE_KFC_TASK
            ):
                logging.info(
                    "Found table %s  and %s in operator scene, no need to generate again",
                    DBNameConstant.TABLE_KFC_OP,
                    DBNameConstant.TABLE_KFC_TASK,
                )
                return False
            logging.info("No kfc table found, to generate it")
            return True
