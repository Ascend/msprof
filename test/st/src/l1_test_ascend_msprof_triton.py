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
import sys
import glob
import pandas as pd
from utils.file_check import FileChecker

SHAPE_COLUMNS = [
    "Input Shapes",
    "Input Data Types",
    "Input Formats",
    "Output Shapes",
    "Output Data Types",
    "Output Formats",
]

REQUIRED_COLUMNS = {"Op Name", "OP Type"} | set(SHAPE_COLUMNS)


def test_ascend_msprof_triton_op(prof_path):
    op_summary_path = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_output/op_summary_*.csv")
    if not op_summary_path:
        raise FileNotFoundError(f"No op_summary.csv found in {prof_path}")

    FileChecker.check_csv_items(
        op_summary_path[0],
        {
            "Op Name": [
                "triton_poi_fused_add_0",
                "aclnnAdd_*",
                "triton_kernel_add",
                "aclnnBitwiseOrTensor_*",
                "triton_kernel_or",
            ]
        },
    )
    FileChecker.check_csv_items(
        op_summary_path[0],
        {"OP Type": ["triton_poi_fused_add_0", "Add", "triton_kernel_add", "triton_kernel_or", "LogicalOr"]},
        fuzzy_match=False,
    )

    df = pd.read_csv(op_summary_path[0])
    missing = REQUIRED_COLUMNS - set(df.columns)
    if missing:
        raise KeyError(f"Missing required columns in op_summary: {missing}")

    mask = df["OP Type"].str.lower().str.contains("add|or", na=False)
    for col in SHAPE_COLUMNS:
        invalid = df.loc[mask, col].isin(["N/A", ""]) | df.loc[mask, col].isna()
        if invalid.any():
            bad_rows = df.loc[mask & invalid, ["Op Name", "OP Type"]]
            raise AssertionError(f"Column '{col}' has invalid values (N/A or empty) for rows:\n{bad_rows.to_string()}")


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_triton_op(path)
