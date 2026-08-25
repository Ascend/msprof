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
# MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import sys

from src.l0_test_ascend_msprof_all_switch import TestAscendMsprofAllSwitch

EXPECTED_KERNEL_PATTERN = "*mmad_custom*"
EXPECTED_RUNTIME_APIS = [
    "LaunchKernelV2",
    "MemCopySync",
    "StreamCreate",
    "StreamDestroy",
    "DeviceReset",
]


class TestAscendMsprofMatmulBasicApi(TestAscendMsprofAllSwitch):
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
            {"pattern": {"Task Type": ["AI_CORE"]}, "fuzzy_match": False},
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
            {"pattern": {"OP Type": [EXPECTED_KERNEL_PATTERN], "Core Type": ["AI_CORE"]}},
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
            {"pattern": {"kernel_type": ["AI_CORE"]}, "fuzzy_match": False},
        ],
        "non_negative_columns": TestAscendMsprofAllSwitch.TASK_TIME_SPEC["non_negative_columns"],
    }


def test_ascend_msprof_matmul_basic_api(prof_path):
    TestAscendMsprofMatmulBasicApi(prof_path).check_msprof_file()


if __name__ == "__main__":
    path = sys.argv[1]
    test_ascend_msprof_matmul_basic_api(path)
