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
#include "analysis/csrc/domain/data_process/system/hccs_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

HCCSProcessor::HCCSProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool HCCSProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<HccsData> allProcessedData;
    std::vector<HccsSummaryData> allSummaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData, allSummaryData) && flag;
    }
    if (!SaveToDataInventory<HccsData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_HCCS) ||
        !SaveToDataInventory<HccsSummaryData>(std::move(allSummaryData), dataInventory, PROCESSOR_NAME_HCCS))
    {
        flag = false;
        ERROR("Save HCCS Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool HCCSProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<HccsData> &allProcessedData,
                                        std::vector<HccsSummaryData> &allSummaryData)
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
    DBInfo hccsDB("hccs.db", "HCCSEventsData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, hccsDB.dbName});
    if (!hccsDB.ConstructDBRunner(dbPath) || hccsDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有HCCS数据
    auto status = CheckPathAndTable(dbPath, hccsDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    auto processedData = ProcessData(hccsDB, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format HCCS data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_HCCS);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());

    auto summaryData = ProcessSummaryData(localtimeContext.deviceId, hccsDB);
    if (summaryData.deviceId == UINT16_MAX)
    {
        ERROR("Process HCCS summary data error, dbPath is %.", dbPath);
        return false;
    }
    allSummaryData.push_back(summaryData);
    return true;
}

std::vector<HccsData> HCCSProcessor::ProcessData(const DBInfo &hccsDB, LocaltimeContext &localtimeContext)
{
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_))
    {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return {};
    }
    OriHccsData oriData = LoadData(hccsDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed, profPath is %, device is %", hccsDB.tableName, profPath_, localtimeContext.deviceId);
        return {};
    }
    return FormatData(oriData, localtimeContext);
}

HccsSummaryData HCCSProcessor::ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hccsDB)
{
    HccsSummaryData processedData;
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>> summaryData;
    std::string sql{
        "SELECT MAX(txthroughput), MIN(txthroughput), AVG(txthroughput), MAX(rxthroughput),"
        "MIN(rxthroughput), AVG(rxthroughput) FROM " +
        hccsDB.tableName};
    if (!hccsDB.dbRunner->QueryData(sql, summaryData) || summaryData.empty())
    {
        ERROR("Failed to obtain throughput data from the % table, profPath is %, deviceId is %", hccsDB.tableName,
              profPath_, deviceId);
        return processedData;
    }
    processedData.deviceId = deviceId;
    // 这里vector已经判断过是否为空，所以可以取固定的索引
    std::tie(processedData.rxMaxThroughput, processedData.rxMinThroughput, processedData.rxAvgThroughput,
             processedData.txMaxThroughput, processedData.txMinThroughput, processedData.txAvgThroughput) =
        summaryData.at(0);
    return processedData;
}

OriHccsData HCCSProcessor::LoadData(const DBInfo &hccsDB)
{
    OriHccsData oriData;
    std::string sql{"SELECT timestamp, txthroughput, rxthroughput FROM " + hccsDB.tableName};
    if (!hccsDB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", hccsDB.tableName);
    }
    return oriData;
}

std::vector<HccsData> HCCSProcessor::FormatData(const OriHccsData &oriData, const LocaltimeContext &localtimeContext)
{
    std::vector<HccsData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for Hccs data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    HccsData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(oriTimestamp, tempData.txThroughput, tempData.rxThroughput) = row;
        HPFloat timestamp =
            GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic, localtimeContext.deviceMonotonic);
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
