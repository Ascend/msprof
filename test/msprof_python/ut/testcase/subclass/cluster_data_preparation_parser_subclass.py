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
from common_func.constant import Constant
from msmodel.ai_cpu.data_preparation_view_model import DataPreparationViewModel
from msparser.cluster.cluster_data_preparation_parser import ClusterDataPreparationParser


class ClusterDataPreparationParserSubclass(ClusterDataPreparationParser):
    def __init__(self: any, params: dict) -> None:
        super().__init__(params)
        self._host_queue_mode = Constant.DEFAULT_INVALID_VALUE
        self._data = {}
        self._model = None
        self._host_queue_step_count = 0

    def calculate_queue_data(self: any, queue_list: list) -> None:
        super()._calculate_queue_data(queue_list)

    def calculate(self: any) -> None:
        super()._calculate()

    def query_host_queue(self: any) -> None:
        super()._query_host_queue()

    def check_device_path_valid(self: any) -> bool:
        return super()._check_device_path_valid()

    def query_data_queue(self: any) -> None:
        super()._query_data_queue()

    def get_data_queue_data(self: any) -> list:
        return super()._get_data_queue_data()

    def storage_data(self: any) -> None:
        super()._storage_data()

    def get_cluster_path(self: any, file_name: str) -> str:
        return super()._get_cluster_path(file_name)

    def set_model(self: any, model: DataPreparationViewModel) -> None:
        self._model = model

    def set_data(self: any, data: dict) -> dict:
        self._data = data

    def get_data(self: any) -> dict:
        return self._data

    def set_host_queue_mode(self: any, value: int) -> None:
        self._host_queue_mode = value

    def set_host_queue_step_count(self: any, value: int) -> None:
        self._host_queue_step_count = value
