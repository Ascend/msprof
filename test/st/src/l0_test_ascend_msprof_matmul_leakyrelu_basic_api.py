# -------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
import glob
import sys

import pandas as pd
from utils.file_check import FileChecker

from src.l0_test_ascend_msprof_all_switch import TestAscendMsprofAllSwitch

MATMUL_LEAKYRELU_KEYWORDS = ("mmad_vec_custom", "matmul", "leakyrelu")
EXPECTED_KERNEL_PATTERN = "*mmad_vec_custom*"
EXPECTED_KERNEL_NAME = "mmad_vec_custom"
EXPECTED_RUNTIME_APIS = [
    "LaunchKernelV2",
    "MemCopySync",
    "StreamCreate",
    "StreamDestroy",
    "DeviceReset",
]
SHAPE_COLUMNS = [
    "Input Shapes",
    "Input Data Types",
    "Input Formats",
    "Output Shapes",
    "Output Data Types",
    "Output Formats",
]


class TestAscendMsprofMatmulLeakyreluBasicApi(TestAscendMsprofAllSwitch):
    TIMELINE_CHECKS = [
        {"key": "cat", "values": ["HostToDevice"], "fuzzy_match": True},
    ]
    OP_SUMMARY_SPEC = {
        "pattern": "op_summary_*.csv",
        "headers": [
            "Op Name",
            "OP Type",
            "Task Type",
            "Task Duration(us)",
            "Task Wait Time(us)",
            "Task Start Time(us)",
            "Block Num",
            "Mix Block Num",
            "Task ID",
            "Stream ID",
            "Device_id",
            "Model ID",
        ],
        "items": [
            {"pattern": {"Op Name": [EXPECTED_KERNEL_PATTERN], "OP Type": [EXPECTED_KERNEL_PATTERN]}},
            {"pattern": {"Task Type": ["MIX_AIC"]}, "fuzzy_match": False},
        ],
        "non_negative_columns": [
            "Task Duration(us)",
            "Task Wait Time(us)",
            "Task Start Time(us)",
            "Block Num",
            "Mix Block Num",
            "Task ID",
            "Stream ID",
            "Device_id",
            "Model ID",
        ],
    }
    OP_STATISTIC_SPEC = {
        "pattern": "op_statistic_*.csv",
        "headers": [
            "Device_id",
            "OP Type",
            "Core Type",
            "Count",
            "Total Time(us)",
            "Min Time(us)",
            "Avg Time(us)",
            "Max Time(us)",
            "Ratio(%)",
        ],
        "items": [
            {"pattern": {"OP Type": [EXPECTED_KERNEL_PATTERN], "Core Type": ["MIX_AIC"]}},
        ],
        "non_negative_columns": ["Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
    }
    API_STATISTIC_SPEC = {
        "pattern": "api_statistic*.csv",
        "headers": ["Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
        "items": [
            {"pattern": {"Level": ["runtime"]}, "fuzzy_match": False},
            {"pattern": {"API Name": EXPECTED_RUNTIME_APIS}, "fuzzy_match": False},
        ],
        "non_negative_columns": ["Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
    }
    L2_CACHE_SPEC = {
        "pattern": "l2_cache*.csv",
        "headers": TestAscendMsprofAllSwitch.L2_CACHE_SPEC["headers"],
        "items": [{"pattern": {"Op Name": [EXPECTED_KERNEL_PATTERN]}}],
        "non_negative_columns": TestAscendMsprofAllSwitch.L2_CACHE_SPEC["non_negative_columns"],
    }
    NPU_MODULE_MEM_SPEC = {
        "pattern": "npu_module_mem*.csv",
        "headers": TestAscendMsprofAllSwitch.NPU_MODULE_MEM_SPEC["headers"],
        "items": [{"pattern": {"Component": ["SLOG"]}, "fuzzy_match": False}],
        "non_negative_columns": TestAscendMsprofAllSwitch.NPU_MODULE_MEM_SPEC["non_negative_columns"],
    }
    SOC_PMU_SPEC = {
        "pattern": "soc_pmu*.csv",
        "headers": TestAscendMsprofAllSwitch.SOC_PMU_SPEC["headers"],
        "items": [{"pattern": {"Op Name": [EXPECTED_KERNEL_PATTERN]}}],
        "non_negative_columns": TestAscendMsprofAllSwitch.SOC_PMU_SPEC["non_negative_columns"],
    }
    TASK_TIME_SPEC = {
        "pattern": "task_time*.csv",
        "headers": TestAscendMsprofAllSwitch.TASK_TIME_SPEC["headers"],
        "items": [
            {"pattern": {"kernel_name": [EXPECTED_KERNEL_PATTERN]}},
            {"pattern": {"kernel_type": ["MIX_AIC"]}, "fuzzy_match": False},
        ],
        "non_negative_columns": TestAscendMsprofAllSwitch.TASK_TIME_SPEC["non_negative_columns"],
    }


def _contains_matmul_leakyrelu_operator(df: pd.DataFrame, columns: list[str]) -> bool:
    combined = df[columns].fillna("").astype(str).agg(" ".join, axis=1).str.lower()
    return combined.str.contains("|".join(MATMUL_LEAKYRELU_KEYWORDS), regex=True).any()


def _get_first_output_csv(prof_path: str, pattern: str, display_name: str) -> str:
    paths = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_output/{pattern}")
    if not paths:
        raise FileNotFoundError(f"No {display_name} found in {prof_path}")
    return paths[0]


def _get_kernel_rows(df: pd.DataFrame, column: str) -> pd.DataFrame:
    return df[df[column].fillna("").str.contains(EXPECTED_KERNEL_NAME, case=False, regex=False)]


def _check_matmul_leakyrelu_operator_presence(prof_path: str):
    task_time_df = pd.read_csv(_get_first_output_csv(prof_path, "task_time*.csv", "task_time.csv"))
    if not _contains_matmul_leakyrelu_operator(task_time_df, ["kernel_name"]):
        raise AssertionError(
            "Matmul + LeakyRelu operator was not found in task_time. "
            "Expected kernel_name to contain mmad_vec_custom, matmul, or leakyrelu."
        )
    mix_rows = _get_kernel_rows(task_time_df, "kernel_name")
    mix_rows = mix_rows[mix_rows["kernel_type"] == "MIX_AIC"]
    if mix_rows.empty:
        raise AssertionError("No MIX_AIC mmad_vec_custom row found in task_time.")


def _check_op_summary_shapes_are_na(prof_path: str):
    op_summary_path = _get_first_output_csv(prof_path, "op_summary_*.csv", "op_summary.csv")
    FileChecker.check_csv_headers(op_summary_path, SHAPE_COLUMNS)
    df = pd.read_csv(op_summary_path)
    kernel_rows = _get_kernel_rows(df, "Op Name")
    if kernel_rows.empty:
        raise AssertionError("No mmad_vec_custom row found in op_summary.")
    for col in SHAPE_COLUMNS:
        values = kernel_rows[col].fillna("N/A").astype(str)
        if not values.eq("N/A").all():
            raise AssertionError(f"Expected '{col}' to be N/A for mmad_vec_custom rows, got {values.tolist()}")


def test_ascend_msprof_matmul_leakyrelu_basic_api(prof_path):
    TestAscendMsprofMatmulLeakyreluBasicApi(prof_path).check_msprof_file()
    _check_matmul_leakyrelu_operator_presence(prof_path)
    _check_op_summary_shapes_are_na(prof_path)


if __name__ == "__main__":
    path = sys.argv[1]
    test_ascend_msprof_matmul_leakyrelu_basic_api(path)
