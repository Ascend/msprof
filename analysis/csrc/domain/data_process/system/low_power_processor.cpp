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

#include "analysis/csrc/domain/data_process/system/low_power_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

LowPowerProcessor::LowPowerProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool LowPowerProcessor::Process(DataInventory& dataInventory)
{
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (!Context::GetInstance().IsChipV6(version))
    {
        WARN("This platformVersion: % does not support the processing of low power data.", version);
        return true;
    }
    bool flag = true;
    std::vector<LowPowerData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
    {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID)
        {
            ERROR("The invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        ProfTimeRecord timeRecord;
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_, deviceId))
        {
            ERROR("Failed to GetProfTimeRecordInfo, fileDir is %, device id is %.", profPath_, deviceId);
            flag = false;
            continue;
        }
        OriLowPowerDataFormat oriLowPowerData;
        if (!ProcessData(devicePath, oriLowPowerData))
        {
            ERROR("Get original data failed.");
            flag = false;
            continue;
        }
        std::vector<LowPowerData> processedData;
        if (!FormatData(deviceId, timeRecord, oriLowPowerData, processedData))
        {
            ERROR("FormatData failed, fileDir is %.", devicePath);
            flag = false;
            continue;
        }
        res.insert(res.end(), processedData.begin(), processedData.end());
    }
    if (!SaveToDataInventory<LowPowerData>(std::move(res), dataInventory, PROCESSOR_NAME_LOW_POWER))
    {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_LOW_POWER);
    }
    return flag;
}

bool LowPowerProcessor::ProcessData(const std::string& devicePath, OriLowPowerDataFormat& oriData)
{
    DBInfo dbInfo("lowpower.db", "LowPower");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
    if (!dbInfo.ConstructDBRunner(dbPath))
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, dbInfo, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == CHECK_SUCCESS)
    {
        oriData = LoadData(dbPath, dbInfo);
    }
    if (oriData.empty())
    {
        WARN("Original data is empty. DBPath is %", dbPath);
    }
    return true;
}

LowPowerProcessor::OriLowPowerDataFormat LowPowerProcessor::LoadData(const std::string& dbPath, DBInfo& lowPowerDB)
{
    INFO("LowPowerProcessor GetData, dir is %", dbPath);
    OriLowPowerDataFormat oriData;
    if (lowPowerDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT timestamp, die_id, data0_soft FROM " + lowPowerDB.tableName;
    if (!lowPowerDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query lowpower data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool LowPowerProcessor::FormatData(const uint16_t& deviceId, const ProfTimeRecord& timeRecord,
                                   const OriLowPowerDataFormat& oriLowPowerData,
                                   std::vector<LowPowerData>& processedData)
{
    INFO("LowPowerProcessor FormatData.");
    if (oriLowPowerData.empty())
    {
        WARN("freq original data is empty, it will use default freq.");
    }
    if (!Reserve(processedData, oriLowPowerData.size()))
    {
        ERROR("Reserve for freq data failed.");
        return false;
    }
    LowPowerData lowPowerData;
    lowPowerData.deviceId = deviceId;
    double oriTimestamp;
    for (auto& row : oriLowPowerData)
    {
        std::tie(oriTimestamp, lowPowerData.dieId, lowPowerData.freq) = row;
        HPFloat timestamp = oriTimestamp;
        lowPowerData.timestamp = GetLocalTime(timestamp, timeRecord).Uint64();
        processedData.push_back(lowPowerData);
    }
    std::sort(processedData.begin(), processedData.end(),
              [](const LowPowerData& ld, const LowPowerData& rd) { return ld.timestamp < rd.timestamp; });
    return true;
}
}  // namespace Domain
}  // namespace Analysis
