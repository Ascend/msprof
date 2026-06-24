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

#ifndef MSPROF_ANALYSIS_METADATA_EVENT_H
#define MSPROF_ANALYSIS_METADATA_EVENT_H

#include <vector>

#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/domain/entities/json_trace/include/trace_event.h"

namespace Analysis
{
namespace Domain
{
// sort_index
enum ProcessSortIndex
{
    LAYER_FRAMEWORK_SORT = 6,
    LAYER_CANN_SORT,
    LAYER_DPU_SORT,
    LAYER_CPU_USAGE_SORT,
    LAYER_MEMORY_USAGE_SORT,
    LAYER_NETWORK_USAGE_SORT,
    LAYER_DISK_USAGE_SORT,
    LAYER_OS_RUNTIME_API_SORT,
    LAYER_ASCEND_HW_SORT,
    LAYER_ASCEND_AICORE_FREQ_SORT,
    LAYER_HCCL,
    LAYER_NPU_MEM,
    LAYER_DDR,
    LAYER_ACC_PMU,
    LAYER_HCCS,
    LAYER_NIC,
    LAYER_ROCE,
    LAYER_PCIE,
    LAYER_HBM,
    LAYER_STARS_SOC,
    LAYER_STARS_CHIP_TRANS,
    LAYER_NPU_LLC,
    LAYER_NPU_SIO,
    LAYER_NPU_QOS,
    LAYER_CCU,
    LAYER_OVERLAP_ANALYSIS,
    LAYER_BIU_PERF,
    LAYER_UB,
    LAYER_BLOCK_DETAIL,
    LAYER_FUSION_TASK,
    RESERVED
};
// 组件名称
const std::string COMPONENT_LAYER_FRAMEWORK = "PID Name";
const std::string COMPONENT_LAYER_CANN = "CANN";
const std::string COMPONENT_LAYER_ASCEND_HW = "Ascend Hardware";
const std::string LABEL_CPU = "CPU";
const std::string LABEL_NPU = "NPU";
const std::string LABEL_DPU = "DPU";
// layer name
const std::string PROCESS_RUNTIME = "Runtime";
const std::string PROCESS_AI_CORE_UTILIZATION = "AI Core Utilization";
const std::string PROCESS_ACL = "AscendCL";
const std::string PROCESS_AI_CPU = "AI CPU";
const std::string PROCESS_ALL_REDUCE = "All Reduce";
const std::string PROCESS_GE = "GE";
const std::string PROCESS_TASK = "Task Scheduler";
const std::string PROCESS_STEP_TRACE = "Step Trace";
const std::string PROCESS_TRAINING_TRACE = "Training Trace";
const std::string PROCESS_MSPROFTX = "MsprofTx";
const std::string PROCESS_SUBTASK = "Subtask Time";
const std::string PROCESS_THREAD_TASK = "Thread Task Time";
const std::string PROCESS_OVERLAP_ANALYSE = "Overlap Analysis";
const std::string PROCESS_API = "Api";
const std::string PROCESS_EVENT = "Event";
const std::string PROCESS_CPU_USAGE = "CPU Usage";
const std::string PROCESS_MEMORY_USAGE = "Memory Usage";
const std::string PROCESS_NETWORK_USAGE = "Network Usage";
const std::string PROCESS_DISK_USAGE = "Disk Usage";
const std::string PROCESS_OS_RUNTIME_API = "OS Runtime API";
const std::string PROCESS_AI_CORE_FREQ = "AI Core Freq";
const std::string PROCESS_LOW_POWER = "Low Power";
const std::string PROCESS_HCCL = "Communication";
const std::string PROCESS_CCU = "CCU";
const std::string PROCESS_NPU_MEM = "NPU MEM";
const std::string PROCESS_DDR = "DDR";
const std::string PROCESS_ACC_PMU = "Acc PMU";
const std::string PROCESS_HCCS = "HCCS";
const std::string PROCESS_NIC = "NIC";
const std::string PROCESS_ROCE = "RoCE";
const std::string PROCESS_PCIE = "PCIe";
const std::string PROCESS_HBM = "HBM";
const std::string PROCESS_STARS_SOC = "Stars Soc Info";
const std::string PROCESS_STARS_CHIP_TRANS = "Stars Chip Trans";
const std::string PROCESS_LLC = "LLC";
const std::string PROCESS_SIO = "SIO";
const std::string PROCESS_QOS = "QoS";
const std::string PROCESS_DEVICE_TX = "DEVICE_TX";
const std::string PROCESS_BIU_PERF = "Biu Perf";
const std::string PROCESS_UB = "Ub";
const std::string PROCESS_BLOCK_DETAIL = "Block Detail";
const std::string PROCESS_DPU = "DPU";
const std::string PROCESS_FUSION_TASK = "Fusion Task";

struct LayerInfo
{
    std::string component;
    std::string label;
    uint32_t sortIndex;
};
const std::unordered_map<std::string, LayerInfo> LAYER_INFO{
    {PROCESS_MSPROFTX, {COMPONENT_LAYER_FRAMEWORK, LABEL_CPU, LAYER_FRAMEWORK_SORT}},
    {PROCESS_ACL, {COMPONENT_LAYER_CANN, LABEL_CPU, LAYER_CANN_SORT}},
    {PROCESS_GE, {COMPONENT_LAYER_CANN, LABEL_CPU, LAYER_CANN_SORT}},
    {PROCESS_RUNTIME, {COMPONENT_LAYER_CANN, LABEL_CPU, LAYER_CANN_SORT}},
    {PROCESS_TASK, {COMPONENT_LAYER_ASCEND_HW, LABEL_NPU, LAYER_ASCEND_HW_SORT}},
    {PROCESS_STEP_TRACE, {COMPONENT_LAYER_ASCEND_HW, LABEL_NPU, LAYER_ASCEND_HW_SORT}},
    {PROCESS_API, {COMPONENT_LAYER_CANN, LABEL_CPU, LAYER_CANN_SORT}},
    {PROCESS_EVENT, {COMPONENT_LAYER_CANN, LABEL_CPU, LAYER_CANN_SORT}},
    {PROCESS_CPU_USAGE, {PROCESS_CPU_USAGE, LABEL_CPU, LAYER_CPU_USAGE_SORT}},
    {PROCESS_MEMORY_USAGE, {PROCESS_MEMORY_USAGE, LABEL_CPU, LAYER_MEMORY_USAGE_SORT}},
    {PROCESS_NETWORK_USAGE, {PROCESS_NETWORK_USAGE, LABEL_CPU, LAYER_NETWORK_USAGE_SORT}},
    {PROCESS_DISK_USAGE, {PROCESS_DISK_USAGE, LABEL_CPU, LAYER_DISK_USAGE_SORT}},
    {PROCESS_OS_RUNTIME_API, {PROCESS_OS_RUNTIME_API, LABEL_CPU, LAYER_OS_RUNTIME_API_SORT}},
    {PROCESS_AI_CORE_FREQ, {PROCESS_AI_CORE_FREQ, LABEL_NPU, LAYER_ASCEND_AICORE_FREQ_SORT}},
    {PROCESS_HCCL, {PROCESS_HCCL, LABEL_NPU, LAYER_HCCL}},
    {PROCESS_CCU, {PROCESS_CCU, LABEL_NPU, LAYER_CCU}},
    {PROCESS_NPU_MEM, {PROCESS_NPU_MEM, LABEL_NPU, LAYER_NPU_MEM}},
    {PROCESS_DDR, {PROCESS_DDR, LABEL_NPU, LAYER_DDR}},
    {PROCESS_ACC_PMU, {PROCESS_ACC_PMU, LABEL_NPU, LAYER_ACC_PMU}},
    {PROCESS_HCCS, {PROCESS_HCCS, LABEL_NPU, LAYER_HCCS}},
    {PROCESS_NIC, {PROCESS_NIC, LABEL_NPU, LAYER_NIC}},
    {PROCESS_ROCE, {PROCESS_ROCE, LABEL_NPU, LAYER_ROCE}},
    {PROCESS_PCIE, {PROCESS_PCIE, LABEL_NPU, LAYER_PCIE}},
    {PROCESS_HBM, {PROCESS_HBM, LABEL_NPU, LAYER_HBM}},
    {PROCESS_STARS_SOC, {PROCESS_STARS_SOC, LABEL_NPU, LAYER_STARS_SOC}},
    {PROCESS_STARS_CHIP_TRANS, {PROCESS_STARS_CHIP_TRANS, LABEL_NPU, LAYER_STARS_CHIP_TRANS}},
    {PROCESS_LLC, {PROCESS_LLC, LABEL_NPU, LAYER_NPU_LLC}},
    {PROCESS_SIO, {PROCESS_SIO, LABEL_NPU, LAYER_NPU_SIO}},
    {PROCESS_QOS, {PROCESS_QOS, LABEL_NPU, LAYER_NPU_QOS}},
    {PROCESS_OVERLAP_ANALYSE, {PROCESS_OVERLAP_ANALYSE, LABEL_NPU, LAYER_OVERLAP_ANALYSIS}},
    {PROCESS_BIU_PERF, {PROCESS_BIU_PERF, LABEL_NPU, LAYER_BIU_PERF}},
    {PROCESS_UB, {PROCESS_UB, LABEL_NPU, LAYER_UB}},
    {PROCESS_BLOCK_DETAIL, {PROCESS_BLOCK_DETAIL, LABEL_NPU, LAYER_BLOCK_DETAIL}},
    {PROCESS_DPU, {COMPONENT_LAYER_CANN, LABEL_DPU, LAYER_DPU_SORT}},
    {PROCESS_FUSION_TASK, {PROCESS_FUSION_TASK, LABEL_NPU, LAYER_FUSION_TASK}},
};

LayerInfo GetLayerInfo(std::string processName);

/**
 * MetaDataEvent对应chrome:tracing的元数据相位(MetaData Phase)，在事件跟踪中，每个事件都有一个与之相关的相位（Phase），
 * 这些相位提供了关于事件的不同方面的信息。常见的相位包括：
 *      B（Begin）：事件的开始。
 *      E（End）：事件的结束。
 *      X：完成（Complete），表示事件已经完成。
 *      M（MetaData）：提供事件的元数据信息。
 * MetaData
 * Phase主要用于提供事件的额外信息，这些信息对于理解事件的上下文和性质非常重要。例如，它可能包含事件的类型、来源、
 * 相关参数等。通过查看MetaData
 * Phase，可以获得关于事件的更多背景和详细信息，从而更深入地分析事件的行为和影响‌
 */
class MetaDataEvent : public TraceEvent
{
   public:
    MetaDataEvent(uint32_t pid, int tid, const std::string &name);

   private:
    void ToJson(JsonWriter &ostream) override;
    virtual void ProcessArgs(JsonWriter &ostream) {};

   private:
    std::string ph_ = "M";
};

// args为name参数的MetaData
class MetaDataNameEvent : public MetaDataEvent
{
   public:
    MetaDataNameEvent(uint32_t pid, int tid, const std::string &name, const std::string &argName);

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    std::string argsName_;
};

// args为labels参数的MetaData
class MetaDataLabelEvent : public MetaDataEvent
{
   public:
    MetaDataLabelEvent(uint32_t pid, int tid, const std::string &name, const std::string &label);

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    std::string argsLabel_;
};

// args为index参数的MetaData
class MetaDataIndexEvent : public MetaDataEvent
{
   public:
    MetaDataIndexEvent(uint32_t pid, int tid, const std::string &name, int index);

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    int argsSortIndex_;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_METADATA_EVENT_H
