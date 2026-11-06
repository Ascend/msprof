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
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_CONTROL_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_CONTROL_H

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "analysis/csrc/infrastructure/process/include/process_struct.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{

namespace Infra
{

struct ProcessStatistics
{
    std::string scope;
    uint32_t chipId = 0;
    std::string processName;  // 以后升级到C++17或更高版本，可以使用std::string_view代替std::string
    std::vector<std::string> dependProcessNames;
    uint32_t returnCode;
    uint64_t startTime;
    uint64_t duration;  // 单位：微秒(us)
    bool mandatory;     // Failure contributes to the result; dispatch policy is controlled by FailureMode.
    bool dfxStop;       // DFX需求：可以指定执行完哪个Process停止
};

struct OneLevelStat
{
    bool generalResult;
    std::vector<ProcessStatistics> processStatistics;
};

struct ExecuteProcessStat
{
    uint32_t chipId;
    std::vector<OneLevelStat> allLevelStat;
};

enum class FailureMode
{
    STOP_ON_MANDATORY_FAILURE,
    CONTINUE_INDEPENDENT
};

struct ProcessControlOptions
{
    bool releaseUnusedData = true;
    // 0 selects min(10, hardware_concurrency()).
    uint32_t maxWorkerThreads = 0;
    uint32_t maxActiveDevices = 0;
    FailureMode failureMode = FailureMode::STOP_ON_MANDATORY_FAILURE;
};

enum class RuntimeProcessState
{
    PENDING,
    RUNNING,
    SUCCEEDED,
    FAILED,
    BLOCKED,
    CANCELLED
};

struct RuntimeProcessResult
{
    RuntimeProcessState state = RuntimeProcessState::PENDING;
    ProcessStatistics statistics{};
    std::vector<RuntimeProcessKey> failedDependencies;
};

void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log);

/**
 * @brief 流程控制类
 * 调度已绑定上下文和数据仓库的运行时拓扑。支持并发执行、资源约束和DFX统计。
 */
class ProcessControl final
{
   public:
    explicit ProcessControl(RuntimeProcessCollection& processes, ProcessControlOptions options = {});
    ~ProcessControl();
    ProcessControl(const ProcessControl&) = delete;
    ProcessControl& operator=(const ProcessControl&) = delete;

    bool ExecuteProcess();
    std::unordered_map<RuntimeProcessKey, RuntimeProcessResult> GetRuntimeResults() const;

    // 按拓扑层级统计实际执行的节点；未执行节点的状态通过GetRuntimeResults获取。
    ExecuteProcessStat GetExecuteStat() const;

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace Infra

}  // namespace Analysis

#endif
