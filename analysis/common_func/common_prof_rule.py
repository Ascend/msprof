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


class CommonProfRule:
    """
    common prof rule class
    """

    # rule json key and value
    RULE_PROF = "prof_rules"
    RULE_ID = "Rule Id"
    RULE_CONDITION = "Rule Condition"
    RULE_TYPE = "Rule Type"
    RULE_SUBTYPE = "Rule Subtype"
    RULE_SUGGESTION = "Rule Suggestion"
    RULE_TUNING_TYPE = "Tuning Type"

    # condition json key and value
    CONDITIONS = "conditions"
    CONDITION_ID = "id"
    CONDITION_TYPE = "type"
    CONDITION_LEFT = "left"
    CONDITION_RIGHT = "right"
    CONDITION_CMP = "cmp"
    CONDITION_DEPENDENCY = "dependency"
    CONDITION_THRESHOLD = "threshold"
    CONDITION_ACCUMULATE = "accumulate"
    CONDITION_COMPARE = "compare"

    COND_TYPE_NORMAL = "normal"
    COND_TYPE_FORMULA = "formula"
    COND_TYPE_COUNT = "count"
    COND_TYPE_ACCUMULATE = "accumulate"

    # return json key and value
    RESULT_RULE_TYPE = "Rule Type"
    RESULT_RULE_SUBTYPE = "Rule Subtype"
    RESULT_RULE_SUGGESTION = "Rule Suggestion"
    RESULT_TUNING_DATA = "Tuning Data"
    RESULT_KEY = "result"

    RESULT_COMPUTATION = "Computation"
    RESULT_MEMORY = "Memory"
    RESULT_OPERATOR_SCHEDULE = "Operator Schedule"
    RESULT_OPERATOR_PROCESSING = "Operator Processing"
    RESULT_OPERATOR_METRICS = "Operator Metrics"
    RESULT_OPERATOR_PARALLELISM = "Operator Parallelism"
    RESULT_MODEL_BOTTLENECK = "Model Bottleneck Analysis"

    RESULT_RULE_DESCRIPTION = "Rule Description"
    RESULT_COMPUTATION_DESCRIPTION = (
        "Prompt users of some improperly high or low vector/cube/scalar usages of operators on AI Cores."
    )
    RESULT_MEMORY_DESCRIPTION = "Display improper memory usages of operators."
    RESULT_SCHEDULE_DESCRIPTION = "Display inefficient scheduling of operators."
    RESULT_PROCESSING_DESCRIPTION = (
        "Provide various processing suggestions based on operator processing policy, "
        "including multi-core processing, tiling policy, "
        "and reduced use of AI CPU operators."
    )
    RESULT_METRICS_DESCRIPTION = (
        "Collect statistics on operator performance efficiency and prompt users of high resource consumption."
    )
    RESULT_PARALLEL_DESCRIPTION = "Identify the serial wait bottleneck between the AI CPU and AI Core."
    RESULT_BOTTLENECK_DESCRIPTION = (
        "Identify model bottleneck distribution indicated by cube, vector, scalar and mte utilization."
    )

    # prof rule and condition file name
    RESULT_PROF_JSON = "prof_rule_{}.json"
    RESULT_PROF_JSON_HOST = "prof_rule.json"

    # tuning type
    TUNING_OPERATOR = 'Op Summary'
    TUNING_OP_PARALLEL = 'Op Parallel'
    TUNING_MODEL = 'Model Summary'
