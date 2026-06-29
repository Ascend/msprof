/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_INFRASTRUCTURE_UTILS_COMMON_CONSTANT_H
#define ANALYSIS_INFRASTRUCTURE_UTILS_COMMON_CONSTANT_H

#include <cstdint>
#include <string>

namespace Analysis
{

// ===== Cross-namespace common constants =====
namespace Common
{
// rts task type
const std::string KERNEL_AICORE_TASK_TYPE = "KERNEL_AICORE";
const std::string KERNEL_AICPU_TASK_TYPE = "KERNEL_AICPU";
const std::string KERNEL_AIVEC_TASK_TYPE = "KERNEL_AIVEC";
const std::string KERNEL_MIX_AIC_TASK_TYPE = "KERNEL_MIX_AIC";
const std::string KERNEL_MIX_AIV_TASK_TYPE = "KERNEL_MIX_AIV";
const std::string KERNEL_SIMT_TASK_TYPE = "KERNEL_SIMT";

const std::string NA = "N/A";
const int32_t INVALID_VALUE = -1;
const std::string UNKNOWN = "UNKNOWN";
const uint64_t NS_TO_US = 1000;
const uint64_t MS_TO_NS = 1000000;
const uint64_t NANO_SECOND = 1000000000;
constexpr const uint64_t MAX_DB_BYTES = 10ULL * 1024 * 1024 * 1024;
const uint16_t PERCENTAGE = 100;
const uint64_t MICRO_SECOND = 1000000;
const uint64_t MILLI_SECOND = 1000;
const uint16_t BYTE_SIZE = 1024;

const std::string HOST = "host";
const std::string DEVICE_PREFIX = "device_";
const std::string SQLITE = "sqlite";
const std::string OUTPUT_PATH = "mindstudio_profiler_output";
}  // namespace Common

// Bring Common constants into Analysis namespace scope
using Common::BYTE_SIZE;
using Common::DEVICE_PREFIX;
using Common::HOST;
using Common::INVALID_VALUE;
using Common::KERNEL_SIMT_TASK_TYPE;
using Common::MAX_DB_BYTES;
using Common::MICRO_SECOND;
using Common::MILLI_SECOND;
using Common::MS_TO_NS;
using Common::NA;
using Common::NANO_SECOND;
using Common::NS_TO_US;
using Common::OUTPUT_PATH;
using Common::PERCENTAGE;
using Common::SQLITE;
using Common::UNKNOWN;

namespace Domain
{
const uint16_t DEVICE_ID_MASK = 0xFFF;
const std::string AI_CPU = "AI_CPU";
const double INVALID_TIME = -1;
const uint64_t S_TO_MS = 1000;
const uint64_t MS_TO_US = 1000;
const uint64_t US_TO_MS = 1000;
const uint64_t FREQ_TO_MHz = 1000000;
}  // namespace Domain

// ===== Analysis::Application specific constants =====
namespace Application
{
const std::string TASK_TYPE_FFTS_PLUS = "FFTS_PLUS";
const std::string OTHER_DIRECTION = "other";
const int SORT_INDEX_OFFSET = 70000;
const std::string ITER_CAT = "Iteration Time";
const std::string FP_BP_CAT = "FP_BP Time";
const std::string REFRESH_CAT = "Iteration Refresh";
const std::string DATA_AUG = "Data_aug Bound";
const std::string REDUCE_CAT = "Reduce";
const std::string GET_NEXT_CAT = "GetNext Time";
const size_t EXPECT_TIME_LEN = 14;
const std::string TASK_INDEX_NAME = "TaskIndex";
const std::string COMM_INDEX_NAME = "CommunicationTaskIndex";
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_INFRASTRUCTURE_UTILS_COMMON_CONSTANT_H
