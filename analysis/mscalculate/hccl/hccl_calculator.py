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
from common_func.constant import Constant
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

    @staticmethod
    def update_bandwidth(communication_data: List[HcclTask]):
        task_dict = defaultdict(lambda: defaultdict(list))
        idx = 0
        for task in communication_data:
            task_dict[task.op_name][task.plane_id].append([idx, task])
            idx += 1

        for op_name in task_dict:
            for planeid in task_dict[op_name]:
                planeid_tasks = task_dict[op_name][planeid]
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
    def _cal_total(type_time: dict) -> int:
        """
        calculate total time for each device
        :param type_time: {"op_type":{'count': 0,'duration': 0, 'min': 0,'avg': 0,max': 0}}
        :return: total time
        """
        total_time = 0
        for ops in type_time.values():
            total_time += ops.get("total_time", 0)
        return total_time

    def update_op_name_by_group_name(self: any, communication_data: List[HcclTask]):
        group_dict = defaultdict(lambda: {"first_timestamp": 0, "count": -1})
        for num, data in enumerate(communication_data):
            # if data start in warmup, index will be set -1
            # else index++ when group_name and task_type in group_dict or group name set first
            task_type = StrConstant.AICPU_KERNEL if StrConstant.AICPU_KERNEL in data.op_name else StrConstant.NORMAL
            key = (data.group_name, task_type)
            if (
                data.timestamp > self.start_time_raw_timestamp
                and data.first_timestamp > group_dict[key]["first_timestamp"]
            ):
                group_dict[key]["first_timestamp"] = data.first_timestamp
                group_dict[key]["count"] += 1

            index = group_dict[key]["count"]
            communication_data[num] = data.replace(
                op_name=f"{data.op_name}_{data.group_name[-3:]}_{str(index)}_{str(data.iter_id)}"
            )

    def _get_hccl_op_report_data(self: any, communication_data: List[HcclTask]) -> any:
        """
        Calculate the hccl op report data by communication data
        return：{"op_type":{'count': 0,'duration': 0, min': 0,'avg': 0,max': 0}}
        """
        if not communication_data:
            return {}
        grouped_data = defaultdict(lambda: {"min_timestamp": float("inf"), "max_timestamp": -float("inf")})
        for data in communication_data:
            if data.is_master == 0 or data.timestamp < self.start_time_raw_timestamp:
                continue
            key = (data.op_name, data.first_timestamp, data.op_type)
            grouped_data[key]["min_timestamp"] = min(grouped_data[key]["min_timestamp"], data.timestamp)
            grouped_data[key]["max_timestamp"] = max(grouped_data[key]["max_timestamp"], data.timestamp + data.duration)
            grouped_data[key]["op_type"] = data.op_type
        for value in grouped_data.values():
            value["duration"] = value["max_timestamp"] - value["min_timestamp"]

        min_key = "min"
        max_key = "max"
        op_type_group = defaultdict(
            lambda: {"count": 0, "total_time": 0, min_key: float("inf"), max_key: -float("inf")}
        )
        for entry in grouped_data.values():
            op_type_status = op_type_group[entry["op_type"]]
            op_type_status["count"] += 1
            op_type_status["total_time"] += entry["duration"]
            op_type_status[min_key] = min(op_type_status[min_key], entry["duration"])
            op_type_status[max_key] = max(op_type_status[max_key], entry["duration"])
        for status in op_type_group.values():
            status["avg"] = 0
            if status["count"] != 0:
                status["avg"] = status["total_time"] / status["count"]
        return op_type_group

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
            hccl_ops = hccl_model.get_hccl_ops(model_id=iter_range.model_id, index_id=iter_range.iteration_id)
            communication_data = self._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

            if not communication_data:
                logging.error("communication data is empty")
                return

            # 前面多线程数据处理 此处的task可能不保序 重新排序
            communication_data.sort(key=lambda x: (x.host_timestamp, x.timestamp))

            self.update_op_name_by_group_name(communication_data)
            self.update_bandwidth(communication_data)
            is_hccl_op_type_valid = self._generate_hccl_op_info(communication_data)
            if is_hccl_op_type_valid:
                hccl_op_report_data = self._get_hccl_op_report_data(communication_data)
                self._create_report(hccl_op_report_data)
            else:
                logging.warning("No valid hccl op type exists, therefore not calculate hccl op report data")

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
                DBNameConstant.TABLE_HCCL_OP_REPORT,
            )
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE):
                logging.info(
                    "Found table %s in operator scene, no need to generate again",
                    DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
                )
                return False
            logging.info(
                "No table %s or %s found, to generate it",
                DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE,
                DBNameConstant.TABLE_HCCL_OP_REPORT,
            )
            return True

    def _generate_hccl_op_info(self, hccl_data: List[HcclTask]) -> bool:
        is_hccl_op_type_valid = False
        for data in hccl_data:
            self._hccl_task_data.append(
                [
                    data.model_id,
                    data.index_id,
                    data.op_name,
                    data.iteration,
                    data.hccl_name,
                    data.group_name,
                    data.first_timestamp,
                    data.plane_id,
                    data.timestamp,
                    data.duration,
                    data.is_dynamic,
                    data.task_type,
                    data.op_type,
                    data.connection_id,
                    data.is_master,
                    data.stream_id,
                    data.task_id,
                    data.duration_estimated,
                    data.local_rank,
                    data.remote_rank,
                    data.transport_type,
                    data.size,
                    data.data_type,
                    data.link_type,
                    data.bandwidth,
                    data.context_id,
                    data.notify_id,
                    data.batch_id,
                    data.rdma_type,
                    data.rank_size,
                ]
            )
            if data.op_type != Constant.NA:
                is_hccl_op_type_valid = True
        return is_hccl_op_type_valid

    def _create_report(self, hccl_op_report_data) -> None:
        """
        calculate report data
        :return: None
        """
        task_data = hccl_op_report_data
        if not task_data:
            return
        hccl_op_total_time = self._cal_total(task_data)
        total_data = []
        for op_type in task_data:
            statistic_data = task_data.get(op_type, {})
            if not statistic_data:
                continue
            if hccl_op_total_time != 0:
                task_duration_ratio = round(
                    float(statistic_data["total_time"] / hccl_op_total_time * 100), NumberConstant.DECIMAL_ACCURACY
                )
            else:
                task_duration_ratio = 0
            total_data.append(
                (
                    op_type,
                    statistic_data["count"],
                    round(float(statistic_data["total_time"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                    task_duration_ratio,
                )
            )
        if total_data:
            self._hccl_op_report_data = sorted(total_data, key=lambda x: x[5], reverse=True)
        else:
            logging.warning("There is no hccl op report data. Maybe an error occurs during the calculation")

    def _merge_hccl_ops_and_tasks(self, hccl_ops: List[HcclOps], hccl_tasks: List[HcclTask]) -> List[HcclTask]:
        def update_task_desc_with_hccl_op(op_desc: HcclOps, task_desc: any, times_for_hccl_op: int) -> HcclTask:
            group_name = op_desc.group_name if task_desc.group_name == Constant.NA else task_desc.group_name
            return task_desc.replace(
                op_name=op_desc.op_name,
                group_name=group_name,
                task_type=op_desc.task_type,
                op_type=op_desc.op_type,
                first_timestamp=op_desc.timestamp,
                iter_id=times_for_hccl_op,
                is_dynamic=op_desc.is_dynamic,
                model_id=op_desc.model_id,
                connection_id=op_desc.connection_id,
            )

        if not hccl_ops or not hccl_tasks:
            logging.error("No Hccl ops or Hccl tasks found")
            return []

        task_thread_map = defaultdict(lambda: deque())
        for task in hccl_tasks:
            task_thread_map[task.thread_id].append(task)

        op_thread_map = defaultdict(lambda: deque())
        for op in hccl_ops:
            op_thread_map[op.thread_id].append(op)

        idx = 0
        res = [None] * len(hccl_tasks)
        hccl_op_with_task_index = defaultdict(lambda: 0)
        # 取数的sql语句已经order by过了
        for thread_id, ops_queue in op_thread_map.items():
            if thread_id not in task_thread_map.keys():
                logging.error("Op data can't match any task, thread id is %d.", thread_id)
                continue
            task_queue = task_thread_map[thread_id]

            while ops_queue and task_queue:
                op = ops_queue.popleft()
                # check corner case: task time between last op end time and next op start time
                if task_queue and task_queue[0].host_timestamp < op.timestamp:
                    logging.error(
                        'Hccl Task (context_id=%d, stream_id=%d, task_id=%d) timestamp not in Ops time range',
                        task_queue[0].context_id,
                        task_queue[0].stream_id,
                        task_queue[0].task_id,
                    )

                while task_queue and task_queue[0].host_timestamp <= (op.timestamp + op.duration):
                    task = task_queue.popleft()
                    key = f'{task.stream_id}-{task.task_id}-{task.context_id}-{task.batch_id}'
                    hccl_op_with_task_index[key] += 1
                    res[idx] = update_task_desc_with_hccl_op(op, task, hccl_op_with_task_index[key])
                    idx += 1

                self._hccl_op_data.append(
                    [
                        op.model_id,
                        op.op_name,
                        op.task_type,
                        op.op_type,
                        op.timestamp,
                        op.relay,
                        op.retry,
                        op.data_type,
                        op.alg_type,
                        op.count,
                        op.group_name,
                        op.connection_id,
                        task.rank_size,
                    ]
                )

            if ops_queue:
                if ProfilingScene().is_step_export():
                    logging.warning('ops_queue is not empty (len=%d)', len(ops_queue))
                else:
                    logging.error('ops_queue is not empty (len=%d)', len(ops_queue))
            if task_queue:
                logging.error('task_queue is not empty (len=%d)', len(task_queue))

        return res[:idx]
