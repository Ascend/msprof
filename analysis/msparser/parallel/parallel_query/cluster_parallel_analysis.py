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

from common_func.db_name_constant import DBNameConstant
from msparser.parallel.parallel_query.data_parallel_analysis import DataParallelAnalysis
from msparser.parallel.parallel_query.model_parallel_analysis import ModelParallelAnalysis
from msparser.parallel.parallel_query.pipeline_parallel_analysis import PipelineParallelAnalysis


class ClusterParallelAnalysis:
    def __init__(self: any, parallel_table_name: str, params: dict):
        self._parallel_table_name = parallel_table_name
        self._params = params

    def get_parallel_data(self: any) -> dict:
        return self._create_parallel_obj().get_parallel_data()

    def get_tuning_suggestion(self: any) -> str:
        return self._create_parallel_obj().get_tuning_suggestion()

    def _create_parallel_obj(self: any) -> object:
        if self._parallel_table_name == DBNameConstant.TABLE_CLUSTER_PIPELINE_PARALLEL:
            return PipelineParallelAnalysis(self._params)
        elif self._parallel_table_name == DBNameConstant.TABLE_CLUSTER_MODEL_PARALLEL:
            return ModelParallelAnalysis(self._params)
        else:
            return DataParallelAnalysis(self._params)
