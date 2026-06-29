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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/data_process/ai_task/overlap_analysis_processor.h"

#include <algorithm>
#include <utility>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Application;
namespace
{
const std::set<std::string> FILTER_TYPE = {"KERNEL_AICORE",  "KERNEL_AIVEC",     "FFTS_PLUS",        "KERNEL_MIX_AIC",
                                           "KERNEL_MIX_AIV", "PROFILING_ENABLE", "PROFILING_DISABLE"};

void SepOneTask(std::vector<TimeDuration> &times, std::set<uint16_t> &mc2StreamsTable, TaskInfoData &task,
                std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections)
{
    if (mc2StreamsTable.find(task.streamId) != mc2StreamsTable.end() || EndsWith(task.opName, AICPU_KERNEL) ||
        EndsWith(task.opName, AIV_KERNEL))
    {
        return;
    }
    for (auto &timeDur : times)
    {
        compSections[task.deviceId].emplace_back(timeDur);
    }
}
}  // namespace

OverlapAnalysisProcessor::OverlapAnalysisProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool OverlapAnalysisProcessor::Process(DataInventory &dataInventory)
{
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (!taskData)
    {
        WARN("No any task data found");
        return true;
    }
    auto computeTaskData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    if (!computeTaskData)
    {
        WARN("No compute task data found");
    }
    auto commOpData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    if (!commOpData)
    {
        WARN("No comm task data found");
    }
    auto kfcOpData = dataInventory.GetPtr<std::vector<KfcOpData>>();
    if (!kfcOpData)
    {
        WARN("No kfc op data found");
    }
    auto mc2CommInfoData = dataInventory.GetPtr<std::vector<MC2CommInfoData>>();
    if (!mc2CommInfoData)
    {
        WARN("No kfc comm info found");
    }

    RecordCompAndCommTaskTime(taskData, computeTaskData, commOpData, kfcOpData, mc2CommInfoData);
    auto overlapData = BuildOverlapAnalysisData();
    if (overlapData.empty())
    {
        WARN("No overlap analysis data generated.");
        return true;
    }
    return SaveToDataInventory<OverlapAnalysisData>(std::move(overlapData), dataInventory,
                                                    PROCESSOR_NAME_OVERLAP_ANALYSIS);
}

void OverlapAnalysisProcessor::RecordCompAndCommTaskTime(
    const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks,
    const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
    const std::shared_ptr<std::vector<CommunicationOpData>> &commOps,
    const std::shared_ptr<std::vector<KfcOpData>> &kfcOps,
    const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos)
{
    std::map<TaskId, std::vector<TimeDuration>> allTaskPool;
    if (ascendTasks)
    {
        for (auto &task : *ascendTasks)
        {
            TaskId id{static_cast<uint16_t>(task.streamId), static_cast<uint16_t>(task.batchId), task.taskId,
                      task.contextId, task.deviceId};
            TimeDuration timePair{task.timestamp, task.timestamp + static_cast<uint64_t>(task.duration)};
            if (allTaskPool.find(id) != allTaskPool.end())
            {
                allTaskPool[id].emplace_back(timePair);
            }
            else
            {
                allTaskPool[id] = {timePair};
            }
            deviceIds_.insert(task.deviceId);
        }
    }

    std::unordered_map<uint16_t, std::vector<TimeDuration>> compSections;
    SepCompTaskAndKFCCommSections(allTaskPool, compTasks, mc2CommInfos, compSections);
    std::unordered_map<uint16_t, std::vector<TimeDuration>> tradCommSections;
    GetCommTaskSections(tradCommSections, commOps);
    GetCommTaskSections(tradCommSections, kfcOps);
    UpdateTaskTimeExtremes(ascendTasks);

    for (auto &pair : tradCommSections)
    {
        TimeLogger logger("sort and union all trad comm task in overlap analysis for device " +
                          std::to_string(pair.first));
        std::sort(pair.second.begin(), pair.second.end());
        commTaskRecords_[pair.first] = UnionOneSet(pair.second);
    }
    for (auto &pair : compSections)
    {
        TimeLogger logger("sort and union all comp task in overlap analysis for device " + std::to_string(pair.first));
        std::sort(pair.second.begin(), pair.second.end());
        compTaskRecords_[pair.first] = UnionOneSet(pair.second);
    }
}

void OverlapAnalysisProcessor::SepCompTaskAndKFCCommSections(
    std::map<TaskId, std::vector<TimeDuration>> &allTaskPool,
    const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
    const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos,
    std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections)
{
    if (!compTasks)
    {
        return;
    }
    std::set<uint16_t> mc2StreamsTable;
    if (mc2CommInfos)
    {
        for (auto &mc2CommInfo : *mc2CommInfos)
        {
            mc2StreamsTable.insert(mc2CommInfo.aiCpuKfcStreamId);
        }
    }
    uint64_t mismatchCount = 0;
    std::set<TaskId> usedTaskIds;
    for (auto &task : *compTasks)
    {
        TaskId id{static_cast<uint16_t>(task.streamId), static_cast<uint16_t>(task.batchId), task.taskId,
                  task.contextId, task.deviceId};
        if (usedTaskIds.find(id) != usedTaskIds.end())
        {
            continue;
        }
        usedTaskIds.insert(id);
        auto it = allTaskPool.find(id);
        if (it != allTaskPool.end())
        {
            SepOneTask(it->second, mc2StreamsTable, task, compSections);
        }
        else
        {
            mismatchCount++;
        }
    }
    INFO("Find % comp tasks not in all tasks.", mismatchCount);
}

template <typename T>
void OverlapAnalysisProcessor::GetCommTaskSections(
    std::unordered_map<uint16_t, std::vector<TimeDuration>> &commOpSections,
    const std::shared_ptr<std::vector<T>> &commOps)
{
    if (!commOps)
    {
        return;
    }
    for (auto &op : *commOps)
    {
        deviceIds_.insert(op.deviceId);
        commOpSections[op.deviceId].emplace_back(op.timestamp, op.end);
    }
}

void OverlapAnalysisProcessor::UpdateTaskTimeExtremes(const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks)
{
    for (const auto &deviceId : deviceIds_)
    {
        end_[deviceId] = 0;
        begin_[deviceId] = UINT64_MAX;
    }
    for (const AscendTaskData &task : *ascendTasks)
    {
        if (FILTER_TYPE.find(task.hostType) != FILTER_TYPE.end())
        {
            continue;
        }
        uint64_t taskStartTime = task.timestamp;
        uint64_t taskEndTime = task.timestamp + static_cast<uint64_t>(task.duration);
        uint16_t deviceId = task.deviceId;
        if (taskEndTime > end_[deviceId])
        {
            end_[deviceId] = taskEndTime;
        }
        if (taskStartTime < begin_[deviceId])
        {
            begin_[deviceId] = taskStartTime;
        }
    }
}

void OverlapAnalysisProcessor::UpdateTimeExtremesBySections(uint16_t deviceId,
                                                            const std::vector<TimeDuration> &sections)
{
    for (auto &section : sections)
    {
        begin_[deviceId] = std::min(begin_[deviceId], section.start);
        end_[deviceId] = std::max(end_[deviceId], section.end);
    }
}

std::vector<OverlapAnalysisData> OverlapAnalysisProcessor::BuildOverlapAnalysisData()
{
    std::vector<OverlapAnalysisData> overlapData;
    if (!Utils::Reserve(overlapData, deviceIds_.size()))
    {
        ERROR("Reserve overlap analysis data error");
        return {};
    }
    for (auto &deviceId : deviceIds_)
    {
        auto computeSections = compTaskRecords_.find(deviceId) == compTaskRecords_.end() ? std::vector<TimeDuration>()
                                                                                         : compTaskRecords_[deviceId];
        auto communicationSections = commTaskRecords_.find(deviceId) == commTaskRecords_.end()
                                         ? std::vector<TimeDuration>()
                                         : commTaskRecords_[deviceId];
        auto commNotOverlapCompSections = GetDifferenceSet(communicationSections, computeSections);
        UpdateTimeExtremesBySections(deviceId, computeSections);
        UpdateTimeExtremesBySections(deviceId, communicationSections);
        AppendEvents(deviceId, computeSections, OverlapAnalysisType::COMPUTE, overlapData);
        AppendEvents(deviceId, communicationSections, OverlapAnalysisType::COMMUNICATION, overlapData);
        AppendEvents(deviceId, commNotOverlapCompSections, OverlapAnalysisType::COMM_NOT_OVERLAP_COMP, overlapData);
        if (end_[deviceId] > begin_[deviceId])
        {
            auto unionRecords = UnionTwoSet(computeSections, communicationSections);
            std::vector<TimeDuration> allTimeSection{{begin_[deviceId], end_[deviceId]}};
            AppendEvents(deviceId, GetDifferenceSet(allTimeSection, unionRecords), OverlapAnalysisType::FREE,
                         overlapData);
        }
    }
    return overlapData;
}

void OverlapAnalysisProcessor::AppendEvents(uint16_t deviceId, const std::vector<TimeDuration> &sections,
                                            OverlapAnalysisType type, std::vector<OverlapAnalysisData> &events)
{
    for (auto &section : sections)
    {
        if (section.end <= section.start)
        {
            continue;
        }
        OverlapAnalysisData event;
        event.deviceId = deviceId;
        event.timestamp = section.start;
        event.duration = section.end - section.start;
        event.type = type;
        events.emplace_back(event);
    }
}

std::vector<TimeDuration> OverlapAnalysisProcessor::UnionTwoSet(const std::vector<TimeDuration> &vecA,
                                                                const std::vector<TimeDuration> &vecB)
{
    std::vector<TimeDuration> vec;
    if (!Utils::Reserve(vec, vecA.size() + vecB.size()))
    {
        ERROR("Reserve vec error");
        return {};
    }
    vec.insert(vec.end(), vecA.begin(), vecA.end());
    vec.insert(vec.end(), vecB.begin(), vecB.end());
    std::sort(vec.begin(), vec.end());
    return UnionOneSet(vec);
}

std::vector<TimeDuration> OverlapAnalysisProcessor::GetDifferenceSet(const std::vector<TimeDuration> &vecA,
                                                                     const std::vector<TimeDuration> &vecB)
{
    if (vecA.empty() || vecB.empty())
    {
        return vecA;
    }
    std::vector<TimeDuration> res;
    uint32_t i = 0;
    uint32_t j = 0;
    uint64_t lastRecordEnd = vecA.front().start;
    while (i < vecA.size() && j < vecB.size())
    {
        while (j < vecB.size() && vecB[j].end <= vecA[i].start)
        {
            j++;
        }
        if (j >= vecB.size())
        {
            break;
        }
        if (vecA[i].end <= vecB[j].start)
        {
            ProcessSetAIsOnTheLeftOfSetB(vecA[i], lastRecordEnd, res);
            i++;
            continue;
        }
        else if (vecA[i].start >= vecB[j].start && vecA[i].end <= vecB[j].end)
        {
            i++;
            continue;
        }
        else if ((vecA[i].start <= vecB[j].start && vecA[i].end >= vecB[j].end) ||
                 (vecA[i].end >= vecB[j].start && vecA[i].start <= vecB[j].start))
        {
            ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(vecA[i], vecB[j], lastRecordEnd, res);
            j++;
            continue;
        }
        else if (vecA[i].start >= vecB[j].start && vecA[i].start <= vecB[j].end)
        {
            lastRecordEnd = vecB[j].end;
            j++;
            continue;
        }
        else
        {
            ERROR("Illegal section situation, secA: (%, %), secB: (%, %)", vecA[i].start, vecA[i].end, vecB[j].start,
                  vecB[j].end);
            i++;
        }
    }
    while (i < vecA.size())
    {
        uint64_t startRecord = std::max(vecA[i].start, lastRecordEnd);
        if (startRecord < vecA[i].end)
        {
            res.emplace_back(startRecord, vecA[i].end);
        }
        lastRecordEnd = vecA[i].end;
        i++;
    }
    return res;
}

void OverlapAnalysisProcessor::ProcessSetAIsOnTheLeftOfSetB(const TimeDuration &durationA, uint64_t &lastRecordEnd,
                                                            std::vector<TimeDuration> &res)
{
    if (durationA.end >= lastRecordEnd)
    {
        uint64_t recordStart = std::max(lastRecordEnd, durationA.start);
        if (recordStart < durationA.end)
        {
            res.emplace_back(recordStart, durationA.end);
        }
        lastRecordEnd = durationA.end;
    }
}

void OverlapAnalysisProcessor::ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(const TimeDuration &durationA,
                                                                                       const TimeDuration &durationB,
                                                                                       uint64_t &lastRecordEnd,
                                                                                       std::vector<TimeDuration> &res)
{
    if (durationB.start >= lastRecordEnd)
    {
        uint64_t recordStart = std::max(lastRecordEnd, durationA.start);
        if (recordStart < durationB.start)
        {
            res.emplace_back(recordStart, durationB.start);
        }
        lastRecordEnd = durationB.end;
    }
}

std::vector<TimeDuration> OverlapAnalysisProcessor::UnionOneSet(const std::vector<TimeDuration> &vecA)
{
    if (vecA.empty())
    {
        return {};
    }
    std::vector<TimeDuration> merged;
    for (auto &i : vecA)
    {
        uint64_t left = i.start;
        uint64_t right = i.end;
        if (merged.empty() || merged.back().end < left)
        {
            merged.emplace_back(left, right);
        }
        else
        {
            merged.back().end = std::max(merged.back().end, right);
        }
    }
    return merged;
}
}  // namespace Domain
}  // namespace Analysis
