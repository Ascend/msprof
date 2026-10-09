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

import logging

from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.suggestion_constant import SuggestionConstant


class PipelineParallelAnalysis:
    def __init__(self: any, params: dict):
        self._params = params

    def get_parallel_data(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            first_field_name, first_header_name = _model.get_first_field_name(self._params)
            condition, query_params = _model.get_parallel_condition_and_query_params(self._params)
            parallel_data = _model.get_pipeline_parallel_data(first_field_name, condition, query_params)
        return {
            "parallel_mode": "Pipeline Parallel",
            "headers": [
                first_header_name,
                "Computation Time(us)",
                "Pure Communication Time(Only Receive Op Included)(us)",
                'Pure Communication Time(Receive Op Not Included)(us)',
                'Stage Time(us)',
            ],
            "data": parallel_data,
        }

    def get_tuning_suggestion(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            tuning_data = _model.get_pipeline_parallel_tuning_data()
        suggestion = {"parallel_mode": "Pipeline Parallel", "suggestion": []}
        if not tuning_data:
            return suggestion
        if not tuning_data[0]:
            return suggestion
        if tuning_data[0][0] is None or tuning_data[0][1] is None or tuning_data[0][2] is None:
            logging.error("Invalid tuning data from ClusterPipelineParallel table. %s", tuning_data[0])
            return suggestion
        if tuning_data[0][1] > 0.1:
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("pipeline-parallel")
                .get("bad_operator_tiling")
                .format('{:.1%}'.format(tuning_data[0][1]))
            )
        index_desc = ""
        if tuning_data[0][0] > 0.1:
            index_desc = (
                index_desc
                + ", the proportion of the pure communication time (only receive op "
                "contained) should be less than 10% (current value: {})".format('{:.1%}'.format(tuning_data[0][0]))
            )
        if tuning_data[0][2] > 0:
            index_desc = (
                index_desc + ", the deviation between the stage time of all devices and the "
                "average stage time should not exceed 20%"
            )
        if index_desc:
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("pipeline-parallel").get("bad_stage_division").format(index_desc)
            )
        if not suggestion.get("suggestion"):
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("pipeline-parallel").get("optimal_stage_division")
            )
        return suggestion
