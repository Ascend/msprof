/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H
#define ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H

#include <string>
#include <map>
#include <stdint.h>

namespace Analysis {
namespace Application {
const uint16_t  KILOBYTE = 1024;
const uint8_t ASSEMBLE_FAILED = 0;
const uint8_t ASSEMBLE_SUCCESS = 1;
const uint8_t DATA_NOT_EXIST = 2;
const std::string  UNKNOWN = "Unknown";
// file name
const std::string OP_SUMMARY_NAME = "op_summary";
const std::string NPU_MEMORY_NAME = "npu_mem";
const std::string NPU_MODULE_MEMORY_NAME = "npu_module_mem";
const std::string API_STATISTIC_NAME = "api_statistic";
const std::string FUSION_OP_NAME = "fusion_op";
const std::string TASK_TIME_SUMMARY_NAME = "task_time";
const std::string STEP_TRACE_SUMMARY_NAME = "step_trace";
const std::string COMM_STATISTIC_NAME = "communication_statistic";
const std::string OP_STATISTIC_NAME = "op_statistic";

const int INVALID_INDEX = -1;
const std::string OUTPUT_PATH = "mindstudio_profiler_output";
const std::string SUMMARY_SUFFIX = ".csv";
const std::string SLICE = "slice";
constexpr uint32_t CSV_LIMIT = 1000000; // csv文件每100w条记录切片一次
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H
