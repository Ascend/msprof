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
#include "analysis/csrc/domain/data_process/system/soc_bandwidth_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

SocBandwidthProcessor::SocBandwidthProcessor(const std::string& profPaths) : DataProcessor(profPaths) {}

OriSocDataFormat SocBandwidthProcessor::LoadData(const DBInfo& socProfilerDB, const std::string& dbPath)
{
    OriSocDataFormat oriData;
    if (socProfilerDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT l2_buffer_bw_level, mata_bw_level, sys_time "
        "FROM " +
        socProfilerDB.tableName};
    if (!socProfilerDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", socProfilerDB.tableName);
    }
    return oriData;
}

std::vector<SocBandwidthData> SocBandwidthProcessor::FormatData(const OriSocDataFormat& oriData,
                                                                const Utils::ProfTimeRecord& timeRecord,
                                                                const uint16_t deviceId)
{
    std::vector<SocBandwidthData> processedData;
    SocBandwidthData data;
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for SocBandwidthData data failed.");
        return processedData;
    }
    double oriTimestamp;
    data.deviceId = deviceId;
    for (auto& row : oriData)
    {
        std::tie(data.l2BufferBwLevel, data.mataBwLevel, oriTimestamp) = row;
        HPFloat timestamp{oriTimestamp};
        data.timestamp = GetLocalTime(timestamp, timeRecord).Uint64();
        processedData.push_back(data);
    }
    return processedData;
}

bool SocBandwidthProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<SocBandwidthData> res;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, res) && flag;
    }
    if (!SaveToDataInventory(std::move(res), dataInventory, PROCESSOR_NAME_SOC))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_SOC);
        flag = false;
    }
    return flag;
}

bool SocBandwidthProcessor::ProcessSingleDevice(const std::string& devicePath, std::vector<SocBandwidthData>& res)
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
        ERROR(
            "Failed to obtain the time in start_info and end_info. "
            "Prof path is %, device id is %.",
            profPath_, deviceId);
        return false;
    }
    DBInfo socProfilerDB("soc_profiler.db", "InterSoc");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, socProfilerDB.dbName});
    if (!socProfilerDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, socProfilerDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    auto oriData = LoadData(socProfilerDB, dbPath);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", socProfilerDB.tableName, dbPath);
        return false;
    }
    auto formatData = FormatData(oriData, timeRecord, deviceId);
    if (formatData.empty())
    {
        ERROR("Soc data format failed, DBPath is %", dbPath);
        return false;
    }
    FilterDataByStartTime(formatData, timeRecord.startTimeNs, PROCESSOR_NAME_SOC);
    res.insert(res.end(), formatData.begin(), formatData.end());
    return true;
}
}  // namespace Domain
}  // namespace Analysis
