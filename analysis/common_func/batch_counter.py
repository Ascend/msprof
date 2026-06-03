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

from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.iter_rec.iter_info_updater.iter_info_manager import IterInfoManager


class BatchCounter:
    """
    common function of calculating batch id
    """

    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"
    TASK_ID = "task_id"
    BATCH_ID = "batch_id"

    def __init__(self: any, project_path) -> None:
        self._project_path = project_path
        self._batch_list = []
        self._ge_static_shape_iter_model_dict = {}
        self._ge_static_shape_model_task_dict = {}
        self._iter_stream_max_value = {}
        self._ge_task_batch_dict = {}
        self._initialized_stream = {}
        self._is_parallel = False

    @staticmethod
    def id_to_int(stream_id: any, task_id: any) -> tuple:
        """
        transform stream id and task id to int type
        :param stream_id: index for stream id
        :param task_id: index for task id
        :return: stream_id, task_id
        """
        if is_number(stream_id) and is_number(task_id):
            return int(stream_id), int(task_id)
        return NumberConstant.INVALID_STREAM_ID, NumberConstant.INVALID_TASK_ID

    def init(self: any) -> None:
        """
        get dict whose key is iter_id, value is list of stream and task from ge info.
        :return: None
        """
        if IterInfoManager.check_parallel(self._project_path):
            self._is_parallel = True

        ge_info_model = GeInfoModel(self._project_path)
        if ge_info_model.check_db() and ge_info_model.check_table():
            self._ge_static_shape_iter_model_dict, self._ge_static_shape_model_task_dict = ge_info_model.get_ge_data(
                Constant.GE_STATIC_SHAPE
            )
            self._ge_task_batch_dict = ge_info_model.get_batch_dict()
        ge_info_model.finalize()

    def calculate_batch(self: any, stream_id: int, task_id: int, current_iter_id=NumberConstant.INVALID_ITER_ID) -> int:
        """
        calculate batch id for stream and task if the scene is not operator.
        :param stream_id: stream id
        :param task_id: task id
        :param current_iter_id: current iter id
        :return: batch id
        """
        if self._is_parallel:
            return NumberConstant.DEFAULT_BATCH_ID

        stream_id, task_id = BatchCounter.id_to_int(stream_id, task_id)

        iter_stream = (current_iter_id, stream_id)

        initial_batch_id = self.calibrate_initial_batch(iter_stream, task_id)
        stream_task_batch_value = self.STREAM_TASK_BATCH_KEY_FMT.format(stream_id, task_id, 0)

        # when scene is single op, ge op iter dict is empty
        model_id = self._ge_static_shape_iter_model_dict.get(current_iter_id)
        if model_id is not None and stream_task_batch_value in self._ge_static_shape_model_task_dict.get(
            model_id, set()
        ):
            batch_id = NumberConstant.DEFAULT_BATCH_ID
        else:
            batch_id = self.deal_batch_id_for_each_task(
                iter_stream=iter_stream,
                task_id=task_id,
                iter_stream_max_value=self._iter_stream_max_value,
                initial_batch_id=initial_batch_id,
            )
        self._batch_list.append(batch_id)
        return batch_id

    def calibrate_initial_batch(self: any, iter_stream: tuple, task_id: int) -> int:
        """
        calibrate initial batch
        :param stream_id: stream_id
        :param task_id: task_id
        :return: initial batch id
        """
        if iter_stream not in self._ge_task_batch_dict:
            return NumberConstant.DEFAULT_BATCH_ID
        if iter_stream in self._initialized_stream:
            return self._initialized_stream.get(iter_stream)

        initial_task_id = self._ge_task_batch_dict.get(iter_stream)[0]
        if task_id <= initial_task_id:
            initial_batch_id = self._ge_task_batch_dict.get(iter_stream)[1]
        else:
            initial_batch_id = self._ge_task_batch_dict.get(iter_stream)[1] - 1
        self._initialized_stream.setdefault(iter_stream, initial_batch_id)
        return initial_batch_id

    def deal_batch_id_for_each_task(
        self: any, iter_stream: tuple, task_id: int, iter_stream_max_value: dict, initial_batch_id: int
    ) -> int:
        """
        add batch id for each task
        :param iter_stream: index for (iter id, stream id)
        :param task_id: index for task id
        :param iter_stream_max_value: store stream, task, batch. Such as {0: {task_id:4, batch_id:2}}
        :param initial_batch_id: initial batch id
        :return: batch_id
        """
        if iter_stream not in iter_stream_max_value:
            iter_stream_max_value.setdefault(iter_stream, {self.TASK_ID: task_id, self.BATCH_ID: initial_batch_id})
        else:
            current_max_value = iter_stream_max_value.pop(iter_stream)
            if current_max_value.get(self.TASK_ID, -1) >= task_id:
                current_max_value[self.BATCH_ID] += 1
            current_max_value[self.TASK_ID] = task_id
            iter_stream_max_value.setdefault(iter_stream, current_max_value)
        return iter_stream_max_value.get(iter_stream).get(self.BATCH_ID)
