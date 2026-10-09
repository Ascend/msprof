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


class ClusterParallelAnalysisTuning:
    def __init__(self: any, params: dict) -> None:
        self._collection_path = params["collection_path"]
        self._params = params
        self._npu_id = params["npu_id"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        self._tuning_suggestion = {}

    def process(self: any) -> None:
        self._prepare_parallel_analysis()
        with ClusterParallelViewModel(self._collection_path) as _model:
            parallel_table_name = _model.get_table_name()
        if parallel_table_name == Constant.NA:
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB, "Cannot find the cluster_parallel table!")
        self._tuning_suggestion = ClusterParallelAnalysis(parallel_table_name, self._params).get_tuning_suggestion()
        output_file_name = "cluster_parallel_suggestion_-1_-1_-1.json"
        FileManager.storage_query_result_json_file(self._collection_path, self._tuning_suggestion, output_file_name)

    def _prepare_parallel_analysis(self: any) -> None:
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_PARALLEL)):
            raise ProfException(
                ProfException.PROF_CLUSTER_INVALID_DB, "Cannot find the cluster_parallel.db or Permission denied!"
            )
        if self._npu_id != Constant.DEFAULT_INVALID_VALUE:
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, "Invalid params! --id must be -1.")
        if self._model_id != Constant.DEFAULT_INVALID_VALUE:
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, "Invalid params! --model-id must be -1.")
        if self._iteration_id != Constant.DEFAULT_INVALID_VALUE:
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, "Invalid params! --iteration-id must be -1.")
