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

#ifndef ANALYSIS_APPLICATION_INCLUDE_REPORT_MAP_H
#define ANALYSIS_APPLICATION_INCLUDE_REPORT_MAP_H

#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/json_trace/include/meta_data_event.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;

// The key in the reports config and the timeline node key it exports, it keeps the export order.
const std::vector<std::pair<std::string, std::string>> TIMELINE_JSON_NODE_KEYS{
    {"ascend", PROCESS_TASK},
    {"acc_pmu", PROCESS_ACC_PMU},
    {"cann", PROCESS_API},
    {"ddr", PROCESS_DDR},
    {"stars_chip_trans", PROCESS_STARS_CHIP_TRANS},
    {"hbm", PROCESS_HBM},
    {"communication", PROCESS_HCCL},
    {"hccs", PROCESS_HCCS},
    {"os_runtime_api", PROCESSOR_NAME_OSRT_API},
    {"network_usage", PROCESS_NETWORK_USAGE},
    {"disk_usage", PROCESS_DISK_USAGE},
    {"memory_usage", PROCESS_MEMORY_USAGE},
    {"cpu_usage", PROCESS_CPU_USAGE},
    {"cpu_freq", PROCESS_CPU_FREQUENCY},
    {"msproftx", PROCESS_MSPROFTX},
    {"npu_mem", PROCESS_NPU_MEM},
    {"overlap_analyse", PROCESS_OVERLAP_ANALYSE},
    {"pcie", PROCESS_PCIE},
    {"sio", PROCESS_SIO},
    {"stars_soc", PROCESS_STARS_SOC},
    {"step_trace", PROCESS_STEP_TRACE},
    {"freq", PROCESS_LOW_POWER},
    {"llc", PROCESS_LLC},
    {"nic", PROCESS_NIC},
    {"roce", PROCESS_ROCE},
    {"qos", PROCESS_QOS},
    {"device_tx", PROCESS_DEVICE_TX},
    {"biu_perf", PROCESS_BIU_PERF},
    {"ub", PROCESS_UB},
    {"block_detail", PROCESS_BLOCK_DETAIL},
    {"ccu", PROCESS_CCU},
    {"dpu", PROCESS_DPU},
    {"fusion_task", PROCESS_FUSION_TASK},
};

// The key in the reports config and the summary node key it exports, it keeps the export order. The aicpu, dp and
// aicpu_mi keys share one summary node, so the order of their first appearance decides where that node is exported.
const std::vector<std::pair<std::string, std::string>> SUMMARY_JSON_NODE_KEYS{
    {"op_summary", PROCESSOR_OP_SUMMARY},
    {"comm_statistic", PROCESSOR_NAME_COMM_STATISTIC},
    {"op_statistic", PROCESSOR_NAME_OP_STATISTIC},
    {"npu_memory", PROCESSOR_NAME_NPU_MEM},
    {"npu_module_memory", PROCESSOR_NAME_NPU_MODULE_MEM},
    {"api_statistic", PROCESSOR_NAME_API},
    {"fusion_op", PROCESSOR_NAME_FUSION_OP},
    {"task_time", PROCESSOR_TASK_TIME_SUMMARY},
    {"step_trace", PROCESSOR_NAME_STEP_TRACE},
    {"page_fault", PROCESSOR_NAME_PAGE_FAULT},
    {"aicpu", PROCESSOR_NAME_AICPU},
    {"dp", PROCESSOR_NAME_AICPU},
    {"aicpu_mi", PROCESSOR_NAME_AICPU},
    {"hbm", PROCESSOR_NAME_HBM},
    {"ub", PROCESSOR_NAME_UB},
    {"llc_read_write", PROCESSOR_NAME_LLC},
};

// The key in the reports config and the DB savers it exports, it keeps the export order. The saver name is the
const std::vector<std::pair<std::string, std::vector<std::string>>> DB_JSON_NODE_KEYS{
    {"ascend_hardware",
     {PROCESSOR_NAME_TASK, PROCESSOR_NAME_COMPUTE_TASK_INFO, PROCESSOR_NAME_MEMCPY_INFO, PROCESSOR_NAME_TASK_PMU_INFO,
      PROCESSOR_NAME_ENUM}},
    {"cann_api", {PROCESSOR_NAME_API, PROCESSOR_NAME_ENUM}},
    {"osrt_api", {PROCESSOR_NAME_OSRT_API}},
    {"hccl", {PROCESSOR_NAME_COMMUNICATION, PROCESSOR_NAME_ENUM}},
    {"ccu", {PROCESSOR_NAME_CCU_MISSION}},
    {"dpu", {PROCESSOR_NAME_DPU}},
    {"overlap_analysis", {PROCESSOR_NAME_OVERLAP_ANALYSIS, PROCESSOR_NAME_ENUM}},
    {"ddr", {PROCESSOR_NAME_DDR}},
    {"hbm", {PROCESSOR_NAME_HBM}},
    {"llc", {PROCESSOR_NAME_LLC}},
    {"npu_mem", {PROCESSOR_NAME_NPU_MEM, PROCESSOR_NAME_NPU_OP_MEM, PROCESSOR_NAME_NPU_MODULE_MEM}},
    {"sio", {PROCESSOR_NAME_SIO}},
    {"hccs", {PROCESSOR_NAME_HCCS}},
    {"pcie", {PROCESSOR_NAME_PCIE}},
    {"nic", {PROCESSOR_NAME_NIC}},
    {"roce", {PROCESSOR_NAME_ROCE}},
    {"netdev_stats", {PROCESSOR_NAME_NETDEV_STATS}},
    {"qos", {PROCESSOR_NAME_QOS}},
    {"ub", {PROCESSOR_NAME_UB}},
    {"soc", {PROCESSOR_NAME_SOC}},
    {"acc_pmu", {PROCESSOR_NAME_ACC_PMU}},
    {"aicore_freq", {PROCESSOR_NAME_AICORE_FREQ}},
    {"sample_pmu", {PROCESSOR_NAME_SAMPLE_PMU_TIMELINE, PROCESSOR_NAME_SAMPLE_PMU_SUMMARY}},
    {"cpu_usage", {PROCESSOR_NAME_CPU_USAGE}},
    {"cpu_freq", {PROCESSOR_NAME_CPU_FREQ}},
    {"host_pmu", {PROCESSOR_NAME_HOST_PMU_COUNTER}},
    {"disk_usage", {PROCESSOR_NAME_DISK_USAGE}},
    {"network_usage", {PROCESSOR_NAME_NETWORK_USAGE}},
    {"mem_usage", {PROCESSOR_NAME_MEM_USAGE}},
    {"msproftx", {PROCESSOR_NAME_MSTX, PROCESSOR_NAME_ENUM}},
    {"ftrace", {PROCESSOR_NAME_FTRACE_GENERIC_EVENT, PROCESSOR_NAME_ENUM}},
};

// The timeline node key of one reports config key, it is empty when the key is not supported.
inline std::string GetTimelineNodeKey(const std::string &jsonKey)
{
    for (const auto &item : TIMELINE_JSON_NODE_KEYS)
    {
        if (item.first == jsonKey)
        {
            return item.second;
        }
    }
    return std::string();
}

// The whole reports config key list, it is the selection when the report config is not given.
inline std::vector<std::string> GetAllTimelineJsonKeys()
{
    std::vector<std::string> jsonKeys;
    jsonKeys.reserve(TIMELINE_JSON_NODE_KEYS.size());
    for (const auto &item : TIMELINE_JSON_NODE_KEYS)
    {
        jsonKeys.emplace_back(item.first);
    }
    return jsonKeys;
}

// The summary node key of one reports config key, it is empty when the key is not supported.
inline std::string GetSummaryNodeKey(const std::string &jsonKey)
{
    for (const auto &item : SUMMARY_JSON_NODE_KEYS)
    {
        if (item.first == jsonKey)
        {
            return item.second;
        }
    }
    return std::string();
}

// The whole summary reports config key list, it is the selection when the report config is not given.
inline std::vector<std::string> GetAllSummaryJsonKeys()
{
    std::vector<std::string> jsonKeys;
    jsonKeys.reserve(SUMMARY_JSON_NODE_KEYS.size());
    for (const auto &item : SUMMARY_JSON_NODE_KEYS)
    {
        jsonKeys.emplace_back(item.first);
    }
    return jsonKeys;
}

// The DB saver names of one reports config key, it is empty when the key is not supported. One key covers every
// saver of its data domain, the order of the names is the export order.
inline std::vector<std::string> GetDBSaverNames(const std::string &jsonKey)
{
    for (const auto &item : DB_JSON_NODE_KEYS)
    {
        if (item.first == jsonKey)
        {
            return item.second;
        }
    }
    return std::vector<std::string>();
}

// The whole DB reports config key list, it is the selection when the report config is not given.
inline std::vector<std::string> GetAllDBJsonKeys()
{
    std::vector<std::string> jsonKeys;
    jsonKeys.reserve(DB_JSON_NODE_KEYS.size());
    for (const auto &item : DB_JSON_NODE_KEYS)
    {
        jsonKeys.emplace_back(item.first);
    }
    return jsonKeys;
}
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_INCLUDE_REPORT_MAP_H
