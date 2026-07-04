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
#include "analysis/csrc/domain/data_process/system/hbm_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

HBMProcessor::HBMProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool HBMProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<HbmData> allProcessedData;
    std::vector<HbmSummaryData> allSummaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData, allSummaryData) && flag;
    }
    if (!SaveToDataInventory<HbmData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_HBM) ||
        !SaveToDataInventory<HbmSummaryData>(std::move(allSummaryData), dataInventory, PROCESSOR_NAME_HBM))
    {
        flag = false;
        ERROR("Save memory on chip Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool HBMProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<HbmData> &allProcessedData,
                                       std::vector<HbmSummaryData> &allSummaryData)
{
    LocaltimeContext localtimeContext;
    localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (localtimeContext.deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified, profPath is %.", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId))
    {
        ERROR(
            "Failed to obtain the time in start_info and end_info, "
            "profPath is %, device id is %.",
            profPath_, localtimeContext.deviceId);
        return false;
    }
    DBInfo hbmDB("hbm.db", "HBMbwData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, hbmDB.dbName});
    if (!hbmDB.ConstructDBRunner(dbPath) || hbmDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有memory on chip数据
    auto status = CheckPathAndTable(dbPath, hbmDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    auto processedData = ProcessData(hbmDB, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format memory on chip data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_HBM);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());

    auto summaryData = ProcessSummaryData(localtimeContext.deviceId, hbmDB);
    if (summaryData.empty())
    {
        ERROR("Process memory on chip summary data error, dbPath is %.", dbPath);
        return false;
    }
    allSummaryData.insert(allSummaryData.end(), summaryData.begin(), summaryData.end());
    return true;
}

std::vector<HbmData> HBMProcessor::ProcessData(const DBInfo &hbmDB, LocaltimeContext &localtimeContext)
{
    OriHbmData oriData = LoadData(hbmDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed, profPath is %, device is %", hbmDB.tableName, profPath_, localtimeContext.deviceId);
        return {};
    }
    return FormatData(oriData, localtimeContext);
}

std::vector<HbmSummaryData> HBMProcessor::ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hbmDB)
{
    std::vector<HbmSummaryData> processedData;
    std::vector<std::tuple<double, double>> avgData;
    std::vector<std::tuple<uint32_t, double, double>> avgHbmIdData;
    // csv表里面的第一行，所有数据的read和write的平均带宽
    std::string sql{
        "SELECT AVG(CASE WHEN event_type = 'read' THEN bandwidth END) AS read_bandwidth, "
        "AVG(CASE WHEN event_type = 'write' THEN bandwidth END) AS write_bandwidth FROM " +
        hbmDB.tableName};
    if (!hbmDB.dbRunner->QueryData(sql, avgData) || avgData.empty())
    {
        ERROR("Failed to obtain avg bandwidth from the % table, profPath is %, deviceId is %", hbmDB.tableName,
              profPath_, deviceId);
        return processedData;
    }
    // 按照hbmid分组的read和write的平均带宽
    sql =
        "SELECT hbmid, AVG(CASE WHEN event_type = 'read' THEN bandwidth END) AS read_bandwidth,"
        "AVG(CASE WHEN event_type = 'write' THEN bandwidth END) AS write_bandwidth FROM " +
        hbmDB.tableName + " GROUP BY hbmid";
    if (!hbmDB.dbRunner->QueryData(sql, avgHbmIdData) || avgHbmIdData.empty())
    {
        ERROR("Failed to obtain avg bandwidth from the % table, profPath is %, deviceId is %", hbmDB.tableName,
              profPath_, deviceId);
        return processedData;
    }
    HbmSummaryData tempData;
    // 在这里赋值可以在外面根据deviceId判断是否查询失败
    tempData.deviceId = deviceId;
    // 这里vector已经判断过是否为空，所以可以取固定的索引
    std::tie(tempData.readBandwidth, tempData.writeBandwidth) = avgData.at(0);
    // 先将所有数据的平均值放进去，然后一一放每个hbmid的平均值
    processedData.push_back(tempData);
    for (const auto &row : avgHbmIdData)
    {
        std::tie(tempData.hbmId, tempData.readBandwidth, tempData.writeBandwidth) = row;
        processedData.push_back(tempData);
    }
    return processedData;
}

OriHbmData HBMProcessor::LoadData(const DBInfo &hbmDB)
{
    OriHbmData oriData;
    std::string sql{"SELECT timestamp, bandwidth, hbmid, event_type FROM " + hbmDB.tableName};
    if (!hbmDB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", hbmDB.tableName);
    }
    return oriData;
}

std::vector<HbmData> HBMProcessor::FormatData(const OriHbmData &oriData, const LocaltimeContext &localtimeContext)
{
    std::vector<HbmData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for memory on chip data failed, profPath is %, deviceId is %.", profPath_,
              localtimeContext.deviceId);
        return formatData;
    }
    HbmData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(oriTimestamp, tempData.bandWidth, tempData.hbmId, tempData.eventType) = row;
        tempData.timestamp = HPFloat(oriTimestamp).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
