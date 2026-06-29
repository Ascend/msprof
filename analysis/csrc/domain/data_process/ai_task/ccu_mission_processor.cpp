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

#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"
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
struct CCUMissionInfo
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t taskId = UINT32_MAX;
    uint32_t relId = UINT32_MAX;
    uint16_t streamId = UINT16_MAX;
    uint16_t lpInstrId = UINT16_MAX;
    uint16_t setCkeBitInstrId = UINT16_MAX;
    std::string timeType;
};

struct CCUWaitSignalInfo
{
    uint32_t taskId = UINT32_MAX;
    uint32_t mask = UINT32_MAX;
    uint16_t streamId = UINT16_MAX;
    uint16_t instrId = UINT16_MAX;
    uint16_t channelId = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;
};

struct CCUGroupInfo
{
    uint64_t dataSize = UINT64_MAX;
    uint32_t taskId = UINT32_MAX;
    uint16_t streamId = UINT16_MAX;
    uint16_t instrId = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;
    std::string reduceOpType;
    std::string inputDataType;
    std::string outputDataType;
};

struct CCUChannelInfo
{
    uint64_t timestamp = UINT64_MAX;
    uint64_t avgDelay = 0;
    uint16_t channelId = UINT16_MAX;
};

struct CCUDelayChannel
{
    uint64_t channelDelay = 0;
    uint16_t channelId = UINT16_MAX;
};

std::string MakeCCUKey(uint64_t streamId, uint64_t taskId, uint64_t instrId)
{
    return std::to_string(streamId) + "_" + std::to_string(taskId) + "_" + std::to_string(instrId);
}

CCUDelayChannel GetMaxDelayChannel(const std::vector<CCUWaitSignalInfo> &hostData, const CCUMissionInfo &missionData,
                                   const std::vector<CCUChannelInfo> &channelData)
{
    if (channelData.empty())
    {
        return {};
    }
    std::unordered_set<uint16_t> hostChannelIds;
    for (const auto &item : hostData)
    {
        hostChannelIds.insert(item.channelId);
    }
    CCUDelayChannel maxDelay;
    bool found = false;
    for (const auto &channel : channelData)
    {
        if (hostChannelIds.find(channel.channelId) == hostChannelIds.end())
        {
            continue;
        }
        if (channel.timestamp >= missionData.endTime)
        {
            continue;
        }
        if (!found || channel.avgDelay > maxDelay.channelDelay)
        {
            maxDelay.channelId = channel.channelId;
            maxDelay.channelDelay = channel.avgDelay;
            found = true;
        }
    }
    return maxDelay;
}

void ConvertMissionData(const OriCCUMissionData &oriData, std::vector<CCUMissionInfo> &missionData)
{
    if (!Reserve(missionData, oriData.size()))
    {
        ERROR("Reserve ccu mission data failed.");
        return;
    }
    CCUMissionInfo oneData;
    for (const auto &row : oriData)
    {
        std::tie(oneData.streamId, oneData.taskId, oneData.lpInstrId, oneData.setCkeBitInstrId, oneData.relId,
                 oneData.startTime, oneData.endTime, oneData.timeType) = row;
        missionData.push_back(oneData);
    }
}

void ConvertWaitSignalData(const OriCCUWaitSignalData &oriData, std::vector<CCUWaitSignalInfo> &waitSignalData)
{
    if (!Reserve(waitSignalData, oriData.size()))
    {
        ERROR("Reserve ccu wait signal data failed.");
        return;
    }
    CCUWaitSignalInfo oneData;
    for (const auto &row : oriData)
    {
        std::tie(oneData.streamId, oneData.taskId, oneData.instrId, oneData.dieId, oneData.mask, oneData.channelId) =
            row;
        waitSignalData.push_back(oneData);
    }
}

void ConvertGroupData(const OriCCUGroupData &oriData, std::vector<CCUGroupInfo> &groupData)
{
    if (!Reserve(groupData, oriData.size()))
    {
        ERROR("Reserve ccu group data failed.");
        return;
    }
    CCUGroupInfo oneData;
    for (const auto &row : oriData)
    {
        std::tie(oneData.streamId, oneData.taskId, oneData.instrId, oneData.dieId, oneData.reduceOpType,
                 oneData.inputDataType, oneData.outputDataType, oneData.dataSize) = row;
        groupData.push_back(oneData);
    }
}

void ConvertChannelData(const OriCCUChannelData &oriData, std::vector<CCUChannelInfo> &channelData)
{
    if (!Reserve(channelData, oriData.size()))
    {
        ERROR("Reserve ccu channel data failed.");
        return;
    }
    CCUChannelInfo oneData;
    uint64_t maxBw = 0;
    uint64_t minBw = 0;
    for (const auto &row : oriData)
    {
        std::tie(oneData.channelId, oneData.timestamp, maxBw, minBw, oneData.avgDelay) = row;
        channelData.push_back(oneData);
    }
}

void FormatLoopTimelineData(const std::vector<CCUMissionInfo> &loopData, const std::vector<CCUGroupInfo> &groupData,
                            std::vector<CCUMissionTimelineData> &result, uint16_t deviceId,
                            const Utils::SyscntConversionParams &params, const Utils::ProfTimeRecord &record)
{
    if (loopData.empty())
    {
        return;
    }
    std::unordered_map<std::string, CCUMissionInfo> groupedLoopData;
    std::unordered_map<std::string, std::vector<CCUGroupInfo>> groupedGroupData;
    for (const auto &item : loopData)
    {
        auto key = MakeCCUKey(item.streamId, item.taskId, item.lpInstrId);
        groupedLoopData[key] = item;
    }
    for (const auto &item : groupData)
    {
        auto key = MakeCCUKey(item.streamId, item.taskId, item.instrId);
        groupedGroupData[key].push_back(item);
    }
    for (const auto &item : groupedLoopData)
    {
        const auto &data = item.second;
        CCUMissionTimelineData traceData;
        traceData.deviceId = deviceId;
        traceData.timeType = data.timeType;
        traceData.streamId = data.streamId;
        traceData.taskId = data.taskId;
        traceData.instructionId = data.lpInstrId;
        HPFloat start = GetTimeFromSyscnt(data.startTime, params);
        traceData.timestamp = GetLocalTime(start, record).Uint64();
        traceData.duration =
            data.endTime > data.startTime ? GetDurTimeFromSyscnt(data.endTime - data.startTime, params).Double() : 0.0;
        auto hostDataIter = groupedGroupData.find(item.first);
        if (hostDataIter != groupedGroupData.end() && !hostDataIter->second.empty())
        {
            const auto &hostData = hostDataIter->second.front();
            traceData.hasDieId = true;
            traceData.dieId = hostData.dieId;
            traceData.hasDataSize = true;
            traceData.dataSize = hostData.dataSize;
            if (traceData.duration > 0)
            {
                traceData.hasBandwidth = true;
                double durationUs = static_cast<double>(traceData.duration) / NS_TO_US;
                traceData.bandwidth = durationUs > 0 ? static_cast<double>(hostData.dataSize) / durationUs *
                                                           (static_cast<double>(MICRO_SECOND) / BYTE_SIZE / BYTE_SIZE)
                                                     : 0.0;
            }
            if (hostData.reduceOpType != "RESERVED")
            {
                traceData.hasReduceInfo = true;
                traceData.reduceOpType = hostData.reduceOpType;
                traceData.inputDataType = hostData.inputDataType;
                traceData.outputDataType = hostData.outputDataType;
            }
        }
        result.push_back(traceData);
    }
}

void FormatWaitTimelineData(const std::vector<CCUMissionInfo> &waitData,
                            const std::vector<CCUWaitSignalInfo> &waitSignalData,
                            const std::vector<CCUChannelInfo> &channelInfo, std::vector<CCUMissionTimelineData> &result,
                            uint16_t deviceId, const Utils::SyscntConversionParams &params,
                            const Utils::ProfTimeRecord &record)
{
    if (waitData.empty())
    {
        return;
    }
    std::unordered_map<std::string, std::vector<CCUMissionInfo>> groupedWaitData;
    std::unordered_map<std::string, std::vector<CCUWaitSignalInfo>> groupedWaitSignalData;
    for (const auto &item : waitData)
    {
        auto key = MakeCCUKey(item.streamId, item.taskId, item.setCkeBitInstrId);
        groupedWaitData[key].push_back(item);
    }
    for (const auto &item : waitSignalData)
    {
        auto key = MakeCCUKey(item.streamId, item.taskId, item.instrId);
        groupedWaitSignalData[key].push_back(item);
    }
    for (const auto &item : groupedWaitData)
    {
        const auto &dataList = item.second;
        auto latestDataIter = std::max_element(dataList.begin(), dataList.end(),
                                               [](const CCUMissionInfo &left, const CCUMissionInfo &right)
                                               { return left.endTime < right.endTime; });
        if (latestDataIter == dataList.end())
        {
            continue;
        }
        const auto &data = *latestDataIter;
        CCUMissionTimelineData traceData;
        traceData.deviceId = deviceId;
        traceData.timeType = data.timeType;
        traceData.streamId = data.streamId;
        traceData.taskId = data.taskId;
        traceData.instructionId = data.setCkeBitInstrId;
        traceData.notifyRankId = data.relId;
        HPFloat start = GetTimeFromSyscnt(data.startTime, params);
        traceData.timestamp = GetLocalTime(start, record).Uint64();
        traceData.duration =
            data.endTime > data.startTime ? GetDurTimeFromSyscnt(data.endTime - data.startTime, params).Double() : 0.0;
        auto hostDataIter = groupedWaitSignalData.find(item.first);
        if (hostDataIter != groupedWaitSignalData.end() && !hostDataIter->second.empty())
        {
            const auto &hostData = hostDataIter->second.front();
            traceData.hasDieId = true;
            traceData.dieId = hostData.dieId;
            traceData.hasMask = true;
            traceData.mask = hostData.mask;
            auto maxDelay = GetMaxDelayChannel(hostDataIter->second, data, channelInfo);
            if (maxDelay.channelId != UINT16_MAX && maxDelay.channelDelay != 0)
            {
                traceData.hasDelayChannel = true;
                traceData.maxDelayChannel = maxDelay.channelId;
                traceData.maxChannelDelay = maxDelay.channelDelay;
            }
        }
        result.push_back(traceData);
    }
}
}  // namespace

CCUMissionProcessor::CCUMissionProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool CCUMissionProcessor::Process(DataInventory &dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    OriCCUWaitSignalData waitSignalData;
    OriCCUGroupData groupData;
    bool flag = LoadHostAddInfo(waitSignalData, groupData);
    std::vector<CCUMissionTimelineData> result;
    for (const auto &devicePath : deviceList)
    {
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %, device id is %.", profPath_, deviceId);
            flag = false;
            continue;
        }
        ProfTimeRecord record;
        if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId))
        {
            ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
            flag = false;
            continue;
        }
        OriCCUMissionData missionData;
        OriCCUChannelData channelData;
        if (!LoadMissionAndChannel(devicePath, missionData, channelData))
        {
            ERROR("Load ccu data failed, device path is %.", devicePath);
            flag = false;
            continue;
        }
        auto oneDeviceData =
            FormatTimelineData(missionData, waitSignalData, groupData, channelData, deviceId, params, record);
        FilterDataByStartTime(oneDeviceData, record.startTimeNs, PROCESSOR_NAME_CCU_MISSION);
        result.insert(result.end(), oneDeviceData.begin(), oneDeviceData.end());
    }
    if (!SaveToDataInventory<CCUMissionTimelineData>(std::move(result), dataInventory, PROCESSOR_NAME_CCU_MISSION))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_CCU_MISSION);
        return false;
    }
    return flag;
}

bool CCUMissionProcessor::LoadMissionAndChannel(const std::string &devicePath, OriCCUMissionData &missionData,
                                                OriCCUChannelData &channelData)
{
    DBInfo missionDB("ccu.db", "OriginMission");
    DBInfo channelDB("ccu.db", "OriginChannel");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, missionDB.dbName});
    if (!missionDB.ConstructDBRunner(dbPath) || !channelDB.ConstructDBRunner(dbPath))
    {
        ERROR("Construct ccu mission db runner failed.");
        return false;
    }
    auto status = CheckPathAndTable(dbPath, missionDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    std::string missionSql =
        "SELECT stream_id, task_id, lp_instr_id, setckebit_instr_id, rel_id, "
        "CASE WHEN setckebit_start_time <> 0 THEN setckebit_start_time ELSE lp_start_time END "
        "AS start_time, "
        "CASE WHEN setckebit_start_time <> 0 THEN rel_end_time ELSE lp_end_time END AS end_time, "
        "CASE WHEN setckebit_start_time <> 0 THEN 'Wait' ELSE 'LoopGroup' END AS time_type "
        "FROM " +
        missionDB.tableName + " WHERE setckebit_start_time <> 0 OR lp_start_time <> 0;";
    if (!missionDB.dbRunner->QueryData(missionSql, missionData))
    {
        ERROR("Query ccu mission data failed, db path is %.", dbPath);
        return false;
    }
    status = CheckPathAndTable(dbPath, channelDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    std::string channelSql =
        "SELECT channel_id, timestamp, max_bw, min_bw, avg_bw FROM " + channelDB.tableName + " WHERE timestamp <> 0;";
    if (!channelDB.dbRunner->QueryData(channelSql, channelData))
    {
        ERROR("Query ccu channel data failed, db path is %.", dbPath);
        return false;
    }
    return true;
}

bool CCUMissionProcessor::LoadHostAddInfo(OriCCUWaitSignalData &waitSignalData, OriCCUGroupData &groupData)
{
    DBInfo waitSignalDB("ccu_add_info.db", "CCUWaitSignalInfo");
    DBInfo groupDB("ccu_add_info.db", "CCUGroupInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, "ccu_add_info.db"});
    if (!waitSignalDB.ConstructDBRunner(dbPath) || !groupDB.ConstructDBRunner(dbPath))
    {
        ERROR("Construct ccu add info db runner failed.");
        return false;
    }
    auto status = CheckPathAndTable(dbPath, waitSignalDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == CHECK_SUCCESS)
    {
        std::string waitSql =
            "SELECT stream_id, task_id, instr_id, die_id, mask, channel_id "
            "FROM " +
            waitSignalDB.tableName + ";";
        if (!waitSignalDB.dbRunner->QueryData(waitSql, waitSignalData))
        {
            ERROR("Query ccu wait signal data failed, db path is %.", dbPath);
            return false;
        }
    }
    status = CheckPathAndTable(dbPath, groupDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    if (status == CHECK_SUCCESS)
    {
        std::string groupSql =
            "SELECT stream_id, task_id, instr_id, die_id, CAST(reduce_op_type AS TEXT), "
            "CAST(input_data_type AS TEXT), CAST(output_data_type AS TEXT), data_size "
            "FROM " +
            groupDB.tableName + ";";
        if (!groupDB.dbRunner->QueryData(groupSql, groupData))
        {
            ERROR("Query ccu group data failed, db path is %.", dbPath);
            return false;
        }
    }
    return true;
}

std::vector<CCUMissionTimelineData> CCUMissionProcessor::FormatTimelineData(
    const OriCCUMissionData &missionData, const OriCCUWaitSignalData &waitSignalData, const OriCCUGroupData &groupData,
    const OriCCUChannelData &channelData, uint16_t deviceId, const Utils::SyscntConversionParams &params,
    const Utils::ProfTimeRecord &record) const
{
    std::vector<CCUMissionTimelineData> result;
    if (missionData.empty())
    {
        return result;
    }
    std::vector<CCUMissionInfo> missionInfo;
    std::vector<CCUWaitSignalInfo> waitSignalInfo;
    std::vector<CCUGroupInfo> groupInfo;
    std::vector<CCUChannelInfo> channelInfo;
    ConvertMissionData(missionData, missionInfo);
    ConvertWaitSignalData(waitSignalData, waitSignalInfo);
    ConvertGroupData(groupData, groupInfo);
    ConvertChannelData(channelData, channelInfo);
    if (!Reserve(result, missionInfo.size()))
    {
        ERROR("Reserve ccu formatted timeline data failed.");
        return result;
    }
    std::vector<CCUMissionInfo> loopData;
    std::vector<CCUMissionInfo> waitData;
    if (!Reserve(loopData, missionInfo.size()) || !Reserve(waitData, missionInfo.size()))
    {
        ERROR("Reserve ccu loop/wait timeline data failed.");
        return result;
    }
    for (const auto &item : missionInfo)
    {
        if (item.timeType == CCU_TIME_TYPE_LOOP_GROUP)
        {
            loopData.push_back(item);
        }
        else if (item.timeType == CCU_TIME_TYPE_WAIT)
        {
            waitData.push_back(item);
        }
    }
    FormatLoopTimelineData(loopData, groupInfo, result, deviceId, params, record);
    FormatWaitTimelineData(waitData, waitSignalInfo, channelInfo, result, deviceId, params, record);
    return result;
}
}  // namespace Domain
}  // namespace Analysis
