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
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"

#include <algorithm>

#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"

namespace Analysis
{

namespace Domain
{
using namespace Infra;
namespace
{
/*
 * 当开始的任务和结束的任务数量不一致时，按照相同streamId-taskId-contextId的数据，先执行的任务结束时间比后续执行任务的开始
 * 时间小的规则进行匹配。例如；
 * start:  [1, 5, 9, 13, 25]                   [4, 6, 9, 13]
 * end:    [3, 7, 10, 15]                      [3, 5, 8, 10, 15]
 * res:    [1-3, 5-7, 9-10, 13-15]             [4-5, 6-8, 9-10, 13-15]
 */
void MergeStartAndEndByQueue(std::vector<HalLogData*>& start, std::vector<HalLogData*>& end,
                             std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
                             std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc)
{
    size_t sIndex = 0;
    size_t eIndex = 0;
    while (sIndex < start.size() && eIndex < end.size())
    {
        if (end[eIndex]->hd.timestamp >= start[sIndex]->hd.timestamp &&
            (sIndex + 1 == start.size() || end[eIndex]->hd.timestamp < start[sIndex + 1]->hd.timestamp))
        {
            auto& vec = deviceTaskMap[start[sIndex]->hd.taskId];
            vec.emplace_back();
            mergeFunc(vec.back(), *start[sIndex], *end[eIndex]);
            sIndex++;
            eIndex++;
        }
        else
        {  // end task对应的不是当前索引位置的start task,继续找下一个
            WARN("Start task in % doesn't match end task in %, streamId is %, taskId is %", sIndex, eIndex,
                 start[sIndex]->hd.taskId.streamId, start[sIndex]->hd.taskId.taskId);
            if (end[eIndex]->hd.timestamp >= start[sIndex]->hd.timestamp)
            {
                sIndex++;
            }
            else
            {
                eIndex++;
            }
        }
    }
}

void MergeStartAndEnd(std::map<MergeTaskKeyData, std::vector<HalLogData*>>& logStart,
                      std::map<MergeTaskKeyData, std::vector<HalLogData*>>& logEnd,
                      std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
                      std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc)
{
    for (auto& startPair : logStart)
    {
        const auto& key = startPair.first;
        auto it = logEnd.find(key);
        if (it == logEnd.end())
        {
            WARN("Start log size(%) not same as End log size(%); context:%, task:%, stream:%", startPair.second.size(),
                 0, key.contextId, key.taskId, key.streamId);
            continue;
        }
        else if (startPair.second.size() != it->second.size())
        {
            MergeStartAndEndByQueue(startPair.second, it->second, deviceTaskMap, mergeFunc);
            WARN("Start log size(%) not same as End log size(%); context:%, task:%, stream:%", startPair.second.size(),
                 it->second.size(), key.contextId, key.taskId, key.streamId);
            continue;
        }
        for (size_t i = 0; i < startPair.second.size(); ++i)
        {
            auto& vec = deviceTaskMap[startPair.second[i]->hd.taskId];
            vec.emplace_back();
            mergeFunc(vec.back(), *startPair.second[i], *it->second[i]);
        }
    }
}
}  // namespace

void LogModeling::SplitLogGroups(std::vector<HalLogData>& logData,
                                 std::shared_ptr<std::vector<HalTrackData>>& flipTrack)
{
    Utils::TimeLogger t{"LogModeling::SplitLogGroups "};
    std::sort(logData.begin(), logData.end(),
              [](HalLogData& ld, HalLogData& rd) { return ld.hd.timestamp < rd.hd.timestamp; });
    for (auto& halLog : logData)
    {
        if (halLog.type == ACSQ_LOG)
        {
            if (halLog.acsq.isEndTimestamp)
            {
                acsqEnd_[GenGroupKey(halLog)].push_back(&halLog);
            }
            else
            {
                acsqStart_[GenGroupKey(halLog)].push_back(&halLog);
            }
        }
        else if (halLog.type == FFTS_LOG)
        {
            if (halLog.ffts.isEndTimestamp)
            {
                fftsEnd_[GenGroupKey(halLog)].push_back(&halLog);
            }
            else
            {
                fftsStart_[GenGroupKey(halLog)].push_back(&halLog);
            }
        }
    }
    if (flipTrack)
    {
        auto flipGroup = GetFlipData(*flipTrack);
        flipData_.swap(flipGroup);
    }
}

size_t GetDeviceTaskNodeSize(const std::map<TaskId, std::vector<DeviceTask>>& tasks)
{
    size_t total{};
    for (const auto& node : tasks)
    {
        total += node.second.size();
    }
    return total;
}

size_t GetTaskNodeSize(const std::unordered_map<uint32_t, std::vector<HalLogData*>>& tasks)
{
    size_t total{};
    for (const auto& node : tasks)
    {
        total += node.second.size();
    }
    return total;
}

void LogModeling::OutputLogCounts(const std::vector<HalLogData>& logData) const
{
    INFO("HalLogData size:%", logData.size());
    INFO("acsqS stream:%, total:%", acsqStart_.size(), GetTaskNodeSize(acsqStart_));
    INFO("acsqE stream:%, total:%", acsqEnd_.size(), GetTaskNodeSize(acsqEnd_));
    INFO("fftsS stream:%, total:%", fftsStart_.size(), GetTaskNodeSize(fftsStart_));
    INFO("fftsE stream:%, total:%", fftsEnd_.size(), GetTaskNodeSize(fftsEnd_));
}

void LogModeling::AddToDeviceTask(
    std::unordered_map<uint32_t, std::vector<HalLogData*>>& startTask,
    std::unordered_map<uint32_t, std::vector<HalLogData*>>& endTask,
    std::unordered_map<uint32_t, std::vector<HalTrackData*>>& flipGroups,
    std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
    std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc)
{
    // 下面for循环中会:1. 给Start补Batch id;  2. 给Start和End按三元组分组;  3. 将分组后的Log写入DeviceTask
    for (auto& streamNode : startTask)
    {
        std::map<MergeTaskKeyData, std::vector<HalLogData*>> acsqTaskS;
        std::map<MergeTaskKeyData, std::vector<HalLogData*>> acsqTaskE;
        auto it = flipGroups.find(streamNode.first);
        if (it != flipGroups.end())
        {
            std::sort(streamNode.second.begin(), streamNode.second.end(),
                      [](HalLogData* lhs, HalLogData* rhs) { return lhs->hd.timestamp < rhs->hd.timestamp; });
            std::sort(it->second.begin(), it->second.end(),
                      [](HalTrackData* lhs, HalTrackData* rhs) { return lhs->hd.timestamp < rhs->hd.timestamp; });
            ModelingComputeBatchIdBinary(Utils::ReinterpretConvert<HalUniData**>(streamNode.second.data()),
                                         streamNode.second.size(),
                                         Utils::ReinterpretConvert<HalUniData**>(it->second.data()), it->second.size());
        }

        uint32_t streamId = streamNode.first;
        for (auto& node : streamNode.second)
        {
            acsqTaskS[GenMergeTaskKey(*node)].push_back(node);
        }

        auto itE = endTask.find(streamId);
        if (itE == endTask.end())
        {
            ERROR("start exist but end not exist, stream:%", streamId);
            continue;
        }
        if (itE->second.size() > 1)
        {
            std::sort(itE->second.begin(), itE->second.end(),
                      [](HalLogData* lhs, HalLogData* rhs) { return lhs->hd.timestamp < rhs->hd.timestamp; });
        }
        for (auto& node : itE->second)
        {
            acsqTaskE[GenMergeTaskKey(*node)].push_back(node);
        }

        MergeStartAndEnd(acsqTaskS, acsqTaskE, deviceTaskMap, mergeFunc);
    }
}

uint32_t LogModeling::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context&)
{
    auto logData = dataInventory.GetPtr<std::vector<HalLogData>>();
    auto flipTrack = dataInventory.GetPtr<std::vector<HalTrackData>>();
    auto deviceTaskMap = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!logData || !deviceTaskMap)
    {
        ERROR("data null:logData:%, deviceTaskMap:%", logData != nullptr, deviceTaskMap != nullptr);
        return Analysis::ANALYSIS_ERROR;
    }

    // 为了后面补BatchId，按stream和开始结束的维度给log分组
    SplitLogGroups(*logData, flipTrack);
    OutputLogCounts(*logData);

    size_t deviceTaskNum = GetDeviceTaskNodeSize(*deviceTaskMap);
    Utils::TimeLogger t{"LogModeling::AddToDeviceTask "};
    AddToDeviceTask(acsqStart_, acsqEnd_, flipData_, *deviceTaskMap,
                    [](Domain::DeviceTask& oneTask, const HalLogData& startLog, const HalLogData& endLog)
                    {
                        oneTask.taskType = startLog.acsq.taskType;
                        oneTask.taskStart = startLog.acsq.timestamp;
                        oneTask.taskEnd = endLog.acsq.timestamp;
                        oneTask.logType = HalLogType::ACSQ_LOG;
                    });
    size_t acsqMatchedCount = GetDeviceTaskNodeSize(*deviceTaskMap) - deviceTaskNum;
    INFO("ACSQ matched count:%", acsqMatchedCount);

    AddToDeviceTask(fftsStart_, fftsEnd_, flipData_, *deviceTaskMap,
                    [](Domain::DeviceTask& oneTask, const HalLogData& startLog, const HalLogData& endLog)
                    {
                        oneTask.taskType = startLog.ffts.subTaskType;
                        oneTask.taskStart = startLog.ffts.timestamp;
                        oneTask.taskEnd = endLog.ffts.timestamp;
                        oneTask.logType = HalLogType::FFTS_LOG;
                    });
    size_t fftsMatchedCount = GetDeviceTaskNodeSize(*deviceTaskMap) - acsqMatchedCount - deviceTaskNum;
    INFO("FFTS matched count:%", fftsMatchedCount);
    return Analysis::ANALYSIS_OK;
}

uint32_t LogModeling::GenGroupKey(const HalLogData& logData) { return logData.hd.taskId.streamId; }
MergeTaskKeyData LogModeling::GenMergeTaskKey(const HalLogData& logData)
{
    return {logData.hd.taskId.contextId, logData.hd.taskId.taskId, logData.hd.taskId.streamId};
}

REGISTER_PROCESS_SEQUENCE(Domain::LogModeling, true, Domain::StarsSocParser, Domain::TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(Domain::LogModeling, std::vector<Domain::HalLogData>, std::vector<Domain::HalTrackData>,
                                std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(Domain::LogModeling, CHIP_V4_1_0);

namespace CHIP_V6
{
REGISTER_PROCESS_SEQUENCE(Domain::LogModelingV6, true, Domain::StarsSocParser, Domain::TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(Domain::LogModelingV6, std::vector<Domain::HalLogData>,
                                std::vector<Domain::HalTrackData>,
                                std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(Domain::LogModelingV6, CHIP_V6_1_0);
}  // namespace CHIP_V6

}  // namespace Domain
}  // namespace Analysis
