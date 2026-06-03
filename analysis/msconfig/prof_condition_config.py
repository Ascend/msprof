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

from msconfig.meta_config import MetaConfig


class ProfConditionConfig(MetaConfig):
    DATA = [
        {
            "id": "condition_common_1",
            "type": "formula",
            "left": ["channel"],
            "right": 0,
            "formula": "{0}%16",
            "cmp": "!=",
        },
        {
            "id": "condition_common_2",
            "type": "formula",
            "left": ["channel"],
            "right": 0,
            "formula": "{0}%32",
            "cmp": "!=",
        },
        {
            "id": "condition_common_3",
            "type": "formula",
            "left": ["height", "width"],
            "right": 0,
            "formula": "({0}*{1})%16",
            "cmp": "!=",
        },
        {
            "id": "condition_common_6",
            "type": "formula",
            "left": ["batch"],
            "right": 0,
            "formula": "{0}%16",
            "cmp": "!=",
        },
        {
            "id": "condition_block_num_1",
            "type": "formula",
            "left": ["block_num", "core_num"],
            "right": 1,
            "formula": "{0}/{1}",
            "cmp": "<",
        },
        {
            "id": "condition_block_num_2",
            "type": "formula",
            "left": ["block_num"],
            "right": 0,
            "formula": "{0}&({0}-1)",
            "cmp": "!=",
        },
        {
            "id": "condition_block_num_3",
            "type": "formula",
            "left": ["block_num", "core_num"],
            "right": 0,
            "formula": "{0}%{1}",
            "cmp": "!=",
        },
        {"id": "condition_transData_1", "type": "normal", "left": "op_type", "right": "TransData", "cmp": "=="},
        {
            "id": "condition_transData_2",
            "dependency": "condition_transData_1",
            "type": "count",
            "threshold": 2,
            "cmp": ">",
        },
        {"id": "condition_memory_workspace_1", "type": "normal", "left": "memory_workspace", "right": 0, "cmp": ">"},
        {"id": "condition_wait_time_1", "type": "normal", "left": "task_wait_time", "right": 10, "cmp": ">"},
        {
            "id": "condition_wait_time_2",
            "dependency": "condition_wait_time_1",
            "type": "accumulate",
            "accumulate": ["task_wait_time"],
            "compare": ["task_duration", "task_wait_time"],
            "threshold": 0.03,
            "cmp": ">",
        },
        {"id": "condition_aicpu_1", "type": "normal", "left": "task_type", "right": "AI_CPU", "cmp": "=="},
        {"id": "condition_memory_bound_1", "type": "normal", "left": "memory_bound", "right": 1, "cmp": ">"},
        {
            "id": "condition_memory_bound_2",
            "dependency": "condition_memory_bound_1",
            "type": "accumulate",
            "accumulate": ["task_duration"],
            "compare": ["task_duration"],
            "threshold": 0.5,
            "cmp": ">",
        },
        {"id": "condition_vector_bound_1", "type": "normal", "left": "vector_bound", "right": 1, "cmp": ">"},
        {"id": "condition_vector_ratio_1", "type": "normal", "left": "vec_ratio", "right": 0.8, "cmp": "<"},
        {"id": "condition_vector_ratio_2", "type": "normal", "left": "vec_ratio", "right": 0.8, "cmp": ">"},
        {"id": "condition_cube_ratio_1", "type": "normal", "left": "mac_ratio", "right": 0.8, "cmp": "<"},
        {"id": "condition_cube_ratio_2", "type": "normal", "left": "mac_ratio", "right": 0, "cmp": ">"},
        {"id": "condition_scalar_ratio_1", "type": "normal", "left": "scalar_ratio", "right": 0.8, "cmp": ">"},
        {
            "id": "condition_vec_bankgroup_cflt_ratio_1",
            "type": "normal",
            "left": "vec_bankgroup_cflt_ratio",
            "right": 0.04,
            "cmp": ">",
        },
        {
            "id": "condition_vec_bank_cflt_ratio_1",
            "type": "normal",
            "left": "vec_bank_cflt_ratio",
            "right": 0.04,
            "cmp": ">",
        },
        {"id": "condition_task_duration_1", "type": "normal", "left": "task_duration", "right": 20, "cmp": ">"},
        {"id": "condition_int64_1", "type": "normal", "left": "input_data_types", "right": "INT64", "cmp": "contain"},
        {
            "id": "condition_strided_slice_grad_1",
            "type": "normal",
            "left": "op_name",
            "right": "StridedSliceGrad",
            "cmp": "contain",
        },
        {
            "id": "condition_ai_cpu_parallelism_1",
            "type": "formula",
            "left": [
                "AI CPU Execution Time(us)",
                "AI Core Execution Time(us)",
                "Concurrent AI Core and AI CPU Execution Time(us)",
            ],
            "right": 0.05,
            "formula": "{0}/({0}+{1}+{2})",
            "cmp": ">",
        },
        {
            "id": "condition_cube_affinity_1",
            "type": "formula",
            "left": ["Cube Utilization", "Vector Utilization", "Scalar Utilization", "MTE Utilization"],
            "right": 0.5,
            "formula": "{0}/({0}+{1}+{2}+{3})",
            "cmp": ">=",
        },
        {
            "id": "condition_cube_affinity_2",
            "type": "formula",
            "left": ["Cube Utilization", "Vector Utilization", "Scalar Utilization", "MTE Utilization"],
            "right": 0.5,
            "formula": "{0}/({0}+{1}+{2}+{3})",
            "cmp": "<",
        },
    ]

    def __init__(self):
        self.support_parser = False
