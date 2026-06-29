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

#include <stdint.h>

#include <map>
#include <string>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis
{
namespace Application
{
const std::string UNKNOWN = "Unknown";  // 仅mem使用 避免兼容性保持首字母大写不变
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

const std::string SUMMARY_SUFFIX = ".csv";
const std::string SLICE = "slice";
constexpr uint32_t CSV_LIMIT = 1000000;  // csv文件每100w条记录切片一次
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_SUMMARY_CONSTANT_H
