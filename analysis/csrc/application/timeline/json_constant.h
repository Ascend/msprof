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

namespace Analysis
{
namespace Application
{
const uint8_t ASSEMBLE_FAILED = 0;
const uint8_t ASSEMBLE_SUCCESS = 1;
const uint8_t DATA_NOT_EXIST = 2;
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
const std::string WAIT_EVENT = "aclrtStreamWaitEvent";
const std::string MS_TX = "MsTx";
const std::string FLOW_START = "s";
const std::string FLOW_END = "f";
const std::string FLOW_BP = "e";
const std::string OUTPUT_PATH = "mindstudio_profiler_output";
const std::string JSON_SUFFIX = ".json";
const std::string MSPROF_JSON_FILE = "msprof";
const double NS_TO_US = 1000.0;
const int CONN_OFFSET = 32;
const double B_TO_KB = 1024.0;
const std::string STEP_TRACE_FILE = "step_trace";
const std::string MSPROF_TX_FILE = "msprof_tx";
const std::string RECORD_EVENT = "aclrtRecordEvent";
const std::string MEMCPY_ASYNC = "MEMCPY_ASYNC";
const std::string KERNEL_SIMT_TASK_TYPE = "KERNEL_SIMT";
/*
 * json格式要求多个对象使用[]包装，再每一层json后添加了","分割，最终会形成[{},true]的结果，因此需要写入内容的时候过滤掉
 * [ true],共6位长度
 */
const std::size_t FILE_CONTENT_SUFFIX = 6;
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_DELIVERABLES_CONSTANT_H
