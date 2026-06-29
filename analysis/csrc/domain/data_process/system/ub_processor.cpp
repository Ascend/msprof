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
#include "analysis/csrc/domain/data_process/system/ub_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;

UbProcessor::UbProcessor(const std::string &profPath) : DataProcessor(profPath) {}

UbProcessor::OriUbFormat LoadUbData(const DBInfo &dbInfo)
{
    UbProcessor::OriUbFormat oriData;
    std::string sql{
        "SELECT device_id, port_id, time_stamp, udma_rx_bind,"
        " udma_tx_bind, rx_port_band_width, tx_port_band_width FROM " +
        dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return oriData;
}

std::vector<UbData> FormatUbData(const UbProcessor::OriUbFormat &oriData, const LocaltimeContext &localtimeContext)
{
    std::vector<UbData> formatData;
    if (!Utils::Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for ub data failed.");
        return formatData;
    }
    UbData tempData;
    for (const auto &row : oriData)
    {
        double timestampCnt = 0.0;
        std::tie(tempData.deviceId, tempData.portId, timestampCnt, tempData.udmaRxBind, tempData.udmaTxBind,
                 tempData.rxPortBandWidth, tempData.txPortBandWidth) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(timestampCnt * MS_TO_US, localtimeContext.hostMonotonic, 0);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}

bool UbProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<UbData> &allProcessedData,
                                      LocaltimeContext &localtimeContext)
{
    DBInfo ubDB("ub.db", "UBBwData");
    if (localtimeContext.deviceId == INVALID_DEVICE_ID)
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
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ubDB.dbName});
    if (!ubDB.ConstructDBRunner(dbPath) || ubDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    auto status = CheckPathAndTable(dbPath, ubDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    UbProcessor::OriUbFormat oriData = LoadUbData(ubDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed in %.", ubDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatUbData(oriData, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format ub data error, dbPath is %.", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

bool UbProcessor::Process(DataInventory &dataInventory)
{
    LocaltimeContext localtimeContext;
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<UbData> allProcessedData;
    for (const auto &devicePath : deviceList)
    {
        localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true,
                                                         localtimeContext.deviceId, profPath_) ||
            !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false,
                                                         localtimeContext.deviceId, profPath_))
        {
            ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
            return false;
        }
        flag = ProcessSingleDevice(devicePath, allProcessedData, localtimeContext) && flag;
    }
    if (!SaveToDataInventory<UbData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_UB))
    {
        flag = false;
        ERROR("Save ub Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}
}  // namespace Domain
}  // namespace Analysis
