/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/data_process/ai_task/ccu_mission_processor.h"

#include <algorithm>
#include <map>
#include <tuple>
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
    uint32_t streamId = UINT32_MAX;
    uint16_t lpInstrId = UINT16_MAX;
    uint16_t setCkeBitInstrId = UINT16_MAX;
    std::string timeType;
};

struct CCUWaitSignalInfo
{
    uint32_t taskId = UINT32_MAX;
    uint32_t mask = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint16_t instrId = UINT16_MAX;
    uint16_t channelId = UINT16_MAX;
    uint16_t dieId = UINT16_MAX;
};

struct CCUGroupInfo
{
    uint64_t dataSize = UINT64_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
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

struct CCUChannelPrefixInfo
{
    uint64_t timestamp = UINT64_MAX;
    uint64_t maxDelay = 0;
    uint16_t channelId = UINT16_MAX;
};

using CCUChannelIndex = std::unordered_map<uint16_t, std::vector<CCUChannelPrefixInfo>>;

std::string MakeCCUKey(uint64_t streamId, uint64_t taskId, uint64_t instrId)
{
    return std::to_string(streamId) + "_" + std::to_string(taskId) + "_" + std::to_string(instrId);
}

CCUDelayChannel GetMaxDelayChannel(const std::vector<CCUWaitSignalInfo> &hostData, uint64_t endSyscnt,
                                   const CCUChannelIndex &channelIndex)
{
    if (channelIndex.empty())
    {
        return {};
    }
    std::unordered_set<uint16_t> hostChannelIds;
    hostChannelIds.reserve(hostData.size());
    CCUDelayChannel maxDelay;
    bool found = false;
    for (const auto &item : hostData)
    {
        if (!hostChannelIds.insert(item.channelId).second)
        {
            continue;
        }
        auto channelIter = channelIndex.find(item.channelId);
        if (channelIter == channelIndex.end())
        {
            continue;
        }
        const auto &channelRecords = channelIter->second;
        auto recordIter = std::lower_bound(channelRecords.begin(), channelRecords.end(), endSyscnt,
                                           [](const CCUChannelPrefixInfo &record, uint64_t endTime)
                                           { return record.timestamp < endTime; });
        if (recordIter == channelRecords.begin())
        {
            continue;
        }
        --recordIter;
        if (recordIter->maxDelay == 0)
        {
            continue;
        }
        if (!found || recordIter->maxDelay > maxDelay.channelDelay ||
            (recordIter->maxDelay == maxDelay.channelDelay && recordIter->channelId < maxDelay.channelId))
        {
            maxDelay.channelId = recordIter->channelId;
            maxDelay.channelDelay = recordIter->maxDelay;
            found = true;
        }
    }
    return maxDelay;
}

CCUChannelIndex BuildChannelIndex(const std::vector<CCUChannelInfo> &channelInfo)
{
    std::unordered_map<uint16_t, std::vector<CCUChannelInfo>> groupedChannelInfo;
    for (const auto &channel : channelInfo)
    {
        groupedChannelInfo[channel.channelId].emplace_back(channel);
    }

    CCUChannelIndex channelIndex;
    channelIndex.reserve(groupedChannelInfo.size());
    for (auto &item : groupedChannelInfo)
    {
        auto &records = item.second;
        std::sort(records.begin(), records.end(), [](const CCUChannelInfo &left, const CCUChannelInfo &right)
                  { return left.timestamp < right.timestamp; });
        auto &prefixRecords = channelIndex[item.first];
        if (!Reserve(prefixRecords, records.size()))
        {
            ERROR("Reserve ccu channel index failed.");
            continue;
        }
        uint64_t maxDelay = 0;
        uint16_t maxDelayChannel = UINT16_MAX;
        for (const auto &record : records)
        {
            if (record.avgDelay > maxDelay)
            {
                maxDelay = record.avgDelay;
                maxDelayChannel = record.channelId;
            }
            CCUChannelPrefixInfo prefixRecord;
            prefixRecord.timestamp = record.timestamp;
            prefixRecord.maxDelay = maxDelay;
            prefixRecord.channelId = maxDelayChannel;
            prefixRecords.emplace_back(prefixRecord);
        }
    }
    return channelIndex;
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

void SetGroupArgs(CCUInstructionArgs &args, const CCUGroupInfo &hostData, double durationNs)
{
    args.hasDieId = true;
    args.dieId = hostData.dieId;
    args.hasDataSize = true;
    args.dataSize = hostData.dataSize;
    const double durationUs = durationNs / NS_TO_US;
    if (durationUs > 0)
    {
        args.hasBandwidth = true;
        args.bandwidth = static_cast<double>(hostData.dataSize) / durationUs;
    }
    if (hostData.reduceOpType != "RESERVED")
    {
        args.hasReduceInfo = true;
        args.reduceOpType = hostData.reduceOpType;
        args.inputDataType = hostData.inputDataType;
        args.outputDataType = hostData.outputDataType;
    }
}

void SetWaitArgs(CCUInstructionArgs &args, const std::vector<CCUWaitSignalInfo> &hostRows, uint64_t endSyscnt,
                 const CCUChannelIndex &channelIndex)
{
    const auto &hostData = hostRows.front();
    args.hasDieId = true;
    args.dieId = hostData.dieId;
    args.hasMask = true;
    args.mask = hostData.mask;
    auto maxDelay = GetMaxDelayChannel(hostRows, endSyscnt, channelIndex);
    if (maxDelay.channelId != UINT16_MAX && maxDelay.channelDelay != 0)
    {
        args.hasDelayChannel = true;
        args.maxDelayChannel = maxDelay.channelId;
        args.maxChannelDelay = maxDelay.channelDelay;
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
            SetGroupArgs(traceData, hostDataIter->second.front(), traceData.duration);
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
    auto channelIndex = BuildChannelIndex(channelInfo);
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
            SetWaitArgs(traceData, hostDataIter->second, data.endTime, channelIndex);
        }
        result.push_back(traceData);
    }
}

using SetCKBRows = std::vector<const OriCCUSetCKBV62Row *>;

bool ResolveSetCKBInterval(const SetCKBRows &records, uint64_t &startSyscnt, const OriCCUSetCKBV62Row *&endRow)
{
    const OriCCUSetCKBV62Row *startRow = nullptr;
    for (const auto *row : records)
    {
        switch (std::get<8>(*row))
        {
            case 1:  // A single timestamp takes precedence over paired timestamps.
                startSyscnt = std::get<5>(*row);
                endRow = row;
                return true;
            case 0:
                startRow = row;
                break;
            case 2:
                endRow = row;
                break;
            default:
                break;
        }
    }
    if (!startRow || !endRow)
    {
        return false;
    }
    startSyscnt = std::get<5>(*startRow);
    return startSyscnt <= std::get<5>(*endRow);
}

using CCUInstructionKey = std::tuple<uint32_t, uint16_t, uint16_t>;
using CCUGroupsByInstruction = std::map<CCUInstructionKey, std::vector<CCUGroupInfo>>;
using CCUWaitsByInstruction = std::map<CCUInstructionKey, std::vector<CCUWaitSignalInfo>>;
using CCUChannelIndices = std::unordered_map<uint16_t, CCUChannelIndex>;

CCUChannelIndices BuildV62ChannelIndices(const OriCCUChannelV62Data &rows)
{
    std::unordered_map<uint16_t, std::vector<CCUChannelInfo>> samplesByDie;
    for (const auto &row : rows)
    {
        CCUChannelInfo sample;
        uint16_t dieId;
        std::tie(sample.channelId, sample.timestamp, sample.avgDelay, dieId) = row;
        samplesByDie[dieId].push_back(sample);
    }
    CCUChannelIndices indices;
    for (const auto &item : samplesByDie)
    {
        indices.emplace(item.first, BuildChannelIndex(item.second));
    }
    return indices;
}

void ApplyV62GroupArgs(CCUV62TimelineData &event, const CCUGroupsByInstruction &groups)
{
    const auto it = groups.find({event.taskId, event.instructionId, event.dieId});
    if (it == groups.end())
    {
        return;
    }
    const auto &first = it->second.front();
    const auto metadata =
        std::tie(first.dieId, first.dataSize, first.reduceOpType, first.inputDataType, first.outputDataType);
    if (std::all_of(it->second.begin(), it->second.end(),
                    [&metadata](const CCUGroupInfo &row) {
                        return std::tie(row.dieId, row.dataSize, row.reduceOpType, row.inputDataType,
                                        row.outputDataType) == metadata;
                    }))
    {
        SetGroupArgs(event, first, event.duration);
    }
}

void ApplyV62WaitArgs(CCUV62TimelineData &event, const CCUWaitsByInstruction &waits, const CCUChannelIndices &indices)
{
    const auto it = waits.find({event.taskId, event.instructionId, event.dieId});
    if (it == waits.end())
    {
        return;
    }
    const auto &first = it->second.front();
    if (std::any_of(it->second.begin(), it->second.end(), [&first](const CCUWaitSignalInfo &row)
                    { return row.dieId != first.dieId || row.mask != first.mask; }))
    {
        return;
    }
    const auto index = indices.find(first.dieId);
    static const CCUChannelIndex emptyIndex;
    SetWaitArgs(event, it->second, event.waitEndSyscnt, index == indices.end() ? emptyIndex : index->second);
}

}  // namespace

CCUMissionProcessor::CCUMissionProcessor(const std::string &profPath) : CCUDataProcessor(profPath) {}

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

bool CCUDataProcessor::LoadHostAddInfo(OriCCUWaitSignalData &waitSignalData, OriCCUGroupData &groupData)
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
// -------------------- v6_2 implementation --------------------

CCUMissionV62Processor::CCUMissionV62Processor(const std::string &profPath) : CCUDataProcessor(profPath) {}

bool CCUMissionV62Processor::Process(DataInventory &dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<CCUV62TimelineData> result;
    bool flag = true;
    CCUGroupsByInstruction groups;
    CCUWaitsByInstruction waits;
    bool hostLoaded = false;
    for (const auto &devicePath : deviceList)
    {
        OriCCULoopGroupV62Data loopgroupData;
        OriCCUSetCKBV62Data setckbData;
        if (!LoadV62Data(devicePath, loopgroupData, setckbData))
        {
            ERROR("Load v6_2 ccu data failed, device path is %.", devicePath);
            flag = false;
            continue;
        }
        if (loopgroupData.empty() && setckbData.empty())
        {
            continue;
        }
        if (!hostLoaded)
        {
            hostLoaded = true;
            OriCCUGroupData groupRows;
            OriCCUWaitSignalData waitRows;
            if (!LoadHostAddInfo(waitRows, groupRows))
            {
                flag = false;
            }
            std::vector<CCUGroupInfo> groupData;
            std::vector<CCUWaitSignalInfo> waitData;
            ConvertGroupData(groupRows, groupData);
            ConvertWaitSignalData(waitRows, waitData);
            for (auto &row : groupData)
            {
                groups[{row.taskId, row.instrId, row.dieId}].push_back(std::move(row));
            }
            for (auto &row : waitData)
            {
                waits[{row.taskId, row.instrId, row.dieId}].push_back(std::move(row));
            }
        }
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
        std::vector<CCUV62TimelineData> oneDeviceData;
        FormatLoopGroupV62Timeline(loopgroupData, oneDeviceData, deviceId, params, record);
        FormatSetCKBV62Timeline(setckbData, oneDeviceData, deviceId, params, record);
        if (!oneDeviceData.empty())
        {
            FilterDataByStartTime(oneDeviceData, record.startTimeNs, PROCESSOR_NAME_CCU_MISSION_V62);
            OriCCUChannelV62Data channelData;
            if (!setckbData.empty() && !waits.empty() && !LoadChannelData(devicePath, channelData))
            {
                flag = false;
            }
            const auto indices = BuildV62ChannelIndices(channelData);
            for (auto &event : oneDeviceData)
            {
                if (event.timeType == CCU_TIME_TYPE_LOOP_GROUP)
                {
                    ApplyV62GroupArgs(event, groups);
                }
                else
                {
                    ApplyV62WaitArgs(event, waits, indices);
                }
            }
            result.insert(result.end(), oneDeviceData.begin(), oneDeviceData.end());
        }
    }
    if (result.empty())
    {
        if (flag)
        {
            WARN("No v6_2 ccu mission timeline data generated.");
        }
        return flag;
    }
    if (!SaveToDataInventory<CCUV62TimelineData>(std::move(result), dataInventory, PROCESSOR_NAME_CCU_MISSION_V62))
    {
        ERROR("Save v6_2 ccu data failed, %.", PROCESSOR_NAME_CCU_MISSION_V62);
        return false;
    }
    return flag;
}

bool CCUMissionV62Processor::LoadV62Data(const std::string &devicePath, OriCCULoopGroupV62Data &loopgroupData,
                                         OriCCUSetCKBV62Data &setckbData)
{
    DBInfo loopgroupDB("ccu.db", "OriginLoopGroupV62");
    DBInfo setckbDB("ccu.db", "OriginSetCKBV62");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, loopgroupDB.dbName});
    if (!loopgroupDB.ConstructDBRunner(dbPath) || !setckbDB.ConstructDBRunner(dbPath))
    {
        ERROR("Construct v6_2 ccu db runner failed.");
        return false;
    }
    auto status = CheckPathAndTable(dbPath, loopgroupDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    if (status == CHECK_SUCCESS)
    {
        std::string sql =
            "SELECT task_id, loopgroup_instr_id, loopgroup_start_syscnt, "
            "loopgroup_stop_syscnt, die_id FROM " +
            loopgroupDB.tableName + ";";
        if (!loopgroupDB.dbRunner->QueryData(sql, loopgroupData))
        {
            ERROR("Query v6_2 loopgroup data failed, db path is %.", dbPath);
            return false;
        }
    }
    status = CheckPathAndTable(dbPath, setckbDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    if (status == CHECK_SUCCESS)
    {
        std::string sql =
            "SELECT task_id, setckb_instr_id, loop_id, loop_cnt, setckb_ckbinfo, "
            "setckb_syscnt, udf_profiling_data, udf_flag, syscnt_type, lp_flg, die_id FROM " +
            setckbDB.tableName + ";";
        if (!setckbDB.dbRunner->QueryData(sql, setckbData))
        {
            ERROR("Query v6_2 setckb data failed, db path is %.", dbPath);
            return false;
        }
    }
    return true;
}

bool CCUMissionV62Processor::LoadChannelData(const std::string &devicePath, OriCCUChannelV62Data &channelData)
{
    DBInfo channelDB("ccu.db", "OriginChannelV62");
    auto dbPath = Utils::File::PathJoin({devicePath, SQLITE, channelDB.dbName});
    if (!channelDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    const auto status = CheckPathAndTable(dbPath, channelDB, false);
    if (status != CHECK_SUCCESS)
    {
        return status == NOT_EXIST;
    }
    const std::string sql =
        "SELECT channel_id, timestamp, avg_delay, die_id FROM " + channelDB.tableName + " WHERE timestamp <> '0';";
    if (!channelDB.dbRunner->QueryData(sql, channelData))
    {
        ERROR("Query v6_2 ccu channel data failed, db path is %.", dbPath);
        return false;
    }
    return true;
}

void CCUMissionV62Processor::FormatLoopGroupV62Timeline(const OriCCULoopGroupV62Data &loopgroupData,
                                                        std::vector<CCUV62TimelineData> &result, uint16_t deviceId,
                                                        const Utils::SyscntConversionParams &params,
                                                        const Utils::ProfTimeRecord &record)
{
    if (loopgroupData.empty())
    {
        return;
    }
    if (!Reserve(result, result.size() + loopgroupData.size()))
    {
        ERROR("Reserve v6_2 loopgroup timeline data failed.");
        return;
    }
    for (const auto &row : loopgroupData)
    {
        CCUV62TimelineData traceData;
        uint64_t startSyscnt = 0;
        uint64_t stopSyscnt = 0;
        std::tie(traceData.taskId, traceData.instructionId, startSyscnt, stopSyscnt, traceData.dieId) = row;

        traceData.deviceId = deviceId;
        traceData.hasDieId = true;
        traceData.timeType = CCU_TIME_TYPE_LOOP_GROUP;
        HPFloat start = GetTimeFromSyscnt(startSyscnt, params);
        traceData.timestamp = GetLocalTime(start, record).Uint64();
        traceData.duration =
            stopSyscnt > startSyscnt ? GetDurTimeFromSyscnt(stopSyscnt - startSyscnt, params).Double() : 0.0;
        result.push_back(traceData);
    }
}

void CCUMissionV62Processor::FormatSetCKBV62Timeline(const OriCCUSetCKBV62Data &setckbData,
                                                     std::vector<CCUV62TimelineData> &result, uint16_t deviceId,
                                                     const Utils::SyscntConversionParams &params,
                                                     const Utils::ProfTimeRecord &record)
{
    using LoopKey = std::tuple<CCUInstructionKey, uint16_t, uint16_t>;
    std::map<CCUInstructionKey, SetCKBRows> missionGroups;
    std::map<LoopKey, SetCKBRows> loopGroups;
    for (const auto &row : setckbData)
    {
        const CCUInstructionKey key{std::get<0>(row), std::get<1>(row), std::get<10>(row)};
        const auto lpFlg = std::get<9>(row);
        if (lpFlg == 0)
        {
            missionGroups[key].push_back(&row);
        }
        else if (lpFlg == 1)
        {
            loopGroups[{key, std::get<3>(row), std::get<2>(row)}].push_back(&row);
        }
    }

    for (const auto &entry : missionGroups)
    {
        uint64_t startSyscnt = 0;
        const OriCCUSetCKBV62Row *endRow = nullptr;
        if (!ResolveSetCKBInterval(entry.second, startSyscnt, endRow))
        {
            continue;
        }
        const uint64_t endSyscnt = std::get<5>(*endRow);
        CCUV62TimelineData traceData;
        std::tie(traceData.taskId, traceData.instructionId, traceData.dieId) = entry.first;
        traceData.deviceId = deviceId;
        traceData.hasDieId = true;
        traceData.timeType = CCU_TIME_TYPE_SETCKB;
        traceData.setckbCkbinfo = std::get<4>(*endRow);
        traceData.hasUdf = std::get<7>(*endRow) == 1;
        traceData.udfProfilingData = std::get<6>(*endRow);
        traceData.waitEndSyscnt = std::get<8>(*endRow) == 2 ? endSyscnt : 0;
        traceData.lpFlg = 0;
        HPFloat start = GetTimeFromSyscnt(startSyscnt, params);
        traceData.timestamp = GetLocalTime(start, record).Uint64();
        traceData.duration =
            endSyscnt > startSyscnt ? GetDurTimeFromSyscnt(endSyscnt - startSyscnt, params).Double() : 0.0;
        result.push_back(std::move(traceData));
    }

    struct LoopTimeInfo
    {
        uint64_t startSyscnt;
        uint64_t endSyscnt;
        uint16_t loopId;
        bool hasWaitEnd;
    };
    std::map<CCUInstructionKey, std::vector<LoopTimeInfo>> instrLoops;
    for (const auto &entry : loopGroups)
    {
        uint64_t startSyscnt = 0;
        const OriCCUSetCKBV62Row *endRow = nullptr;
        if (!ResolveSetCKBInterval(entry.second, startSyscnt, endRow))
        {
            continue;
        }
        const uint64_t endSyscnt = std::get<5>(*endRow);
        instrLoops[std::get<0>(entry.first)].push_back(
            {startSyscnt, endSyscnt, std::get<2>(entry.first), std::get<8>(*endRow) == 2});
    }

    // Loop groups are ordered by loop count and then loop ID, so the first tie wins.
    for (const auto &entry : instrLoops)
    {
        const auto &loops = entry.second;
        const auto earliest =
            std::min_element(loops.begin(), loops.end(), [](const LoopTimeInfo &a, const LoopTimeInfo &b)
                             { return a.startSyscnt < b.startSyscnt; });
        const auto latest =
            std::max_element(loops.begin(), loops.end(),
                             [](const LoopTimeInfo &a, const LoopTimeInfo &b) { return a.endSyscnt < b.endSyscnt; });
        const auto longest =
            std::max_element(loops.begin(), loops.end(), [](const LoopTimeInfo &a, const LoopTimeInfo &b)
                             { return a.endSyscnt - a.startSyscnt < b.endSyscnt - b.startSyscnt; });

        CCUV62TimelineData traceData;
        std::tie(traceData.taskId, traceData.instructionId, traceData.dieId) = entry.first;
        traceData.deviceId = deviceId;
        traceData.hasDieId = true;
        traceData.timeType = CCU_TIME_TYPE_SETCKB;
        traceData.lpFlg = 1;
        if (std::any_of(loops.begin(), loops.end(), [&latest](const LoopTimeInfo &loop)
                        { return loop.endSyscnt == latest->endSyscnt && loop.hasWaitEnd; }))
        {
            traceData.waitEndSyscnt = latest->endSyscnt;
        }
        traceData.earliestLoopId = earliest->loopId;
        traceData.latestLoopId = latest->loopId;
        traceData.longestLoopId = longest->loopId;
        traceData.longestDuration = GetDurTimeFromSyscnt(longest->endSyscnt - longest->startSyscnt, params).Double();
        HPFloat start = GetTimeFromSyscnt(earliest->startSyscnt, params);
        traceData.timestamp = GetLocalTime(start, record).Uint64();
        traceData.duration = latest->endSyscnt > earliest->startSyscnt
                                 ? GetDurTimeFromSyscnt(latest->endSyscnt - earliest->startSyscnt, params).Double()
                                 : 0.0;
        result.push_back(std::move(traceData));
    }
}
}  // namespace Domain
}  // namespace Analysis
