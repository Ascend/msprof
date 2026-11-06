/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */
#include "analysis/csrc/infrastructure/process/include/process_control.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <set>
#include <sstream>
#include <thread>

#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Infra
{
namespace
{
std::string ProcessName(const RuntimeProcessInfo& info)
{
    return info.scope.empty() ? info.processName : info.scope + "/" + info.processName;
}

bool VerifyDependencies(const RuntimeProcessCollection& processes)
{
    std::map<RuntimeProcessKey, size_t> pending;
    std::unordered_map<RuntimeProcessKey, std::vector<RuntimeProcessKey>> dependents;
    std::deque<RuntimeProcessKey> ready;
    for (const auto& item : processes)
    {
        std::set<RuntimeProcessKey> unique;
        auto dependencies = item.second.processDependence;
        dependencies.insert(dependencies.end(), item.second.completionDependence.begin(),
                            item.second.completionDependence.end());
        for (auto dependency : dependencies)
        {
            if (processes.count(dependency) == 0 || !unique.insert(dependency).second)
            {
                ERROR("Missing or duplicate runtime dependency for %.", ProcessName(item.second));
                return false;
            }
            dependents[dependency].push_back(item.first);
        }
        pending[item.first] = unique.size();
        if (unique.empty())
        {
            ready.push_back(item.first);
        }
    }
    size_t removed = 0;
    while (!ready.empty())
    {
        const auto key = ready.front();
        ready.pop_front();
        ++removed;
        for (auto child : dependents[key])
        {
            if (--pending.at(child) == 0)
            {
                ready.push_back(child);
            }
        }
    }
    if (removed != processes.size())
    {
        ERROR("Process topology contains a cycle.");
        return false;
    }
    return true;
}

void LogProcessResult(const ProcessStatistics& stat)
{
    if (stat.returnCode == ANALYSIS_OK && !stat.dfxStop)
    {
        return;
    }
    std::ostringstream details;
    details << "node=" << stat.processName << ", chip_id=" << stat.chipId << ", return_code=0x" << std::hex
            << stat.returnCode << std::dec << ", mandatory=" << std::boolalpha << stat.mandatory;
    if (stat.returnCode != ANALYSIS_OK)
    {
        ERROR("Process failed: %.", details.str());
    }
    if (stat.dfxStop)
    {
        INFO("DFX stop requested: %.", details.str());
    }
}

}  // namespace

class ProcessControl::Impl
{
   public:
    Impl(RuntimeProcessCollection processes, ProcessControlOptions options)
        : processes_(std::move(processes)), options_(options)
    {
    }

    bool Verify() const
    {
        for (const auto& item : processes_)
        {
            if (!item.second.creator || !item.second.context || !item.second.inventory)
            {
                ERROR("Invalid runtime binding for %.", ProcessName(item.second));
                return false;
            }
        }
        return VerifyDependencies(processes_);
    }

    bool Execute()
    {
        results_.clear();
        levels_.clear();
        if (!Verify())
        {
            return false;
        }
        if (processes_.empty())
        {
            return true;
        }
        std::unordered_map<RuntimeProcessKey, size_t> pending;
        std::unordered_map<RuntimeProcessKey, std::vector<RuntimeProcessKey>> dependents;
        std::deque<RuntimeProcessKey> ready;
        for (const auto& item : processes_)
        {
            results_[item.first] = RuntimeProcessResult{};
            auto dependencies = item.second.processDependence;
            dependencies.insert(dependencies.end(), item.second.completionDependence.begin(),
                                item.second.completionDependence.end());
            pending[item.first] = dependencies.size();
            levels_[item.first] = 0;
            if (pending[item.first] == 0)
            {
                ready.push_back(item.first);
            }
            for (auto dependency : dependencies)
            {
                dependents[dependency].push_back(item.first);
            }
        }
        const uint32_t hardware = std::thread::hardware_concurrency();
        const uint32_t workers =
            std::max(1U, std::min(options_.maxWorkerThreads == 0 ? (hardware == 0 ? 10U : std::min(10U, hardware))
                                                                 : options_.maxWorkerThreads,
                                  static_cast<uint32_t>(processes_.size())));
        Utils::ThreadPool pool(workers);
        if (!pool.Start())
        {
            return false;
        }
        std::mutex mutex;
        std::condition_variable changed;
        struct Completion
        {
            RuntimeProcessKey key;
            ProcessStatistics stat;
        };
        std::deque<Completion> completed;
        std::set<std::string> resources;
        std::set<std::string> activeDevices;
        size_t active = 0;
        size_t finished = 0;
        bool stopped = false;

        auto finish = [&](RuntimeProcessKey key, RuntimeProcessState state, ProcessStatistics stat)
        {
            const auto& info = processes_.at(key);
            if (results_.at(key).state == RuntimeProcessState::RUNNING)
            {
                --active;
                for (const auto& resource : info.resources)
                {
                    resources.erase(resource);
                }
            }
            if (info.closesDeviceScope)
            {
                activeDevices.erase(info.scope);
            }
            stat.processName = info.processName;
            stat.scope = info.scope;
            stat.mandatory = info.mandatory;
            for (auto dependency : info.processDependence)
            {
                const auto& predecessor = processes_.at(dependency);
                stat.dependProcessNames.push_back(ProcessName(predecessor));
            }
            results_.at(key).state = state;
            results_.at(key).statistics = std::move(stat);
            ++finished;
            if ((options_.failureMode == FailureMode::STOP_ON_MANDATORY_FAILURE &&
                 state != RuntimeProcessState::SUCCEEDED && info.mandatory) ||
                results_.at(key).statistics.dfxStop)
            {
                stopped = true;
            }
            for (auto child : dependents[key])
            {
                levels_[child] = std::max(levels_.at(child), levels_.at(key) + 1);
                if (--pending[child] == 0)
                {
                    ready.push_back(child);
                }
            }
            if (options_.releaseUnusedData)
            {
                ReleaseUnused();
            }
        };

        std::unique_lock<std::mutex> lock(mutex);
        // Join workers before their captured completion queue and synchronization objects are destroyed,
        // including when dispatch or inventory reclamation throws.
        struct DrainWorkers
        {
            Utils::ThreadPool& pool;
            std::unique_lock<std::mutex>& lock;
            ~DrainWorkers()
            {
                if (lock.owns_lock())
                {
                    lock.unlock();
                }
                pool.Stop();
            }
        } drain{pool, lock};
        while (finished < processes_.size())
        {
            while (!completed.empty())
            {
                auto completion = std::move(completed.front());
                completed.pop_front();
                const auto state = completion.stat.returnCode == ANALYSIS_OK && !completion.stat.dfxStop
                                       ? RuntimeProcessState::SUCCEEDED
                                       : RuntimeProcessState::FAILED;
                finish(completion.key, state, std::move(completion.stat));
            }
            bool progressed = false;
            const size_t readyCount = ready.size();
            for (size_t index = 0; index < readyCount; ++index)
            {
                const auto key = ready.front();
                ready.pop_front();
                const auto& info = processes_.at(key);
                const bool blocked =
                    !info.runOnDependencyFailure &&
                    std::any_of(info.processDependence.begin(), info.processDependence.end(),
                                [&](RuntimeProcessKey dependency)
                                { return results_.at(dependency).state != RuntimeProcessState::SUCCEEDED; });
                if (blocked)
                {
                    for (auto dependency : info.processDependence)
                    {
                        const auto& result = results_.at(dependency);
                        if (result.state == RuntimeProcessState::SUCCEEDED)
                        {
                            continue;
                        }
                        auto& causes = results_.at(key).failedDependencies;
                        if (result.failedDependencies.empty())
                        {
                            causes.push_back(dependency);
                        }
                        else
                        {
                            causes.insert(causes.end(), result.failedDependencies.begin(),
                                          result.failedDependencies.end());
                        }
                        std::sort(causes.begin(), causes.end());
                        causes.erase(std::unique(causes.begin(), causes.end()), causes.end());
                    }
                }
                if (blocked || stopped)
                {
                    ProcessStatistics stat{};
                    stat.returnCode = ANALYSIS_ERROR;
                    finish(key, blocked ? RuntimeProcessState::BLOCKED : RuntimeProcessState::CANCELLED, stat);
                    progressed = true;
                    continue;
                }
                const bool deviceLimit = info.opensDeviceScope && options_.maxActiveDevices != 0 &&
                                         activeDevices.count(info.scope) == 0 &&
                                         activeDevices.size() >= options_.maxActiveDevices;
                const bool busy =
                    std::any_of(info.resources.begin(), info.resources.end(),
                                [&](const std::string& resource) { return resources.count(resource) != 0; });
                if (active >= workers || deviceLimit || busy)
                {
                    ready.push_back(key);
                    continue;
                }
                if (info.opensDeviceScope)
                {
                    activeDevices.insert(info.scope);
                }
                resources.insert(info.resources.begin(), info.resources.end());
                results_.at(key).state = RuntimeProcessState::RUNNING;
                ++active;
                progressed = true;
                pool.AddTask(
                    [&, key]()
                    {
                        const auto& node = processes_.at(key);
                        ProcessStatistics stat{};
                        stat.returnCode = ANALYSIS_ERROR;
                        stat.processName = node.processName;
                        stat.mandatory = node.mandatory;
                        const auto start = std::chrono::steady_clock::now();
                        try
                        {
                            stat.chipId = node.context->GetChipID();
                            auto process = node.creator();
                            if (process)
                            {
                                stat.returnCode = process->Run(*node.inventory, *node.context);
                            }
                            const auto& stopAt = node.context->GetDfxStopAtName();
                            stat.dfxStop = !stopAt.empty() && (stopAt == node.processName ||
                                                               stopAt == node.scope + "/" + node.processName);
                        }
                        catch (const std::exception& error)
                        {
                            stat.returnCode = ANALYSIS_ERROR;
                            ERROR("Runtime process %/% failed: %.", node.scope, node.processName, error.what());
                        }
                        catch (...)
                        {
                            stat.returnCode = ANALYSIS_ERROR;
                            ERROR("Runtime process %/% failed with an unknown exception.", node.scope,
                                  node.processName);
                        }
                        stat.startTime =
                            std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
                        stat.duration = std::chrono::duration_cast<std::chrono::microseconds>(
                                            std::chrono::steady_clock::now() - start)
                                            .count();
                        LogProcessResult(stat);
                        std::lock_guard<std::mutex> guard(mutex);
                        completed.push_back({key, std::move(stat)});
                        changed.notify_one();
                    });
            }
            if (finished == processes_.size())
            {
                break;
            }
            if (!progressed)
            {
                if (active == 0)
                {
                    ERROR("Runtime graph cannot make progress; check device scope/resource lifetimes.");
                    stopped = true;
                    for (auto& result : results_)
                    {
                        if (result.second.state == RuntimeProcessState::PENDING)
                        {
                            result.second.state = RuntimeProcessState::CANCELLED;
                        }
                    }
                    break;
                }
                changed.wait(lock, [&]() { return !completed.empty(); });
            }
        }
        if (LogMandatoryFailures())
        {
            PRINT_ERROR("Processing failed. Please check the logs for details.");
        }
        return !stopped && std::all_of(results_.begin(), results_.end(),
                                       [&](const std::pair<const RuntimeProcessKey, RuntimeProcessResult>& item) {
                                           return !processes_.at(item.first).mandatory ||
                                                  item.second.state == RuntimeProcessState::SUCCEEDED;
                                       });
    }

    ExecuteProcessStat Statistics() const
    {
        ExecuteProcessStat stat{};
        std::map<size_t, OneLevelStat> levels;
        for (const auto& item : results_)
        {
            // Blocked/cancelled nodes never ran. Their states remain available in Results().
            if (item.second.state != RuntimeProcessState::SUCCEEDED && item.second.state != RuntimeProcessState::FAILED)
            {
                continue;
            }
            const size_t index = levels_.at(item.first);
            auto inserted = levels.emplace(index, OneLevelStat{true, {}});
            auto& level = inserted.first->second;
            const auto& process = item.second.statistics;
            level.processStatistics.push_back(process);
            level.generalResult = level.generalResult && (process.returnCode == ANALYSIS_OK || !process.mandatory);
        }
        for (auto& level : levels)
        {
            stat.allLevelStat.push_back(std::move(level.second));
        }
        return stat;
    }
    std::unordered_map<RuntimeProcessKey, RuntimeProcessResult> Results() const { return results_; }

   private:
    std::set<RuntimeProcessKey> ExecutionFailureCauses(RuntimeProcessKey key) const
    {
        const auto& result = results_.at(key);
        const auto failed = [](const RuntimeProcessResult& value)
        {
            // A successful stopAt callback is represented as FAILED, but is not an execution error.
            return value.state == RuntimeProcessState::FAILED && value.statistics.returnCode != ANALYSIS_OK;
        };
        std::set<RuntimeProcessKey> causes;
        if (failed(result))
        {
            causes.insert(key);
        }
        for (auto dependency : result.failedDependencies)
        {
            if (failed(results_.at(dependency)))
            {
                causes.insert(dependency);
            }
        }
        return causes;
    }

    std::string DescribeFailures(const std::set<RuntimeProcessKey>& causes) const
    {
        std::ostringstream details;
        for (auto key : causes)
        {
            if (key != *causes.begin())
            {
                details << ", ";
            }
            details << ProcessName(processes_.at(key)) << "(return_code=0x" << std::hex
                    << results_.at(key).statistics.returnCode << std::dec << ")";
        }
        return details.str();
    }

    bool LogMandatoryFailures() const
    {
        std::set<RuntimeProcessKey> requiredFailureCauses;
        std::vector<RuntimeProcessKey> stoppedNodes;
        for (const auto& node : processes_)
        {
            const auto& result = results_.at(node.first);
            if (!node.second.mandatory || result.state == RuntimeProcessState::SUCCEEDED)
            {
                continue;
            }
            const auto causes = ExecutionFailureCauses(node.first);
            requiredFailureCauses.insert(causes.begin(), causes.end());
            if (result.state != RuntimeProcessState::BLOCKED && result.state != RuntimeProcessState::CANCELLED)
            {
                continue;
            }
            if (causes.empty())
            {
                stoppedNodes.push_back(node.first);
                continue;
            }
            ERROR("Required process blocked: node=%, failed_dependencies=[%].", ProcessName(node.second),
                  DescribeFailures(causes));
        }
        if (requiredFailureCauses.empty())
        {
            return false;
        }
        for (auto key : stoppedNodes)
        {
            const auto state = results_.at(key).state == RuntimeProcessState::BLOCKED ? "BLOCKED" : "CANCELLED";
            ERROR("Required process not executed: node=%, state=%, execution_failures=[%].",
                  ProcessName(processes_.at(key)), state, DescribeFailures(requiredFailureCauses));
        }
        return true;
    }

    void ReleaseUnused()
    {
        using InventoryUse = std::pair<std::shared_ptr<DataInventory>, std::set<std::type_index>>;
        std::map<DataInventory*, InventoryUse> uses;
        for (const auto& item : processes_)
        {
            auto& use = uses[item.second.inventory.get()];
            use.first = item.second.inventory;
            const auto state = results_.at(item.first).state;
            if (state == RuntimeProcessState::PENDING || state == RuntimeProcessState::RUNNING)
            {
                use.second.insert(item.second.paramTypes.begin(), item.second.paramTypes.end());
            }
        }
        for (auto& use : uses)
        {
            use.second.first->RemoveRestData(use.second.second);
        }
    }
    RuntimeProcessCollection processes_;
    ProcessControlOptions options_;
    std::unordered_map<RuntimeProcessKey, size_t> levels_;
    std::unordered_map<RuntimeProcessKey, RuntimeProcessResult> results_;
};

void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log)
{
    std::stringstream ss;
    try
    {
        ss.imbue(std::locale(""));
    }
    catch (const std::runtime_error&)
    {
        ss.imbue(std::locale::classic());
    }
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
            const auto name = proc.scope.empty() ? proc.processName : proc.scope + "/" + proc.processName;
            ss << "\tprocess" << j++ << "[" << name << "]:" << "return: 0x" << std::hex << proc.returnCode << std::dec
               << ", mandatory:" << std::boolalpha << proc.mandatory << ", dfxStop:" << proc.dfxStop << std::noboolalpha
               << std::endl;
            if (!proc.scope.empty())
            {
                ss << "\t\tchip id: " << proc.chipId << std::endl;
            }
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

ProcessControl::ProcessControl(RuntimeProcessCollection& processes, ProcessControlOptions options)
    : impl_(new Impl(std::move(processes), options))
{
}

ProcessControl::~ProcessControl() = default;

bool ProcessControl::ExecuteProcess() { return impl_->Execute(); }

ExecuteProcessStat ProcessControl::GetExecuteStat() const { return impl_->Statistics(); }

std::unordered_map<RuntimeProcessKey, RuntimeProcessResult> ProcessControl::GetRuntimeResults() const
{
    return impl_->Results();
}

}  // namespace Infra
}  // namespace Analysis
