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

#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{

namespace Infra
{

struct ProcessStatistics
{
    std::string processName;  // 以后升级到C++17或更高版本，可以使用std::string_view代替std::string
    std::vector<std::string> dependProcessNames;
    uint32_t returnCode;
    uint64_t startTime;
    uint64_t duration;  // 单位：微秒(us)
    bool mandatory;  // 是否为关键流程，关键流程失败后，会停止后续流程导致整体失败。true表示关键流程
    bool dfxStop;  // DFX需求：可以指定执行完哪个Process停止
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

struct ProcessControlOptions
{
    bool releaseUnusedData = true;
    // 0 selects min(10, hardware_concurrency()).
    uint32_t maxWorkerThreads = 0;
};

void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log);

/**
 * @brief 流程控制类
 * 本类用于控制用户通过Register注册的所有流程。支持同一个level并发运行，并提供一些DFX功能
 */
class ProcessControl final
{
   public:
    explicit ProcessControl(ProcessCollection& processes, ProcessControlOptions options = {});
    ~ProcessControl();

    bool ExecuteProcess(DataInventory& dataInventory, const Context& context);

    // 获取运行结果 key为level, value为统计结构
    ExecuteProcessStat GetExecuteStat() const;

    bool VerifyProcess(const ProcessCollection& chipRelatedProcess) const;

   private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace Infra

}  // namespace Analysis

#endif
