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

import json
import logging
import math
import os

from common_func.common import call_sys_exit
from common_func.common import error
from common_func.common import print_msg
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FdOpen
from common_func.file_manager import FileManager
from common_func.file_manager import check_path_valid
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.ai_cpu.data_preparation_view_model import DataPreparationViewModel
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msparser.aicpu.data_preparation_parser import DataPreparationParser


class ClusterDataPreparationParser:
    """
    Data preparation parser.
    """

    FILE_NAME = os.path.basename(__file__)
    QUERY_FILE_NAME = 'query'

    def __init__(self: any, params: dict) -> None:
        self._collection_path = params.get('collection_path', '')
        self._rank_id = params.get('npu_id', None)
        self._model_id = params.get("model_id", None)
        self._iteration_id = params.get("iteration_id", None)
        self._device_path = ''
        self._host_queue_mode = Constant.DEFAULT_INVALID_VALUE
        self._host_queue_step_count = 0
        self._model = None
        self._data = {}

    def process(self: any) -> None:
        """
        entrance for calculating data preparation
        :return: None or dict
        """
        if self._rank_id is None:
            error(self.FILE_NAME, "The query id is wrong. Please enter a valid value.")
            print_msg({"status": NumberConstant.ERROR, "info": "To query data queue, id is required", "data": ''})
            return
        if not (self._model_id is None and self._iteration_id is None):
            logging.warning("To query data queue, the parameters '--model-id' and '--iteration-id' are invalid.")
        try:
            self._calculate()
        except ProfException:
            print_msg(
                {
                    "status": NumberConstant.ERROR,
                    "info": "Some error occurred, please check input parameters and "
                    "ensure that necessary commands have been executed.",
                    "data": "",
                }
            )
            return
        try:
            self._storage_data()
        except ProfException:
            print_msg(
                {
                    "status": NumberConstant.ERROR,
                    "info": "Storing data failed,you may not have the permission to write files in the current path.",
                    "data": "",
                }
            )

    def _calculate_queue_data(self: any, queue_list: list) -> None:
        """
        calculate data queue
        :return: None
        """
        queue_list_length = len(queue_list)
        step_count = self._host_queue_step_count if self._host_queue_step_count else queue_list_length
        total_info = {"step_count": step_count, "empty_queue": 0, "total_time": 0, "avg_time": 0}
        if not step_count:
            return
        if queue_list_length % step_count != 0:
            logging.warning(
                "The data queue total length is not an integer multiple of the host queue data,"
                "maybe the collected data is incomplete."
            )
        multiple = math.ceil(queue_list_length / step_count)
        total_time = 0
        empty_queue = 0
        data_list = []
        step_index = 0
        for index in range(0, queue_list_length, multiple):
            queue_size = queue_list[index].queue_size
            duration = sum(item.duration for item in queue_list[index : min(index + multiple, queue_list_length)])
            total_time += duration
            if not queue_size:
                empty_queue += 1
            step_index += 1
            data_list.append({"step": step_index, "duration": duration, "queue_size": queue_size})
        total_info["empty_queue"] = empty_queue
        total_info["total_time"] = total_time
        total_info["avg_time"] = round(total_time / step_count, NumberConstant.ROUND_THREE_DECIMAL)
        self._data.setdefault("total_info", total_info)
        self._data.setdefault("data_list", data_list)

    def _calculate(self: any) -> None:
        """
        calculate data
        :return: None
        """
        try:
            check_path_valid(self._collection_path, False)
        except ProfException as err:
            if err.message:
                error(self.FILE_NAME, err)
            call_sys_exit(err.code)
        if not self._check_device_path_valid():
            raise ProfException(
                ProfException.PROF_INVALID_PARAM_ERROR, "Parameter settings are incorrect, please check input: --id."
            )
        if DataCheckManager.contain_info_json_data(self._device_path):
            if not os.path.exists(
                PathManager.get_db_path(self._device_path, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS)
            ):
                logging.error("The data preparation dataset file does not exist.")
                raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
            self._model = DataPreparationViewModel(self._device_path)
            self._query_host_queue()
            if self._host_queue_mode == Constant.DEFAULT_INVALID_VALUE:
                logging.warning("Failed to query host queue data.")
            elif self._host_queue_mode == DataPreparationParser.HOST_DATASET_NOT_SINK_MODE:
                # If mode is HOST_DATASET_NOT_SINK_MODE, data queue does not exist, no need to continue
                return
            self._query_data_queue()
        else:
            message = f"Invalid parsing dir(\"{self._device_path}\"), there is no PROF file in this path."
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message)

    def _query_host_queue(self: any) -> None:
        with self._model as model:
            if not model.check_db():
                return
            self._host_queue_mode = model.get_host_queue_mode()
            host_queue_data = model.get_host_queue()
        if not host_queue_data or self._host_queue_mode == Constant.DEFAULT_INVALID_VALUE:
            return
        self._host_queue_step_count = len(host_queue_data)
        if not self._host_queue_step_count:
            return
        host_total_info = {}
        host_data_list = []
        get_time = 0
        send_time = 0
        empty_queue_count = 0
        for data in host_queue_data:
            host_data_list.append(
                {
                    "step": data.index_id,
                    "get_time": data.get_time,
                    "send_time": data.send_time,
                    "total_time": data.total_time,
                    "queue_size": data.queue_size,
                }
            )
            get_time += data.get_time
            send_time += data.send_time
            if not data.queue_size:
                empty_queue_count += 1
        host_total_info.setdefault("step_count", self._host_queue_step_count)
        host_total_info.setdefault("empty_queue", empty_queue_count)
        host_total_info.setdefault(
            "avg_get_time", round(get_time / self._host_queue_step_count, NumberConstant.ROUND_THREE_DECIMAL)
        )
        host_total_info.setdefault(
            "avg_send_time", round(send_time / self._host_queue_step_count, NumberConstant.ROUND_THREE_DECIMAL)
        )
        host_total_info.setdefault(
            "avg_total_time",
            round((get_time + send_time) / self._host_queue_step_count, NumberConstant.ROUND_THREE_DECIMAL),
        )
        host_total_info.setdefault("mode", self._host_queue_mode)
        self._data.setdefault("host_total_info", host_total_info)
        self._data.setdefault("host_data_list", host_data_list)

    def _check_device_path_valid(self: any) -> bool:
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            return False
        with ClusterInfoViewModel(self._collection_path) as model:
            if not model.check_db():
                return False
            rank_data = model.get_info_based_on_rank_or_device_id(self._rank_id)
        if not rank_data:
            return False
        self._device_path = os.path.join(self._collection_path, rank_data.dir_name)
        return True

    def _query_data_queue(self: any) -> None:
        """
        query data queue
        :return: None
        """
        data_queue_data = self._get_data_queue_data()
        if not data_queue_data:
            message = (
                "Query data failed, maybe import command has not run successfully yet or "
                "import data preparation has no data, please check and run import command first."
            )
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message)
        self._calculate_queue_data(data_queue_data)

    def _get_data_queue_data(self: any) -> list:
        data_queue_data = []
        db_path = PathManager.get_db_path(self._device_path, DBNameConstant.DB_CLUSTER_DATA_PREPROCESS)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_DATA_QUEUE):
            return data_queue_data
        with self._model as _model:
            data_queue_data = _model.get_data_queue()
        return data_queue_data

    def _storage_data(self: any) -> None:
        """
        save data into file
        :return: None
        """
        file_name = 'data_preparation_{0}.json'.format(self._rank_id)
        file_path = self._get_cluster_path(file_name)
        if os.path.exists(file_path) and (not FileManager.remove_file(file_path)):
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR)
        try:
            with FdOpen(file_path) as _file:
                _file.write(json.dumps(self._data))
        except (OSError, SystemError, RuntimeError, TypeError) as err:
            message = "Storing data failed, you may not have the permission to write files in the current path."
            raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message) from err
        else:
            print_msg({"status": NumberConstant.SUCCESS, "info": "", "data": file_path})

    def _get_cluster_path(self: any, file_name: str) -> str:
        query_path = os.path.realpath(os.path.join(self._collection_path, self.QUERY_FILE_NAME))
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path, mode=NumberConstant.DIR_AUTHORITY)
            except OSError as err:
                message = "Storing data failed, you may not have the permission to write files in the current path."
                raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message) from err
        return os.path.realpath(os.path.join(query_path, file_name))
