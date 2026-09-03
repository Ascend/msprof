/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/data_process/ai_task/aicpu_processor.h"

#include <algorithm>
#include <map>
#include <tuple>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Common;

namespace
{
using StreamTaskKey = std::tuple<uint16_t, uint32_t, uint32_t>;
using GeKey = std::tuple<uint16_t, uint32_t, uint32_t, uint32_t>;

// AiCpuData / AiCpuDP 落盘为 ns，对齐 Python aicpuViewer：CSV 为 us
double NsToUs(double timeNs) { return timeNs / static_cast<double>(NS_TO_US); }
}  // namespace

AicpuProcessor::AicpuProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool AicpuProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<AicpuSummaryData> summaryData;
    std::vector<AicpuDpData> dpData;
    std::vector<AicpuMiData> miData;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, summaryData, dpData, miData) && flag;
    }

    auto ascendTasks = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    auto taskInfos = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto version = Context::GetInstance().GetPlatformVersion();
    const bool isChipV6 = Context::IsChipV6(version);
    if (!isChipV6 && ascendTasks != nullptr)
    {
        MatchBatchId(summaryData, *ascendTasks);
    }
    if (taskInfos != nullptr)
    {
        MatchNodeName(summaryData, *taskInfos, isChipV6);
    }
    std::stable_sort(summaryData.begin(), summaryData.end(),
                     [](const AicpuSummaryData &lhs, const AicpuSummaryData &rhs)
                     { return lhs.timestampNs < rhs.timestampNs; });

    flag = SaveToDataInventory<AicpuSummaryData>(std::move(summaryData), dataInventory, PROCESSOR_NAME_AICPU) && flag;
    flag = SaveToDataInventory<AicpuDpData>(std::move(dpData), dataInventory, PROCESSOR_NAME_AICPU) && flag;
    flag = SaveToDataInventory<AicpuMiData>(std::move(miData), dataInventory, PROCESSOR_NAME_AICPU) && flag;
    return flag;
}

bool AicpuProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<AicpuSummaryData> &summaryData,
                                         std::vector<AicpuDpData> &dpData, std::vector<AicpuMiData> &miData)
{
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    ProfTimeRecord timeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_, deviceId))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    bool flag = LoadAiCpuData(devicePath, deviceId, timeRecord, summaryData);
    flag = LoadDpData(devicePath, timeRecord, dpData) && flag;
    flag = LoadMiData(devicePath, miData) && flag;
    return flag;
}

bool AicpuProcessor::LoadAiCpuData(const std::string &devicePath, uint16_t deviceId, const ProfTimeRecord &timeRecord,
                                   std::vector<AicpuSummaryData> &summaryData)
{
    DBInfo aicpuDB(DB_NAME_AI_CPU, TABLE_NAME_AI_CPU);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, aicpuDB.dbName});
    if (!aicpuDB.ConstructDBRunner(dbPath) || aicpuDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, aicpuDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    OriAiCpuData oriData;
    std::string sql{
        "SELECT stream_id, task_id, sys_start, sys_end, node_name, compute_time, memcpy_time, task_time, "
        "dispatch_time, total_time FROM " +
        aicpuDB.tableName + " ORDER BY sys_start"};
    if (!aicpuDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", aicpuDB.tableName);
        return false;
    }
    for (const auto &row : oriData)
    {
        AicpuSummaryData data;
        double sysStart = 0;
        double sysEnd = 0;
        double computeTime = 0;
        double memcpyTime = 0;
        double taskTime = 0;
        double dispatchTime = 0;
        double totalTime = 0;
        std::tie(data.streamId, data.taskId, sysStart, sysEnd, data.nodeName, computeTime, memcpyTime, taskTime,
                 dispatchTime, totalTime) = row;
        data.deviceId = deviceId;
        HPFloat start{sysStart};
        HPFloat end{sysEnd};
        data.timestampNs = GetLocalTime(start, timeRecord).Uint64();
        data.endNs = GetLocalTime(end, timeRecord).Uint64();
        if (data.timestampNs < timeRecord.startTimeNs)
        {
            continue;
        }
        data.nodeName = data.nodeName.empty() ? NA : data.nodeName;
        data.computeTimeUs = NsToUs(computeTime);
        data.memcpyTimeUs = NsToUs(memcpyTime);
        data.taskTimeUs = NsToUs(taskTime);
        data.dispatchTimeUs = NsToUs(dispatchTime);
        data.totalTimeUs = NsToUs(totalTime);
        summaryData.push_back(data);
    }
    return true;
}

bool AicpuProcessor::LoadDpData(const std::string &devicePath, const ProfTimeRecord &timeRecord,
                                std::vector<AicpuDpData> &dpData)
{
    DBInfo dpDB(DB_NAME_AI_CPU, TABLE_NAME_AI_CPU_DP);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, dpDB.dbName});
    if (!dpDB.ConstructDBRunner(dbPath) || dpDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, dpDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    OriAiCpuDpData oriData;
    std::string sql{"SELECT timestamp, action, source, buffer_size FROM " + dpDB.tableName + " ORDER BY timestamp"};
    if (!dpDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dpDB.tableName);
        return false;
    }
    for (const auto &row : oriData)
    {
        AicpuDpData data;
        double rawTimestamp = 0;
        std::tie(rawTimestamp, data.action, data.source, data.bufferSize) = row;
        HPFloat timestamp{rawTimestamp};
        data.timestamp = GetLocalTime(timestamp, timeRecord).Uint64();
        dpData.push_back(data);
    }
    return true;
}

bool AicpuProcessor::LoadMiData(const std::string &devicePath, std::vector<AicpuMiData> &miData)
{
    DBInfo miDB(DB_NAME_DATA_PREPROCESS, TABLE_NAME_DATA_QUEUE);
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, miDB.dbName});
    if (!miDB.ConstructDBRunner(dbPath) || miDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, miDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    OriAiCpuMiData oriData;
    std::string sql{"SELECT node_name, start_time, end_time, queue_size FROM " + miDB.tableName +
                    " ORDER BY start_time"};
    if (!miDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", miDB.tableName);
        return false;
    }
    for (const auto &row : oriData)
    {
        AicpuMiData data;
        double startTime = 0;
        double endTime = 0;
        std::tie(data.nodeName, startTime, endTime, data.queueSize) = row;
        data.startTime = static_cast<uint64_t>(startTime);
        data.endTime = static_cast<uint64_t>(endTime);
        miData.push_back(data);
    }
    return true;
}

void AicpuProcessor::MatchBatchId(std::vector<AicpuSummaryData> &summaryData,
                                  const std::vector<AscendTaskData> &ascendTasks)
{
    std::map<StreamTaskKey, std::vector<const AscendTaskData *>> taskMap;
    for (const auto &task : ascendTasks)
    {
        if (task.hostType != KERNEL_AICPU_TASK_TYPE)
        {
            continue;
        }
        taskMap[{task.deviceId, task.streamId, task.taskId}].push_back(&task);
    }
    for (auto &entry : taskMap)
    {
        std::sort(entry.second.begin(), entry.second.end(),
                  [](const AscendTaskData *lhs, const AscendTaskData *rhs) { return lhs->timestamp < rhs->timestamp; });
    }

    std::map<StreamTaskKey, std::vector<AicpuSummaryData *>> aicpuMap;
    for (auto &item : summaryData)
    {
        aicpuMap[{item.deviceId, item.streamId, item.taskId}].push_back(&item);
    }
    for (auto &entry : aicpuMap)
    {
        std::sort(entry.second.begin(), entry.second.end(),
                  [](const AicpuSummaryData *lhs, const AicpuSummaryData *rhs) { return lhs->endNs < rhs->endNs; });
    }

    for (auto &entry : aicpuMap)
    {
        auto taskIt = taskMap.find(entry.first);
        if (taskIt == taskMap.end())
        {
            continue;
        }
        auto &aicpuList = entry.second;
        auto &taskList = taskIt->second;
        size_t aicpuIndex = 0;
        size_t taskIndex = 0;
        while (aicpuIndex < aicpuList.size() && taskIndex < taskList.size())
        {
            const uint64_t sysEndNs = aicpuList[aicpuIndex]->endNs;
            const auto *task = taskList[taskIndex];
            if (task->timestamp <= sysEndNs && sysEndNs <= task->end)
            {
                aicpuList[aicpuIndex]->batchId = task->batchId;
                ++aicpuIndex;
            }
            else if (sysEndNs < task->timestamp)
            {
                ++aicpuIndex;
            }
            else
            {
                ++taskIndex;
            }
        }
    }
}

void AicpuProcessor::MatchNodeName(std::vector<AicpuSummaryData> &summaryData,
                                   const std::vector<TaskInfoData> &taskInfos, bool isChipV6)
{
    std::map<GeKey, std::string> geMap;
    std::map<StreamTaskKey, std::string> geMapV6;
    for (const auto &info : taskInfos)
    {
        if (!isChipV6 && info.taskType != AI_CPU)
        {
            continue;
        }
        if (isChipV6)
        {
            geMapV6.emplace(StreamTaskKey{info.deviceId, info.streamId, info.taskId}, info.opName);
        }
        else
        {
            geMap.emplace(GeKey{info.deviceId, info.streamId, info.taskId, info.batchId}, info.opName);
        }
    }
    for (auto &item : summaryData)
    {
        if (isChipV6)
        {
            auto it = geMapV6.find({item.deviceId, item.streamId, item.taskId});
            if (it != geMapV6.end())
            {
                item.nodeName = it->second;
            }
            continue;
        }
        auto it = geMap.find({item.deviceId, item.streamId, item.taskId, item.batchId});
        if (it != geMap.end())
        {
            item.nodeName = it->second;
        }
    }
}
}  // namespace Domain
}  // namespace Analysis
