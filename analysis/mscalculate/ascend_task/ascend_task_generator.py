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

import logging
from collections import deque
from typing import List
from typing import Tuple
from typing import Union
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.profiling_scene import ProfilingScene
from common_func.step_trace_constant import StepTraceConstant
from mscalculate.ascend_task.ascend_task import DeviceTask, TaskTypeDto
from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.device_task_collector import DeviceTaskCollector
from mscalculate.ascend_task.host_task_collector import HostTaskCollector
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.flip.flip_calculator import FlipCalculator
from msmodel.step_trace.ts_track_model import TsTrackModel


class AscendTaskGenerator:
    EXCLUDE_HOST_TYPE = "PROFILER_TRACE_EX"

    def __init__(self, result_dir: str):
        self.result_dir = result_dir
        self.device_task_collector = DeviceTaskCollector(result_dir)
        self.host_task_collector = HostTaskCollector(result_dir)
        self.iter_id = -1
        self.top_down_task_tuple_type = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TopDownTask, [])

    @classmethod
    def _sep_task_by_stream_task_ctx(cls, tasks: Union[List[DeviceTask], List[HostTask]]) -> dict:
        ret = {}
        for task in tasks:
            ret.setdefault((task.stream_id, task.task_id, task.context_id), []).append(task)
        return ret

    @classmethod
    def _sep_task_by_stream_task_ctx_batch(cls, tasks: Union[List[DeviceTask], List[HostTask]]) -> dict:
        ret = {}
        for task in tasks:
            ret.setdefault((task.stream_id, task.task_id, task.context_id, task.batch_id), []).append(task)
        return ret

    def _get_step_trace_task_type_data(self) -> dict:
        ret = {}
        if ChipManager().is_chip_v3_3():
            with TsTrackModel(self.result_dir, DBNameConstant.DB_STEP_TRACE, []) as tsTrackModel:
                task_types = tsTrackModel.get_all_data(DBNameConstant.TABLE_TASK_TYPE, TaskTypeDto)
                # calc flip
                task_types = FlipCalculator.set_device_batch_id(task_types, self.result_dir)
                for task in task_types:
                    if StrConstant.TASK_TYPE_MAPPING[task.task_type] in StepTraceConstant.SCHEDULE_TASK_TYPE:
                        ret.setdefault((task.stream_id, task.task_id, task.batch_id), []).append(task)
        return ret

    @classmethod
    def _is_task_in_static_model(cls, host_task: HostTask) -> bool:
        return host_task.index_id == NumberConstant.STATIC_GRAPH_INDEX

    def run(self, model_id: int, iter_start: int, iter_end: int) -> List[TopDownTask]:
        """
        get top-down ascend tasks
        :return: all ascend tasks in model(model_id) within iter(iter_start, iter_end)
        """
        ascend_tasks = []
        if model_id == NumberConstant.INVALID_MODEL_ID:
            # 全导和按step导出
            ascend_tasks = self._get_all_ascend_tasks()
        else:
            # 按子图导出
            for iter_ in range(iter_start, iter_end + 1):
                self.iter_id = iter_
                ascend_tasks_in_iter = self._get_ascend_tasks_within_iter(model_id, iter_)
                ascend_tasks.extend(ascend_tasks_in_iter)

        ascend_tasks = sorted(ascend_tasks, key=lambda x: x.start_time)
        return ascend_tasks

    def _gen_top_down_task(self, dt: DeviceTask, ht: HostTask) -> TopDownTask:
        # wait time will calculate after this
        return self.top_down_task_tuple_type(
            ht.model_id,
            self.iter_id,
            ht.stream_id,
            ht.task_id,
            dt.context_id,
            ht.batch_id,
            dt.timestamp,
            dt.duration,
            ht.task_type,
            dt.task_type,
            ht.connection_id,
        )

    def _gen_top_down_task_by_device_task(self, dt: DeviceTask) -> TopDownTask:
        return self.top_down_task_tuple_type(
            Constant.GE_OP_MODEL_ID,
            self.iter_id,
            dt.stream_id,
            dt.task_id,
            dt.context_id,
            dt.batch_id,
            dt.timestamp,
            dt.duration,
            Constant.TASK_TYPE_UNKNOWN,
            dt.task_type,
            Constant.DEFAULT_INVALID_VALUE,
        )

    def _gen_top_down_task_by_host_task(self, ht: HostTask, task_type_map=None) -> TopDownTask:
        start, dur = -1, -1
        device_task_type = Constant.TASK_TYPE_UNKNOWN
        if task_type_map:
            task_data = task_type_map.get((ht.stream_id, ht.task_id, ht.batch_id))
            if task_data:
                # 查找开始和结束状态的时间戳
                start_obj = next((x for x in task_data if x.task_state == 0), None)
                end_obj = next((x for x in task_data if x.task_state == 2), None)

                if start_obj and end_obj:
                    start = InfoConfReader().time_from_syscnt(start_obj.timestamp)
                    dur = InfoConfReader().time_from_syscnt(end_obj.timestamp) - start
                    device_task_type = ht.task_type
                else:
                    logging.info("stream=[%s], task=[%s] uncompleted timestamp.", ht.stream_id, ht.task_id)

        return self.top_down_task_tuple_type(
            ht.model_id,
            self.iter_id,
            ht.stream_id,
            ht.task_id,
            ht.context_id,
            ht.batch_id,
            start,
            dur,
            ht.task_type,
            device_task_type,
            ht.connection_id,
        )

    def _match_host_device_task_in_static_model(
        self, host_task: HostTask, device_tasks: List[DeviceTask]
    ) -> List[TopDownTask]:
        logging.debug(
            "Found %d device tasks for stream_id %d and task_id %d in static model %d",
            len(device_tasks),
            host_task.stream_id,
            host_task.task_id,
            host_task.model_id,
        )
        return [self._gen_top_down_task(dt, host_task) for dt in device_tasks]

    def _exclude_profiling_trace_ex_host_task(self, host_tasks: List[HostTask]) -> List[HostTask]:
        return [host_task for host_task in host_tasks if host_task.task_type != self.EXCLUDE_HOST_TYPE]

    def _match_host_device_task(
        self, host_tasks: List[HostTask], device_tasks: List[DeviceTask]
    ) -> Tuple[List[TopDownTask], List[TopDownTask], List[TopDownTask]]:
        if len(host_tasks) == 1 and device_tasks and self._is_task_in_static_model(host_tasks[0]):
            # task in static model, there may be a one-to-many relationship between host tasks and device tasks.
            return self._match_host_device_task_in_static_model(host_tasks[0], device_tasks), [], []

        if host_tasks and device_tasks and len(host_tasks) != len(device_tasks):
            # notice: this will be removed since in normal host and device will both report profiling_trace_ex task
            host_tasks = self._exclude_profiling_trace_ex_host_task(host_tasks)

        host_queue = deque(host_tasks)
        device_queue = deque(device_tasks)
        # only when host and device task queue are all not empty then we match
        failed_match = host_queue and device_queue and len(host_queue) != len(device_queue)

        top_down_tasks = []
        pre_batch_id = -1
        if host_queue:
            pre_batch_id = host_queue[0].batch_id - 1
        while device_queue and host_queue:
            if host_queue[0].batch_id != pre_batch_id + 1:
                logging.error(
                    "lost host tasks for stream_id: %d, task_id: %d, batch_id: %d, ctx_id: %d."
                    " Tasks that use these IDs may have mismatches.",
                    host_queue[0].stream_id,
                    host_queue[0].task_id,
                    pre_batch_id + 1,
                    host_queue[0].context_id,
                )
            device_task = device_queue.popleft()
            host_task = host_queue.popleft()
            # when unique id takes effect, judge match or not by it
            top_down_t = self._gen_top_down_task(device_task, host_task)
            top_down_tasks.append(top_down_t)
            pre_batch_id = host_task.batch_id

        if host_queue:
            if failed_match:
                if ProfilingScene().is_step_export():
                    logging.error(
                        "device tasks less than host tasks for stream_id: %d, task_id: %d, ctx_id: %d."
                        " Tasks that use these IDs may have mismatches.",
                        host_queue[0].stream_id,
                        host_queue[0].task_id,
                        host_queue[0].context_id,
                    )
                else:
                    logging.warning(
                        "device tasks less than host tasks for stream_id: %d, task_id: %d, ctx_id: %d."
                        " Tasks that use these IDs may have mismatches.",
                        host_queue[0].stream_id,
                        host_queue[0].task_id,
                        host_queue[0].context_id,
                    )
            else:
                logging.debug(
                    "no device tasks found for stream_id: %d, task_id: %d, ctx_id: %d.",
                    host_queue[0].stream_id,
                    host_queue[0].task_id,
                    host_queue[0].context_id,
                )
        if device_queue:
            if failed_match:
                logging.error(
                    "host tasks less than device tasks for stream_id: %d, task_id: %d, ctx_id: %d."
                    " Tasks that use these IDs may have mismatches.",
                    device_queue[0].stream_id,
                    device_queue[0].task_id,
                    device_queue[0].context_id,
                )
            else:
                logging.debug(
                    "no host tasks found for stream_id: %d, task_id: %d, ctx_id: %d.",
                    device_queue[0].stream_id,
                    device_queue[0].task_id,
                    device_queue[0].context_id,
                )
        mismatch_device_tasks = [self._gen_top_down_task_by_device_task(data) for data in list(device_queue)]
        mismatch_host_tasks = [self._gen_top_down_task_by_host_task(data) for data in list(host_queue)]
        return top_down_tasks, mismatch_host_tasks, mismatch_device_tasks

    def _generate_top_down_tasks(
        self: any, host_tasks: List[HostTask], device_tasks: List[DeviceTask]
    ) -> List[TopDownTask]:
        """
        associate host and device task to generate up-down task which
        contains complete ascend software and hardware info.
        """
        stream_task_ctx_separated_host_tasks = self._sep_task_by_stream_task_ctx(host_tasks)
        stream_task_ctx_separated_device_tasks = self._sep_task_by_stream_task_ctx(device_tasks)

        top_down_tasks = []
        matched_top_down_task_num = 0
        stream_task_set = stream_task_ctx_separated_host_tasks.keys() | stream_task_ctx_separated_device_tasks.keys()
        for key in stream_task_set:
            host_t = stream_task_ctx_separated_host_tasks.get(key, [])
            device_t = stream_task_ctx_separated_device_tasks.get(key, [])
            matched_top_down_t, mismatch_host_t, mismatch_device_t = self._match_host_device_task(host_t, device_t)
            # statistic mismatch task, for future interface
            top_down_tasks.extend([*matched_top_down_t, *mismatch_host_t, *mismatch_device_t])
            matched_top_down_task_num += len(matched_top_down_t)

        logging.info(
            "Found %d host device matched task, %d top down task", matched_top_down_task_num, len(top_down_tasks)
        )
        return top_down_tasks

    def _generate_top_down_tasks_by_batch_id(
        self: any, host_tasks: List[HostTask], device_tasks: List[DeviceTask]
    ) -> List[TopDownTask]:
        """
        associate host and device task to generate top-down task which
        contains complete ascend software and hardware info.
        """
        host_tasks = self._sep_task_by_stream_task_ctx_batch(host_tasks)
        device_tasks = self._sep_task_by_stream_task_ctx_batch(device_tasks)
        task_types = self._get_step_trace_task_type_data()
        top_down_tasks = []
        matched_top_down_task_num = 0
        stream_task_batch_set = host_tasks.keys() | device_tasks.keys()
        for key in stream_task_batch_set:
            host_t = host_tasks.get(key, [])
            device_t = device_tasks.get(key, [])
            if len(host_t) == 1 and len(device_t) == 1:
                top_down_tasks.append(self._gen_top_down_task(device_t[0], host_t[0]))
                matched_top_down_task_num += 1
                continue
            if len(host_t) == 1 and device_t:
                # task in static model, there may be a one-to-many relationship between host tasks and device tasks.
                top_down_tasks.extend(self._match_host_device_task_in_static_model(host_t[0], device_t))
                matched_top_down_task_num += len(device_t)
                continue
            if not device_t:  # device task is empty
                top_down_tasks.extend([self._gen_top_down_task_by_host_task(task, task_types) for task in host_t])
                continue
            if not host_t:  # host task is empty
                top_down_tasks.extend([self._gen_top_down_task_by_device_task(task) for task in device_t])
                continue
            # host_t or device_t have more than one tasks, and both host_t and device_t are not empty
            logging.error(
                "%d host tasks and %d device mismatch tasks for stream_id: %d, task_id: %d, batch_id: %d, ctx_id: %d.",
                len(host_t),
                len(device_t),
                host_t[0].stream_id,
                host_t[0].task_id,
                host_t[0].batch_id,
                host_t[0].context_id,
            )
        logging.info(
            "Found %d host device matched task, %d top down task", matched_top_down_task_num, len(top_down_tasks)
        )
        return top_down_tasks

    def _get_all_ascend_tasks(self) -> List[TopDownTask]:
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []
        host_tasks = self.host_task_collector.get_host_tasks(int(device_id))
        device_tasks = self.device_task_collector.get_all_device_tasks()
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            top_down_tasks = self._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        else:
            top_down_tasks = self._generate_top_down_tasks(host_tasks, device_tasks)
        return top_down_tasks

    def _get_ascend_tasks_within_iter(self, model_id, iter_id) -> List[TopDownTask]:
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []
        host_tasks = self.host_task_collector.get_host_tasks_by_model_and_iter(model_id, iter_id, int(device_id))
        device_tasks = self.device_task_collector.get_device_tasks_by_model_and_iter(model_id, iter_id)
        if ChipManager().is_chip_all_data_export() and InfoConfReader().is_all_export_version():
            top_down_tasks = self._generate_top_down_tasks_by_batch_id(host_tasks, device_tasks)
        else:
            top_down_tasks = self._generate_top_down_tasks(host_tasks, device_tasks)
        return top_down_tasks
