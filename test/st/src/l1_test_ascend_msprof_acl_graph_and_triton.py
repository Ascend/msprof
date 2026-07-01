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

DEFAULT_MODEL_ID = 4294967295


def test_ascend_msprof_acl_graph_and_triton(prof_path):
    op_summary_path = glob.glob(f"{prof_path}/PROF_*/mindstudio_profiler_output/op_summary_*.csv")
    if not op_summary_path:
        raise FileNotFoundError(f"No op_summary.csv found in {prof_path}")

    FileChecker.check_csv_headers(op_summary_path[0], ["Op Name", "Model ID"])
    df = pd.read_csv(op_summary_path[0])

    if "Op Name" not in df.columns:
        raise KeyError("Missing required column 'Op Name' in op_summary")
    if "Model ID" not in df.columns:
        raise KeyError("Missing required column 'Model ID' in op_summary")

    matched = df[df["Op Name"] == "triton_kernel_add"]
    if matched.empty:
        raise AssertionError("Op Name 'triton_kernel_add' not found in op_summary")

    invalid_model = matched[matched["Model ID"] == DEFAULT_MODEL_ID]
    if not invalid_model.empty:
        raise AssertionError(f"Found triton_kernel_add with invalid Model ID {DEFAULT_MODEL_ID}")


if __name__ == '__main__':
    path = sys.argv[1]
    test_ascend_msprof_acl_graph_and_triton(path)
