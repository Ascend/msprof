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


class TestAscendMsprofAllSysSwitch(BaseAscendMsprofChecker):
    EXPECT_DB_TABLES = [
        "ACC_PMU", "AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE",
        "ENUM_HCCL_DATA_TYPE", "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
        "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HBM", "HCCS", "HOST_INFO",
        "LLC", "META_DATA", "NETDEV_STATS", "NIC", "NPU_INFO", "NPU_MEM",
        "NPU_MODULE_MEM", "PCIE", "QOS", "RANK_DEVICE_MAP", "ROCE", "SESSION_TIME_INFO",
        "SOC_BANDWIDTH_LEVEL", "STRING_IDS", "TASK", "TASK_PMU_INFO",
    ]
    DISABLED_CHECKS = {"soc_pmu", "l2_cache"}
    API_STATISTIC_SPEC = {
        "pattern": "api_statistic*.csv",
        "headers": ["Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
        "items": [
            {"pattern": {"Level": ["acl", "node"], "Device_id": ["host"]}, "fuzzy_match": False},
            {"pattern": {"API Name": ["Add*", "Cast*", "Relu*"]}},
        ],
        "non_negative_columns": ["Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"],
    }
    OTHER_CSV_SPECS = [
        {
            "display_name": "dvpp.csv",
            "pattern": "dvpp_*.csv",
            "headers": ["Device_id", "Dvpp Id", "Engine Type", "Engine ID", "All Time(us)", "All Frame", "All Utilization(%)"],
            "non_negative_columns": ["Dvpp Id", "Engine ID", "All Time(us)", "All Frame", "All Utilization(%)"],
        },
        {
            "display_name": "hccs.csv",
            "pattern": "hccs_*.csv",
            "headers": ["Device_id", "Mode", "Max", "Min", "Average"],
            "non_negative_columns": ["Max", "Min", "Average"],
        },
        {
            "display_name": "nic.csv",
            "pattern": "nic_*.csv",
            "headers": [
                "Device_id", "Timestamp(us)", "Bandwidth(MB/s)", "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                "rxDropped rate(%)", "Tx Bandwidth efficiency(%)", "txPacket/s", "txError rate(%)", "txDropped rate(%)", "funcId",
            ],
            "non_negative_columns": [
                "Timestamp(us)", "Bandwidth(MB/s)", "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                "rxDropped rate(%)", "Tx Bandwidth efficiency(%)", "txPacket/s", "txError rate(%)", "txDropped rate(%)", "funcId",
            ],
        },
        {
            "display_name": "pcie.csv",
            "pattern": "pcie_*.csv",
            "headers": ["Device_id", "Mode", "Min", "Max", "Avg"],
            "non_negative_columns": ["Min", "Max", "Avg"],
        },
        {
            "display_name": "roce.csv",
            "pattern": "roce_*.csv",
            "headers": [
                "Device_id", "Timestamp(us)", "Bandwidth(MB/s)", "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                "rxDropped rate(%)", "Tx Bandwidth efficiency(%)", "txPacket/s", "txError rate(%)", "txDropped rate(%)", "funcId",
            ],
            "non_negative_columns": [
                "Timestamp(us)", "Bandwidth(MB/s)", "Rx Bandwidth efficiency(%)", "rxPacket/s", "rxError rate(%)",
                "rxDropped rate(%)", "Tx Bandwidth efficiency(%)", "txPacket/s", "txError rate(%)", "txDropped rate(%)", "funcId",
            ],
        },
        {
            "display_name": "ts_cpu_pmu_events.csv",
            "pattern": "ts_cpu_pmu_events_*.csv",
            "headers": ["Device_id", "Event", "Name", "Count"],
            "non_negative_columns": ["Count"],
        },
        {
            "display_name": "ts_cpu_top_function.csv",
            "pattern": "ts_cpu_top_function_*.csv",
            "headers": ["Device_id", "Function", "Cycles", "Cycles(%)"],
            "non_negative_columns": ["Cycles", "Cycles(%)"],
        },
    ]


def test_ascend_msprof_all_sys_switch(prof_path):
    TestAscendMsprofAllSysSwitch(prof_path).check_msprof_file()


if __name__ == "__main__":
    path = sys.argv[1]
    test_ascend_msprof_all_sys_switch(path)
