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

# pylint: disable=duplicate-code

import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.cluster_parallel_analysis import ClusterParallelAnalysis


class ClusterParallelAnalysisParser:
    def __init__(self: any, params: dict) -> None:
        self._collection_path = params["collection_path"]
        self._params = params
        self._npu_id = params["npu_id"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        self._parallel_table_name = Constant.NA
        self.parallel_data_result = {}

    def process(self: any) -> None:
        self._prepare_parallel_analysis()
        self.parallel_data_result = ClusterParallelAnalysis(self._parallel_table_name, self._params).get_parallel_data()
        output_file_name = "cluster_parallel_analysis_{}_{}_{}.json".format(
            self._npu_id, self._model_id, self._iteration_id
        )
        FileManager.storage_query_result_json_file(self._collection_path, self.parallel_data_result, output_file_name)

    def _prepare_parallel_analysis(self: any) -> None:
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_PARALLEL)):
            raise ProfException(
                ProfException.PROF_CLUSTER_INVALID_DB, "Cannot find the cluster_parallel.db or Permission denied!"
            )
        npu_ids = []
        model_iteration_ids = {}
        with ClusterParallelViewModel(self._collection_path) as _model:
            self._parallel_table_name = _model.get_table_name()
            if self._parallel_table_name == Constant.NA:
                raise ProfException(
                    ProfException.PROF_CLUSTER_INVALID_DB,
                    "Cannot find the cluster parallel table or Permission denied!",
                )
            npu_ids = _model.get_npu_ids(self._parallel_table_name)
            model_iteration_ids = _model.get_model_iteration_ids(self._parallel_table_name)
        self._check_arguments_valid(npu_ids, model_iteration_ids)

    def _check_arguments_valid(self: any, npu_ids: list, model_iteration_ids: dict) -> None:
        if self._npu_id == Constant.DEFAULT_INVALID_VALUE:
            if self._model_id not in model_iteration_ids.keys():
                min_value = min(model_iteration_ids.keys())
                max_value = max(model_iteration_ids.keys())
                message = f"Invalid arguments! The argument '--model-id' should be between {min_value} and {max_value}."
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
            iteration_ids = model_iteration_ids.get(self._model_id, [])
            if self._iteration_id not in iteration_ids:
                min_value = min(iteration_ids)
                max_value = max(iteration_ids)
                message = (
                    f"Invalid arguments! The argument '--iteration-id' should be between {min_value} and {max_value}."
                )
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
            return
        if self._iteration_id == Constant.DEFAULT_INVALID_VALUE:
            if self._npu_id not in npu_ids:
                message = f"Invalid arguments! The argument '--id' should be on the list {str(npu_ids)}."
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
            if self._model_id not in model_iteration_ids.keys():
                min_value = min(model_iteration_ids.keys())
                max_value = max(model_iteration_ids.keys())
                message = f"Invalid arguments! The argument '--model-id' should be between {min_value} and {max_value}."
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
            return
        message = "Query arguments error! One of the arguments '--id' or '--iteration-id' must be -1."
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)
