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
import logging

from src.base_ascend_msprof_checker import BaseAscendMsprofChecker

logging.basicConfig(level=logging.INFO,
                    format="\n%(asctime)s %(filename)s [line:%(lineno)d] [%(levelname)s] %(message)s")


class TestAscendMsprofCommunication(BaseAscendMsprofChecker):
    EXPECT_DB_TABLES = [
        "ACC_PMU",
        "AICORE_FREQ",
        "CANN_API",
        "COMMUNICATION_OP",
        "COMMUNICATION_TASK_INFO",
        "COMPUTE_TASK_INFO",
        "ENUM_API_TYPE",
        "ENUM_HCCL_DATA_TYPE",
        "ENUM_HCCL_LINK_TYPE",
        "ENUM_HCCL_RDMA_TYPE",
        "ENUM_HCCL_TRANSPORT_TYPE",
        "ENUM_MEMCPY_OPERATION",
        "ENUM_MODULE",
        "ENUM_MSTX_EVENT_TYPE",
        "ENUM_OVERLAP_ANALYSIS_TYPE",
        "HBM",
        "HCCS",
        "HOST_INFO",
        "LLC",
        "MEMCPY_INFO",
        "META_DATA",
        "NETDEV_STATS",
        "NIC",
        "NPU_INFO",
        "NPU_MEM",
        "NPU_MODULE_MEM",
        "OVERLAP_ANALYSIS",
        "PCIE",
        "QOS",
        "RANK_DEVICE_MAP",
        "ROCE",
        "SIO",
        "SESSION_TIME_INFO",
        "SOC_BANDWIDTH_LEVEL",
        "STRING_IDS",
        "TASK",
        "TASK_PMU_INFO",
    ]
    DB_VALUE_SPECS = [
        {
            "table": "COMMUNICATION_OP",
            "field": "rankSize",
            "expected_values": [2],
        }
    ]
    TIMELINE_CHECKS = [
        {"key": "cat", "values": ["HostToDevice"], "fuzzy_match": True},
        {"key": "name", "values": ["*allReduce*"], "fuzzy_match": True},
    ]
    OP_SUMMARY_SPEC = {
        "pattern": "op_summary_*.csv",
        "headers": [
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
            {"pattern": {"Op Name": ["*allReduce*"]}},
            {"pattern": {"OP Type": ["*allReduce*"], "Task Type": ["COMMUNICATION", "AI_VECTOR_CORE"]}},
        ],
        "non_negative_columns": ["Task Duration(us)", "Task Wait Time(us)", "Task Start Time(us)", "Device_id", "Model ID"],
    }
    OP_STATISTIC_SPEC = {
        "pattern": "op_statistic_*.csv",
        "headers": ["Device_id", "OP Type", "Core Type", "Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
        "items": [{"pattern": {"OP Type": ["Fill"], "Core Type": ["AI_VECTOR_CORE"]}, "fuzzy_match": False}],
        "non_negative_columns": ["Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
    }
    API_STATISTIC_SPEC = {
        "pattern": "api_statistic*.csv",
        "headers": ["Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
        "non_negative_columns": ["Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
    }
    L2_CACHE_SPEC = {
        "pattern": "l2_cache*.csv",
        "headers": ["Device_id", "Stream Id", "Task Id", "Hit Rate", "Victim Rate", "Op Name"],
        "non_negative_columns": ["Stream Id", "Task Id", "Victim Rate"],
    }
    SOC_PMU_SPEC = {
        "pattern": "soc_pmu*.csv",
        "headers": ["Device_id", "Stream Id", "Task Id", "TLB Hit Rate", "TLB Miss Rate", "Op Name"],
        "non_negative_columns": ["Stream Id", "Task Id", "TLB Hit Rate", "TLB Miss Rate"],
    }
    TASK_TIME_SPEC = {
        "pattern": "task_time*.csv",
        "headers": ["Device_id", "kernel_name", "kernel_type", "stream_id", "task_id", "task_time(us)", "task_start(us)", "task_stop(us)"],
        "non_negative_columns": ["stream_id", "task_id", "task_time(us)", "task_start(us)", "task_stop(us)"],
    }
    OTHER_CSV_SPECS = [
        {
            "display_name": "communication_statistic.csv",
            "pattern": "communication_statistic*.csv",
            "headers": ["Device_id", "OP Type", "Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
            "non_negative_columns": ["Device_id", "Count", "Total Time(us)", "Min Time(us)", "Avg Time(us)", "Max Time(us)", "Ratio(%)"],
        }
    ]


def test_ascend_msprof_communication(prof_path):
    TestAscendMsprofCommunication(prof_path).check_msprof_file()


if __name__ == "__main__":
    path = sys.argv[1]
    test_ascend_msprof_communication(path)
