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
# pylint: skip-file

import logging
import os
from collections import defaultdict
from collections import deque
from typing import List
from typing import Union
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from mscalculate.interface.icalculator import ICalculator
from msconfig.config_manager import ConfigManager
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
from msparser.cluster.meta_parser import HcclAnalysisTool


class HcclCalculator(ICalculator, MsMultiProcess):
    """
    Class to calculate hccl communication data and statistic data
    """

    TABLE_PATH = ConfigManager.TABLES

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = HcclViewModel(
            self._project_path,
            DBNameConstant.DB_HCCL_SINGLE_DEVICE,
            [
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
                DBNameConstant.TABLE_HCCL_OP_REPORT,
                DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE,
            ],
        )
        self._hccl_task_data = []
        self._hccl_op_data = []
        self._hccl_op_report_data = []
        start_ts, _ = InfoConfReader().get_collect_time()
        self.start_time_raw_timestamp = InfoConfReader().trans_from_local_time_into_dev_raw_time(start_ts)
        self.is_level0 = InfoConfReader().is_level0()

    @staticmethod
    def update_bandwidth(communication_data: List[HcclTask]):
        task_dict = defaultdict(lambda: defaultdict(list))
        idx = 0
        for task in communication_data:
            # op_name 已从 task 表移除，用 (op_id, iter_id) 精确区分每个算子实例
            task_dict[(task.op_id, task.iter_id)][task.plane_id].append([idx, task])
            idx += 1

        for op_key in task_dict:
            for planeid in task_dict[op_key]:
                planeid_tasks = task_dict[op_key][planeid]
                HcclCalculator.calc_bandwidth(planeid_tasks)
                for i, _ in enumerate(planeid_tasks):
                    communication_data[planeid_tasks[i][0]] = communication_data[planeid_tasks[i][0]].replace(
                        bandwidth=planeid_tasks[i][1].bandwidth
                    )

    @staticmethod
    def calc_bandwidth(communication_data: List[List[Union[int, HcclTask]]]):
        idx = 0
        if (
            'send' in communication_data[idx][1].op_name.lower()
            or 'receive' in communication_data[idx][1].op_name.lower()
        ):
            idx_jump = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        else:
            idx_jump = NumberConstant.RDMA_WITH_BARRIER_TASK_NUM
        for index, data in enumerate(communication_data):
            if data[1].rdma_type == 'RDMA_SEND_PAYLOAD':
                continue
            bandwidth = HcclCalculator._calculate_bandwidth_gb_s(data[1].duration, data[1].size)
            communication_data[index][1] = data[1].replace(bandwidth=bandwidth)
        while idx < len(communication_data):
            cur_task = communication_data[idx][1]
            if cur_task.rdma_type == 'RDMA_SEND_PAYLOAD':
                payload_cnt = HcclCalculator.find_consecutive_payload_tasks(communication_data, idx)
                rdma_send_payload_transit_result = HcclCalculator.calculate_consecutive_payload_tasks_info(
                    communication_data, idx, payload_cnt, idx_jump
                )
                if not rdma_send_payload_transit_result:
                    HcclCalculator.update_unclosed_rdma_task_bandwidth(idx, payload_cnt, communication_data)
                    idx += payload_cnt
                    continue
                payload_time = rdma_send_payload_transit_result[0] / NumberConstant.NS_TO_MS
                payload_size = rdma_send_payload_transit_result[1] / NumberConstant.COMMUNICATION_B_to_MB
                if payload_time:
                    payload_bandwidth = payload_size / payload_time
                else:
                    payload_bandwidth = 0
                for index in range(idx, idx + payload_cnt):
                    communication_data[index][1] = communication_data[index][1].replace(bandwidth=payload_bandwidth)
                idx += payload_cnt + idx_jump - 1
                continue
            idx += 1

    @staticmethod
    def update_unclosed_rdma_task_bandwidth(idx, payload_cnt, communication_data):
        for index in range(idx, idx + payload_cnt):
            bandwidth = HcclCalculator._calculate_bandwidth_gb_s(
                communication_data[index][1].duration, communication_data[index][1].size
            )
            communication_data[index][1] = communication_data[index][1].replace(bandwidth=bandwidth)

    @staticmethod
    def find_consecutive_payload_tasks(events, idx):
        count = 0
        while idx < len(events) and events[idx][1].rdma_type == 'RDMA_SEND_PAYLOAD':
            idx += 1
            count += 1
        return count

    @staticmethod
    def calculate_consecutive_payload_tasks_info(events, idx, payload_cnt, idx_jump):
        if (idx + payload_cnt + idx_jump - 2) >= len(events):
            op_name = events[idx][1].op_name
            logging.warning(
                "Bandwidth calculation abnormal. Index out of range, missing closure tasks. op_name:%s", op_name
            )
            return []
        saved_size = 0
        first_payload_time = events[idx][1].timestamp
        for i in range(idx, idx + payload_cnt):
            saved_size += events[i][1].size
        transit_time = HcclAnalysisTool.get_value(
            events[idx + payload_cnt + idx_jump - 2][1].duration
            + events[idx + payload_cnt + idx_jump - 2][1].timestamp
            - first_payload_time,
            'duration',
        )
        return [transit_time, saved_size]

    @staticmethod
    def _calculate_bandwidth_gb_s(duration, size):
        if abs(duration) < 1e-15:
            bandwidth = 0
        else:
            bandwidth = (size * NumberConstant.COMMUNICATION_B_to_GB) / (duration * NumberConstant.NS_TO_S)
        return bandwidth

    @staticmethod
    def update_op_name_by_group_name(hccl_ops: List[HcclOps], start_time_raw_timestamp: int) -> None:
        # 前面多线程数据处理 此处的task可能不保序 重新排序
        hccl_ops.sort(key=lambda x: (x.start, x.end))
        group_dict = defaultdict(lambda: -1)
        for i, op in enumerate(hccl_ops):
            if op.end > start_time_raw_timestamp:
                group_dict[op.group_name] += 1
            index = group_dict[op.group_name]
            hccl_ops[i] = op.replace(op_name=f"{op.op_name}_{op.group_name[-3:]}_{str(index)}_{str(op.iter_id)}")

    @staticmethod
    def generate_op_report_data(ops: list, report_list: list, start_time_raw_timestamp: int = 0) -> None:
        """按op_type聚合统计 op 数据，结果追加到 report_list
        ops需有 op_type, start, end 属性
        """
        op_type_group = defaultdict(lambda: {"count": 0, "total_time": 0, "min": float("inf"), "max": -float("inf")})
        for data in ops:
            if data.end < start_time_raw_timestamp:
                continue
            duration = data.end - data.start
            op_type_group[data.op_type]["count"] += 1
            op_type_group[data.op_type]["total_time"] += duration
            op_type_group[data.op_type]["min"] = min(op_type_group[data.op_type]["min"], duration)
            op_type_group[data.op_type]["max"] = max(op_type_group[data.op_type]["max"], duration)

        if not any(s["total_time"] for s in op_type_group.values()):
            return

        for status in op_type_group.values():
            status["avg"] = status["total_time"] / status["count"] if status["count"] else 0

        for op_type, status in op_type_group.items():
            report_list.append(
                [
                    op_type,
                    status["count"],
                    round(float(status["total_time"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(status["min"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(status["avg"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(status["max"]), NumberConstant.DECIMAL_ACCURACY),
                ]
            )
        report_list.sort(key=lambda x: x[2], reverse=True)

    def calculate(self: any) -> None:
        """
        calculate hccl communication data and hccl op report data
        """
        with self._model as hccl_model:
            if not DBManager.check_tables_in_db(
                PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL),
                DBNameConstant.TABLE_HCCL_OP,
                DBNameConstant.TABLE_HCCL_TASK,
            ):
                logging.warning("The HCCL table does not exist, so there is no need to continue associating operators.")
                return

            iter_range: IterationRange = self.sample_config.get(StrConstant.PARAM_ITER_ID)
            hccl_tasks = hccl_model.get_hccl_task_data()
            host_ops = hccl_model.get_hccl_ops(model_id=iter_range.model_id, index_id=iter_range.iteration_id)
            hccl_ops = self._merge_hccl_ops_and_tasks(host_ops, hccl_tasks)

        if not hccl_ops:
            logging.error("communication data is empty")
            return

        self.update_op_name_by_group_name(hccl_ops, self.start_time_raw_timestamp)
        self.update_bandwidth(hccl_tasks)

        self._generate_hccl_data(hccl_ops, hccl_tasks)
        if self.is_level0:
            logging.warning("Profiling level is level0, no need to export statistics data.")
            return
        self.generate_op_report_data(hccl_ops, self._hccl_op_report_data, self.start_time_raw_timestamp)

    def save(self: any) -> None:
        with self._model as hccl_model:
            if not self._hccl_task_data:
                return
            hccl_model.rebuild_hccl_task_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE, self._hccl_task_data)
            if not self._hccl_op_data:
                return
            hccl_model.rebuild_hccl_op_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE, self._hccl_op_data)
            if not self._hccl_op_report_data:
                return
            hccl_model.rebuild_hccl_op_report_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP_REPORT, self._hccl_op_report_data)

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)):
            return
        if not self._judge_calculate_again():
            return
        self._drop_table()
        self.calculate()
        self.save()

    def _drop_table(self):
        with self._model as hccl_model:
            hccl_model.drop_table(DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)
            hccl_model.drop_table(DBNameConstant.TABLE_HCCL_OP_REPORT)
            hccl_model.drop_table(DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE)

    def _judge_calculate_again(self):
        if not ProfilingScene().is_all_export():
            logging.info(
                "In graph scene, to generate table %s and %s",
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
                DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE,
            )
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            if DBManager.check_tables_in_db(
                hccl_db_path, DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE
            ) or DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
                logging.info(
                    "Found table %s  and %s in operator scene, no need to generate again",
                    DBNameConstant.TABLE_HCCL_OP_SINGLE_DEVICE,
                    DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
                )
                return False
            logging.info("No hccl table found, to generate it")
            return True

    def _generate_hccl_data(self, hccl_ops: List[HcclOps], hccl_tasks: List[HcclTask]) -> None:
        for task in hccl_tasks:
            self._hccl_task_data.append(
                [
                    task.model_id,
                    task.index_id,
                    task.hccl_name,
                    task.group_name,
                    task.plane_id,
                    task.timestamp,
                    task.duration,
                    task.op_id,
                    task.is_master,
                    task.stream_id,
                    task.task_id,
                    task.context_id,
                    task.batch_id,
                    task.size,
                    task.bandwidth,
                    task.local_rank,
                    task.remote_rank,
                    task.rank_size,
                    task.transport_type,
                    task.data_type,
                    task.link_type,
                    task.rdma_type,
                    task.notify_id,
                    task.iter_id,
                ]
            )

        for op in hccl_ops:
            self._hccl_op_data.append(
                [
                    op.model_id,
                    op.index_id,
                    op.op_name,
                    op.task_type,
                    op.op_type,
                    op.start,
                    op.end,
                    op.relay,
                    op.retry,
                    op.data_type,
                    op.alg_type,
                    op.count,
                    op.group_name,
                    op.connection_id,
                    op.rank_size,
                    op.iter_id,
                ]
            )

    def _merge_hccl_ops_and_tasks(self, hccl_ops: List[HcclOps], hccl_tasks: List[HcclTask]) -> List[HcclOps]:
        if not hccl_ops or not hccl_tasks:
            logging.error("No Hccl ops or Hccl tasks found")
            return []

        # 存 (原始索引, task)，便于原地回写 hccl_tasks 的 iter_id
        task_thread_map = defaultdict(lambda: deque())
        for idx, task in enumerate(hccl_tasks):
            task_thread_map[task.thread_id].append((idx, task))
        # 同一线程可能下发到多个 op，不同 op 的 task 时间戳会交错；
        # 先按 op_id 分组、组内按 timestamp 排序，保证同一 op 的 task 连续，避免按 op_id 切分窗口时
        # 把单个 op 错误拆成多次迭代。
        for thread_id in task_thread_map:
            task_thread_map[thread_id] = deque(
                sorted(task_thread_map[thread_id], key=lambda item: (item[1].op_id, item[1].timestamp))
            )

        op_thread_map = defaultdict(dict)
        for op in hccl_ops:
            op_thread_map[op.thread_id][op.connection_id] = op

        res = []
        mismatch_op_ids = set()

        for thread_id, task_queue in task_thread_map.items():
            if thread_id not in op_thread_map:
                logging.error("Op data can't match any task, thread id is %d.", thread_id)
                continue
            op_id_map = op_thread_map[thread_id]

            current_op_id = None
            current_iter_id = 1
            # 以四元组 (stream_id, task_id, context_id, batch_id) 作为 task 的唯一标识：
            # 同一静态 op 反复执行时四元组会重复，据此递增 iter_id；不同 op 的 task 四元组不同，不会误判，
            # 通信域交错的 task 仍能正确合并到各自 op。
            seen_task_ids = set()
            group_start = None
            group_end = None
            last_rank_size = None

            for idx, task in task_queue:
                if task.op_id not in op_id_map:
                    mismatch_op_ids.add(task.op_id)
                    continue

                task_key = (task.stream_id, task.task_id, task.context_id, task.batch_id)
                # 是否开始新一轮：op_id 变动，或同一 op 内 task 四元组重复（静态算子反复执行）
                new_round = (task.op_id != current_op_id) or (task_key in seen_task_ids)

                if new_round:
                    if current_op_id is not None:
                        op = op_id_map[current_op_id]
                        res.append(
                            op.replace(
                                start=group_start,
                                end=group_end,
                                rank_size=last_rank_size,
                                iter_id=current_iter_id,
                            )
                        )

                    if task.op_id != current_op_id:
                        current_op_id = task.op_id
                        current_iter_id = 1
                    else:
                        current_iter_id += 1
                    seen_task_ids.clear()
                    group_start = None
                    group_end = None
                    last_rank_size = None

                seen_task_ids.add(task_key)

                # 所有 task（含 non-master）统一打 iter_id，落盘时写入
                # 同时回填 op 的原始 op_name，供带宽计算判定 send/receive（op 名 ≠ task 的 hccl_name）；
                # task 侧 group_name 与 op 侧不同源，level0 下 task 侧残缺，统一用所属 op 的 group_name 刷新
                hccl_tasks[idx] = task.replace(
                    iter_id=current_iter_id,
                    op_name=op_id_map[current_op_id].op_name,
                    group_name=op_id_map[current_op_id].group_name,
                )

                # 再取主流：只 master 聚合 [start, end] 窗口
                if not task.is_master:
                    continue
                last_rank_size = task.rank_size
                if group_start is None:
                    group_start = task.timestamp
                    group_end = task.timestamp + task.duration
                else:
                    new_end = task.timestamp + task.duration
                    group_end = new_end if new_end > group_end else group_end

            if current_op_id is not None:
                op = op_id_map[current_op_id]
                res.append(
                    op.replace(
                        start=group_start,
                        end=group_end,
                        rank_size=last_rank_size,
                        iter_id=current_iter_id,
                    )
                )

        if mismatch_op_ids:
            logging.error("Some op_id can't match any task, op_id is %s", mismatch_op_ids)
        return res
