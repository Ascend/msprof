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

#include "analysis/csrc/domain/data_process/system/acc_pmu_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
AccPmuProcessor::AccPmuProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool AccPmuProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<AccPmuData> res;
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessOneDevice(devicePath, res) && flag;
    }
    if (!SaveToDataInventory<AccPmuData>(std::move(res), dataInventory, PROCESSOR_NAME_ACC_PMU))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_ACC_PMU);
        flag = false;
    }
    return flag;
}

bool AccPmuProcessor::ProcessOneDevice(const std::string &devicePath, std::vector<AccPmuData> &res)
{
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    Utils::ProfTimeRecord timeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_, deviceId))
    {
        ERROR("Failed to get time record info data. Path is %, device id is %", profPath_, deviceId);
        return false;
    }
    DBInfo accPmuDB("acc_pmu.db", "AccPmu");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, accPmuDB.dbName});
    if (!accPmuDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, accPmuDB);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    auto oriData = LoadData(accPmuDB, dbPath);
    if (oriData.empty())
    {
        ERROR("Get acc_pmu original data failed in %.", dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, timeRecord, deviceId);
    if (processedData.empty())
    {
        ERROR("format acc_pmu data failed, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, timeRecord.startTimeNs, PROCESSOR_NAME_ACC_PMU);
    res.insert(res.end(), processedData.begin(), processedData.end());
    return true;
}

OriAccPmuData AccPmuProcessor::LoadData(const DBInfo &accPmuDB, const std::string &dbPath)
{
    OriAccPmuData oriData;
    if (accPmuDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp "
        "FROM " +
        accPmuDB.tableName + " ORDER BY timestamp ASC, acc_id ASC"};
    if (!accPmuDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", accPmuDB.tableName);
    }
    return oriData;
}

std::vector<AccPmuData> AccPmuProcessor::FormatData(const OriAccPmuData &oriData,
                                                    const Utils::ProfTimeRecord &timeRecord, const uint16_t deviceId)
{
    std::vector<AccPmuData> processedData;
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for AccPmu data failed.");
        return processedData;
    }
    AccPmuData data;
    uint32_t accId;
    double timestamp;
    data.deviceId = deviceId;
    for (const auto &row : oriData)
    {
        std::tie(accId, data.readBwLevel, data.writeBwLevel, data.readOstLevel, data.writeOstLevel, timestamp) = row;
        HPFloat time{timestamp};
        data.accId = static_cast<uint16_t>(accId);
        data.timestamp = GetLocalTime(time, timeRecord).Uint64();
        processedData.push_back(data);
    }
    return processedData;
}
}  // namespace Domain
}  // namespace Analysis
