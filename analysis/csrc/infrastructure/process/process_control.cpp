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
#include "analysis/csrc/infrastructure/process/include/process_control.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/process_topo.h"

namespace Analysis
{

namespace Infra
{
namespace
{

void GetDepProcessNames(const RegProcessInfo& procInfo, const ProcessCollection& allRegProcess,
                        std::vector<std::string>& depProcNames)
{
    for (const auto& typeIndex : procInfo.processDependence)
    {
        auto it = allRegProcess.find(typeIndex);
        if (it == allRegProcess.end())
        {
            continue;
        }
        depProcNames.emplace_back(it->second.processName);
    }
}

void FillStatisticianDependence(const ProcessCollection& regProcess, std::vector<ProcessStatistics>& stat)
{
    for (auto& node : stat)
    {
        for (const auto& regPair : regProcess)
        {
            if (node.processName == regPair.second.processName)
            {
                GetDepProcessNames(regPair.second, regProcess, node.dependProcessNames);
            }
        }
    }
}

}  // namespace

class ProcessControl::Impl final
{
   public:
    explicit Impl(ProcessCollection& processes, ProcessControlOptions options)
        : allProcess_(std::move(processes)), options_(options)
    {
        stat_.chipId = 0;
    }
    ~Impl() = default;

    bool VerifyProcess(const ProcessCollection& chipRelatedProcess) const
    {
        auto verifyProcess = chipRelatedProcess;
        auto preparedProcess = TakeAwayPreparedProcess(verifyProcess);
        while (!preparedProcess.empty())
        {
            preparedProcess = TakeAwayPreparedProcess(verifyProcess);
        }
        return verifyProcess.empty();
    }

    bool ExecuteProcess(DataInventory& dataInventory, const Context& context)
    {
        chipId_ = context.GetChipID();
        stat_.chipId = chipId_;

        ProcessTopo processTopoBuilder(allProcess_);
        auto chipRelatedProcess = processTopoBuilder.BuildProcessControlTopoByChip(chipId_);
        if (!VerifyProcess(chipRelatedProcess))
        {
            ERROR("Topo Verify failed!");
            return false;
        }

        return RunProcesses(chipRelatedProcess, dataInventory, context);
    }

    ExecuteProcessStat GetExecuteStat() const { return stat_; }

   private:
    /**
     * @brief 拓扑排序算法执行所有Process 使用的算法为卡恩算法
     * @param chipRelatedProcess 相应Chip的Process集合
     */
    bool RunProcesses(ProcessCollection& chipRelatedProcess, DataInventory& dataInventory, const Context& context)
    {
        const ProcessCollection regProcessCopy = chipRelatedProcess;
        if (chipRelatedProcess.empty())
        {
            return true;
        }

        using ProcessKey = std::type_index;
        struct CompletedProcess
        {
            ProcessKey key;
            size_t level;
            ProcessStatistics statistics;
        };

        std::unordered_map<ProcessKey, size_t> unresolvedDependencies;
        std::unordered_map<ProcessKey, std::vector<ProcessKey>> dependents;
        std::unordered_map<ProcessKey, size_t> levels;
        std::vector<ProcessKey> readyProcesses;
        for (const auto& processPair : chipRelatedProcess)
        {
            const ProcessKey& key = processPair.first;
            unresolvedDependencies.emplace(key, processPair.second.processDependence.size());
            levels.emplace(key, 0UL);
            if (processPair.second.processDependence.empty())
            {
                readyProcesses.emplace_back(key);
            }
            for (const auto& dependency : processPair.second.processDependence)
            {
                dependents[dependency].emplace_back(key);
            }
        }

        const uint32_t hardwareWorkerCount = std::thread::hardware_concurrency();
        const uint32_t defaultWorkerCount = hardwareWorkerCount == 0 ? 10U : std::min(10U, hardwareWorkerCount);
        const uint32_t configuredWorkerCount =
            options_.maxWorkerThreads == 0 ? defaultWorkerCount : options_.maxWorkerThreads;
        const uint32_t workerCount =
            std::max(1U, std::min(configuredWorkerCount, static_cast<uint32_t>(chipRelatedProcess.size())));
        Analysis::Utils::ThreadPool pool(workerCount);
        if (!pool.Start())
        {
            ERROR("Start process worker pool failed.");
            return false;
        }

        std::mutex schedulerMutex;
        std::condition_variable schedulerDone;
        ProcessCollection remainingProcesses = chipRelatedProcess;
        std::vector<CompletedProcess> completedProcesses;
        size_t activeTaskCount = 0;
        size_t completedTaskCount = 0;
        size_t releaseIndex = 0;
        bool stopScheduling = false;

        std::function<void(const ProcessKey&)> schedule;
        schedule = [&](const ProcessKey& key)
        {
            const auto processIter = chipRelatedProcess.find(key);
            if (processIter == chipRelatedProcess.end())
            {
                ERROR("Scheduled process is not registered.");
                stopScheduling = true;
                return;
            }
            ++activeTaskCount;
            const RegProcessInfo processInfo = processIter->second;
            const size_t level = levels.at(key);
            pool.AddTask(
                [&, key, level, processInfo]()
                {
                    ProcessStatistics statistics{};
                    const auto startTime = std::chrono::steady_clock::now();
                    if (!processInfo.creator)
                    {
                        ERROR("creator==nullptr, process name: %", processInfo.processName);
                    }
                    else
                    {
                        try
                        {
                            auto proc = processInfo.creator();
                            if (proc != nullptr)
                            {
                                statistics.returnCode = proc->Run(dataInventory, context);
                                statistics.mandatory = processInfo.mandatory;
                            }
                        }
                        catch (const std::exception& exception)
                        {
                            ERROR("Process % threw an exception: %", processInfo.processName, exception.what());
                            statistics.returnCode = ANALYSIS_ERROR;
                            statistics.mandatory = processInfo.mandatory;
                        }
                        catch (...)
                        {
                            ERROR("Process % threw an unknown exception.", processInfo.processName);
                            statistics.returnCode = ANALYSIS_ERROR;
                            statistics.mandatory = processInfo.mandatory;
                        }
                    }
                    const auto endTime = std::chrono::steady_clock::now();
                    statistics.startTime =
                        std::chrono::duration_cast<std::chrono::microseconds>(startTime.time_since_epoch()).count();
                    statistics.duration =
                        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                    statistics.processName = processInfo.processName;
                    const std::string& stopProcessName = context.GetDfxStopAtName();
                    statistics.dfxStop = !stopProcessName.empty() && stopProcessName == processInfo.processName;

                    std::unique_lock<std::mutex> lock(schedulerMutex);
                    --activeTaskCount;
                    ++completedTaskCount;
                    completedProcesses.push_back({key, level, std::move(statistics)});
                    const ProcessStatistics& completedStat = completedProcesses.back().statistics;
                    remainingProcesses.erase(key);
                    if (options_.releaseUnusedData)
                    {
                        ReleaseNoLongerUsedData(remainingProcesses, dataInventory, releaseIndex++);
                    }

                    if ((completedStat.returnCode != ANALYSIS_OK && completedStat.mandatory) || completedStat.dfxStop)
                    {
                        stopScheduling = true;
                    }
                    if (!stopScheduling)
                    {
                        const auto dependentIter = dependents.find(key);
                        if (dependentIter != dependents.end())
                        {
                            for (const auto& dependent : dependentIter->second)
                            {
                                size_t& pendingCount = unresolvedDependencies.at(dependent);
                                levels.at(dependent) = std::max(levels.at(dependent), level + 1);
                                if (--pendingCount == 0)
                                {
                                    schedule(dependent);
                                }
                            }
                        }
                    }
                    schedulerDone.notify_all();
                });
        };

        {
            std::unique_lock<std::mutex> lock(schedulerMutex);
            for (const auto& readyProcess : readyProcesses)
            {
                schedule(readyProcess);
            }
        }
        {
            std::unique_lock<std::mutex> lock(schedulerMutex);
            schedulerDone.wait(lock,
                               [&]() {
                                   return activeTaskCount == 0 &&
                                          (stopScheduling || completedTaskCount == chipRelatedProcess.size());
                               });
        }
        pool.Stop();

        std::map<size_t, std::vector<ProcessStatistics>> statisticsByLevel;
        for (auto& completed : completedProcesses)
        {
            statisticsByLevel[completed.level].emplace_back(std::move(completed.statistics));
        }
        bool overallResult = !stopScheduling;
        for (auto& levelStatistics : statisticsByLevel)
        {
            FillStatisticianDependence(regProcessCopy, levelStatistics.second);
            bool dfxStop = false;
            const bool levelResult = GetStatistician(std::move(levelStatistics.second), dfxStop);
            overallResult = overallResult && levelResult && !dfxStop;
        }
        if (stopScheduling)
        {
            PRINT_INFO("!!!!!!! DFX Stops or a mandatory process failed !!!!!!!!");
        }
        return overallResult && completedTaskCount == chipRelatedProcess.size();
    }

    bool GetStatistician(std::vector<ProcessStatistics>&& statistics, bool& dfxStop)
    {
        auto generalResult = std::all_of(statistics.begin(), statistics.end(), [](ProcessStatistics& node)
                                         { return (node.returnCode == 0 || !node.mandatory); });
        dfxStop =
            std::any_of(statistics.begin(), statistics.end(), [](ProcessStatistics& node) { return node.dfxStop; });
        stat_.allLevelStat.emplace_back();
        auto& oneLevelStat = stat_.allLevelStat.back();
        oneLevelStat.generalResult = generalResult;
        oneLevelStat.processStatistics = std::move(statistics);
        return generalResult;  // Stop on error  这里是否停止还与Process类注册时，注册宏中mandatory字段确定
    }

    void ReleaseNoLongerUsedData(const ProcessCollection& chipRelatedProcess, DataInventory& dataInventory,
                                 size_t levelIndex)
    {
        std::set<std::type_index> dataTobeUsing;
        for (const auto& processInfo : chipRelatedProcess)
        {
            for (const auto& dataType : processInfo.second.paramTypes)
            {
                dataTobeUsing.insert(dataType);
            }
        }
        auto removedTypes = dataInventory.RemoveRestData(dataTobeUsing);

        std::string typeStr;
        for (const auto& typeInfo : removedTypes)
        {
            typeStr += typeInfo.name();
            typeStr += " ";
        }
        INFO("Level[%]Release Data Types: %", levelIndex, typeStr);
    }

    ProcessCollection TakeAwayPreparedProcess(ProcessCollection& chipRelatedProcess) const
    {
        ProcessCollection preparedProcess;
        // 先拿走已经没有前向依赖的，即入度为0的节点
        for (auto it = chipRelatedProcess.begin(); it != chipRelatedProcess.end();)
        {
            if (it->second.processDependence.empty())
            {
                preparedProcess.insert({it->first, it->second});
                it = chipRelatedProcess.erase(it);
                continue;
            }
            ++it;
        }

        // 再将其它节点中的前向依赖删除，即修正其它节点的入度
        for (auto& process : chipRelatedProcess)
        {
            auto& processDep = process.second.processDependence;
            for (const auto& preparedNode : preparedProcess)
            {
                processDep.erase(std::remove(processDep.begin(), processDep.end(), preparedNode.first),
                                 processDep.end());
            }
        }
        return preparedProcess;
    }

   private:
    ProcessCollection allProcess_;
    ProcessControlOptions options_;
    ExecuteProcessStat stat_;  // dfx: 统计运行结果
    uint32_t chipId_{};        // dfx: 记录运行什么芯片ID
};

void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << "===============================================================================" << std::endl;
    ss << "deivce file in " << subDir << std::endl;

    auto levelCount = stat.allLevelStat.size();
    ss << "chip id: " << stat.chipId << ", process levels:" << levelCount << std::endl;
    for (size_t i = 0; i < levelCount; ++i)
    {
        const auto& node = stat.allLevelStat[i];
        ss << "-------------------------------------------------------------------------------" << std::endl;
        ss << "level[" << i << "] generalResult:" << std::boolalpha << node.generalResult << std::noboolalpha
           << ", process num:" << node.processStatistics.size() << std::endl;
        size_t j = 0;
        for (const auto& proc : node.processStatistics)
        {
            ss << "\tprocess" << j++ << "[" << proc.processName << "]:" << "return: 0x" << std::hex << proc.returnCode
               << std::dec << ", mandatory:" << std::boolalpha << proc.mandatory << std::noboolalpha << std::endl;
            ss << "\t\tdependProcessNames: ";
            for (const auto& depName : proc.dependProcessNames)
            {
                ss << depName << ", ";
            }
            ss << std::endl
               << "\t\tstartTime:" << proc.startTime << " us, duration: " << proc.duration << " us" << std::endl;
        }
    }
    ss << std::endl;
    log += ss.str();
}

ProcessControl::ProcessControl(ProcessCollection& processes, ProcessControlOptions options)
    : impl_(new(std::nothrow) Impl(processes, options))
{
}

ProcessControl::~ProcessControl() = default;

bool ProcessControl::ExecuteProcess(DataInventory& dataInventory, const Context& context)
{
    if (!impl_)
    {
        ERROR("ProcessControl Impl create failed!");
        return false;
    };
    return impl_->ExecuteProcess(dataInventory, context);
}

// 获取运行结果 key为level, value为统计结构
ExecuteProcessStat ProcessControl::GetExecuteStat() const
{
    if (!impl_)
    {
        ERROR("ProcessControl Impl create failed!");
        return {};
    };
    return impl_->GetExecuteStat();
}

bool ProcessControl::VerifyProcess(const ProcessCollection& chipRelatedProcess) const
{
    if (!impl_)
    {
        ERROR("ProcessControl Impl create failed!");
        return false;
    };
    return impl_->VerifyProcess(chipRelatedProcess);
}

}  // namespace Infra

}  // namespace Analysis
