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
from typing import List
from typing import Union
from dataclasses import dataclass

from msparser.compact_info.task_track_bean import TaskTrackBean
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip
from mscalculate.ascend_task.ascend_task import DeviceTask
from common_func.db_name_constant import DBNameConstant
from msmodel.step_trace.ts_track_model import TsTrackModel


@dataclass
class InfDataHelper:
    timestamp = float("inf")


class FlipCalculator:
    """
    calculate batch id by flip number
    """

    STREAM_DESTROY_FLIP = 65535

    @staticmethod
    def compute_batch_id(
        task_data: List[Union[TaskTrackBean, DeviceTask]],
        flip_data: List[Union[TaskTrackBean, TaskFlip]],
        is_flip_num: bool = False,
    ) -> List:
        if not task_data:
            return []
        task_data_bin = FlipCalculator.sep_data_by_device_stream(task_data)
        flip_data = FlipCalculator.sep_data_by_device_stream(flip_data)
        new_task_data = [None] * len(task_data)
        new_task_index = 0
        for key, data in task_data_bin.items():
            flip_data_stream = flip_data.get(key, [])
            data.sort(key=lambda x: x.timestamp)
            flip_data_stream.sort(key=lambda x: x.timestamp)
            flip_data_stream.append(InfDataHelper())  # avoid overflow
            batch_id = 0
            task_index = 0
            flip_index = 0
            stream_destroy_num = 0
            while task_index < len(data):
                task = data[task_index]
                flip = flip_data_stream[flip_index]
                if task.timestamp >= flip.timestamp:
                    batch_id, stream_destroy_num = FlipCalculator.get_next_batch_id(
                        flip, batch_id, is_flip_num, stream_destroy_num
                    )
                    flip_index += 1
                    FlipCalculator.calibrate_when_flip_task_id_not_zero(
                        new_task_data, flip, new_task_index, batch_id, is_flip_num
                    )
                    continue
                if isinstance(task, tuple):
                    data[task_index] = task.replace(batch_id=batch_id)
                else:
                    task.batch_id = batch_id
                new_task_data[new_task_index] = data[task_index]
                task_index += 1  # next task
                new_task_index += 1
        return new_task_data

    @staticmethod
    def get_next_batch_id(
        flip: Union[TaskTrackBean, TaskFlip],
        batch_id: int,
        is_flip_num: bool,
        stream_destroy_num: int,
    ):
        if flip.flip_num == FlipCalculator.STREAM_DESTROY_FLIP:
            stream_destroy_num += 1
            batch_id += 1
            return batch_id, stream_destroy_num
        if is_flip_num:
            batch_id = flip.flip_num + stream_destroy_num
        else:
            batch_id += 1  # next flip
        return batch_id, stream_destroy_num

    @staticmethod
    def calibrate_when_flip_task_id_not_zero(
        task_data: List[Union[TaskTrackBean, DeviceTask]],
        flip: Union[TaskTrackBean, TaskFlip],
        task_index: int,
        batch_id: int,
        is_flip_num: int,
    ) -> None:
        if flip.flip_num == FlipCalculator.STREAM_DESTROY_FLIP or is_flip_num:  # do not calibrate when stream destroy
            return
        # Because tasks in multi-threads will apply for task id 0 simultaneously,
        # the flip may not get the task_id 0, we should search backward to calibrate the task
        # which task id is less than flip's task_id, and set these tasks the next batch id.
        task_index_backward = task_index - 1
        while (
            task_index_backward >= 0
            and task_data[task_index_backward].stream_id == flip.stream_id
            and task_data[task_index_backward].task_id < flip.task_id
        ):
            if isinstance(task_data[task_index_backward], tuple):
                task_data[task_index_backward] = task_data[task_index_backward].replace(batch_id=batch_id)
            else:
                task_data[task_index_backward].batch_id = batch_id
            task_index_backward -= 1

    @staticmethod
    def sep_data_by_device_stream(raw_data: List[Union[TaskTrackBean, DeviceTask, TaskFlip]]) -> dict:
        sep_data = {}
        for data in raw_data:
            # Host task has device_id,
            # but device task data has no device_id attribution because device data loaded is seperated by device_id
            key = "{}-{}".format(getattr(data, "device_id", 0), data.stream_id)
            sep_data.setdefault(key, []).append(data)
        return sep_data

    @staticmethod
    def set_device_batch_id(data: list, result_dir: str, is_flip_num: bool = False) -> List:
        with TsTrackModel(result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_DEVICE_TASK_FLIP]) as model:
            task_flip = model.get_task_flip_data()
        device_tasks = FlipCalculator.compute_batch_id(data, task_flip, is_flip_num)
        return device_tasks
