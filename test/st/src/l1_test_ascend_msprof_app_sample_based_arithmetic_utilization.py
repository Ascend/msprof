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


class TestAscendMsprofAppSampleBasedArithmeticUtilization(BaseAscendMsprofChecker):
    EXPECT_DB_TABLES = [
        "ACC_PMU", "AICORE_FREQ", "CANN_API", "COMPUTE_TASK_INFO", "ENUM_API_TYPE",
        "ENUM_HCCL_DATA_TYPE", "ENUM_HCCL_LINK_TYPE", "ENUM_HCCL_RDMA_TYPE", "ENUM_HCCL_TRANSPORT_TYPE",
        "ENUM_MEMCPY_OPERATION", "ENUM_MODULE", "ENUM_MSTX_EVENT_TYPE", "HBM", "HOST_INFO", "LLC",
        "MEMCPY_INFO", "META_DATA", "NPU_INFO", "NPU_MEM", "NPU_MODULE_MEM", "QOS", "RANK_DEVICE_MAP",
        "SAMPLE_PMU_SUMMARY", "SAMPLE_PMU_TIMELINE", "SESSION_TIME_INFO", "SOC_BANDWIDTH_LEVEL",
        "STRING_IDS", "TASK",
    ]
    AI_VECTOR_CORE_UTILIZATION_SPEC = {
        "pattern": "ai_vector_core_utilization*.csv",
        "headers": ["mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio", "cube_fops", "vector_fops"],
        "non_negative_columns": ["mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio", "cube_fops", "vector_fops"],
    }
    AI_CORE_UTILIZATION_SPEC = {
        "pattern": "ai_core_utilization*.csv",
        "headers": ["mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio", "cube_fops", "vector_fops"],
        "non_negative_columns": ["mac_fp16_ratio", "mac_int8_ratio", "vec_fp32_ratio", "vec_fp16_ratio", "vec_int32_ratio", "vec_misc_ratio", "cube_fops", "vector_fops"],
    }


def test_ascend_msprof_app_sample_based_arithmetic_utilization(prof_path):
    TestAscendMsprofAppSampleBasedArithmeticUtilization(prof_path).check_msprof_file()


if __name__ == "__main__":
    path = sys.argv[1]
    test_ascend_msprof_app_sample_based_arithmetic_utilization(path)
