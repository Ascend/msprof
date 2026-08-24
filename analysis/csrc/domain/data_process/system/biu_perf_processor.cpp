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
#include "analysis/csrc/domain/data_process/system/biu_perf_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;

BiuPerfProcessor::BiuPerfProcessor(const std::string& profPath) : DataProcessor(profPath) {}

OriBiuPerfFormat LoadBiuPerfData(const DBInfo& dbInfo)
{
    OriBiuPerfFormat oriData;
    std::string sql{"SELECT group_id, core_type, block_id, instruction, timestamp, duration, checkpoint_info FROM " +
                    dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return oriData;
}

std::vector<BiuPerfData> FormatBiuPerfData(const OriBiuPerfFormat& oriData, const LocaltimeContext& localtimeContext,
                                           SyscntConversionParams& params)
{
    std::vector<BiuPerfData> formatData;
    if (!Utils::Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for BiuPerf data failed.");
        return formatData;
    }
    BiuPerfData tempData;
    for (const auto& row : oriData)
    {
        double syscnt = 0.0;
        double duration = 0.0;
        std::tie(tempData.groupId, tempData.coreType, tempData.blockId, tempData.instruction, syscnt, duration,
                 tempData.checkpointInfo) = row;
        HPFloat timestamp{GetTimeFromSyscnt(static_cast<uint64_t>(syscnt), params)};
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        tempData.duration = GetDurTimeFromSyscnt(duration, params).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}

bool BiuPerfProcessor::ProcessSingleDevice(const std::string& devicePath, std::vector<BiuPerfData>& allProcessedData,
                                           LocaltimeContext& localtimeContext, SyscntConversionParams& params)
{
    DBInfo biuPerfDB("biu_perf.db", "BiuInstrStatus");
    if (localtimeContext.deviceId == Environment::INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR("Failed to obtain the time in start_info and end_info, profPath is %.", profPath_);
        return false;
    }
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, biuPerfDB.dbName});
    if (!biuPerfDB.ConstructDBRunner(dbPath) || biuPerfDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, biuPerfDB, false);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    OriBiuPerfFormat oriData = LoadBiuPerfData(biuPerfDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", biuPerfDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatBiuPerfData(oriData, localtimeContext, params);
    if (processedData.empty())
    {
        ERROR("Format BiuPerf data error, dbPath is %.", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

bool BiuPerfProcessor::Process(DataInventory& dataInventory)
{
    LocaltimeContext localtimeContext;
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<BiuPerfData> allProcessedData;
    for (const auto& devicePath : deviceList)
    {
        localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
        SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, localtimeContext.deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        }
        flag = ProcessSingleDevice(devicePath, allProcessedData, localtimeContext, params) && flag;
    }
    if (!SaveToDataInventory<BiuPerfData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_NPU_MEM))
    {
        flag = false;
        ERROR("Save BiuPerf Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

}  // namespace Domain
}  // namespace Analysis
