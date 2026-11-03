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

#ifndef ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H
#define ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H

#include <stdint.h>

#include <string>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis
{
namespace Application
{
const int DEFAULT_TID = 0;
const uint32_t HOST_PID = 31;
const int HIGH_BIT_OFFSET = 10;
const int MIDDLE_BIT_OFFSET = 5;
const std::string META_DATA_PROCESS_NAME = "process_name";
const std::string META_DATA_PROCESS_INDEX = "process_sort_index";
const std::string META_DATA_PROCESS_LABEL = "process_labels";
const std::string META_DATA_THREAD_NAME = "thread_name";
const std::string META_DATA_THREAD_INDEX = "thread_sort_index";
const std::string HOST_TO_DEVICE = "HostToDevice";
const std::string MODEL_EXECUTE_TASK_TYPE = "MODEL_EXECUTE";
const std::string ACLMDL_RI_EXECUTE_ASYNC = "aclmdlRIExecuteAsync";
const std::string WAIT_EVENT = "aclrtStreamWaitEvent";
const std::string MS_TX = "MsTx";
const std::string FLOW_START = "s";
const std::string FLOW_END = "f";
const std::string FLOW_BP = "e";
const std::string JSON_SUFFIX = ".json";
constexpr std::size_t JSON_EVENT_BATCH = 4096;
const std::string MSPROF_JSON_FILE = "msprof";
const int CONN_OFFSET = 32;
const std::string STEP_TRACE_FILE = "step_trace";
const std::string MSPROF_TX_FILE = "msprof_tx";
const std::string RECORD_EVENT = "aclrtRecordEvent";
const std::string MEMCPY_ASYNC = "MEMCPY_ASYNC";

/*
 * host侧api与device侧task之间连线的类别。连线名统一为 HostToDevice，非默认类别追加
 * 首字母大写的括号后缀（如 HostToDevice(Graph)）。新增类别时在这里补枚举值与常量即可。
 */
enum class FlowCategory : uint8_t
{
    DEFAULT = 0,  // 不带后缀，直接输出 HostToDevice
    KERNEL = 1,   // HostToDevice(Kernel)
    GRAPH = 2     // HostToDevice(Graph)
};

const std::string KERNEL_TO_DEVICE = "HostToDevice(Kernel)";
const std::string GRAPH_TO_DEVICE = "HostToDevice(Graph)";

// 类别名映射：DEFAULT 直接返回 HostToDevice；其余类别返回对应的完整常量名。
// 新增类别必须在此补充分支，未配映射的取值退化为 DEFAULT，避免输出非法连线名。
inline const std::string &GetFlowCategoryName(FlowCategory category)
{
    switch (category)
    {
        case FlowCategory::KERNEL:
            return KERNEL_TO_DEVICE;
        case FlowCategory::GRAPH:
            return GRAPH_TO_DEVICE;
        case FlowCategory::DEFAULT:
        default:
            break;
    }
    return HOST_TO_DEVICE;
}
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H
