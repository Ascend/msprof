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
#include "analysis/csrc/domain/data_process/system/llc_processor.h"

#include <cstddef>
#include <map>
#include <unordered_set>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/config.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
using namespace analysis::dvvp::common::config;

namespace
{
std::unordered_set<std::string> LLC_MODES = {LLC_PROFILING_READ, LLC_PROFILING_WRITE};

}

LLcProcessor::LLcProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool LLcProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<LLcData> allProcessedData;
    std::vector<LLcSummaryData> allSummaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData, allSummaryData) && flag;
    }
    if (!SaveToDataInventory<LLcData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_LLC) ||
        !SaveToDataInventory<LLcSummaryData>(std::move(allSummaryData), dataInventory, PROCESSOR_NAME_LLC))
    {
        flag = false;
        ERROR("Save LLC Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool LLcProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<LLcData> &allProcessedData,
                                       std::vector<LLcSummaryData> &allSummaryData)
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
    DBInfo llcDB("llc.db", "LLCMetrics");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, llcDB.dbName});
    if (!llcDB.ConstructDBRunner(dbPath) || llcDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有LLC数据
    auto status = CheckPathAndTable(dbPath, llcDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    std::string mode = Context::GetInstance().GetLLCProfiling(localtimeContext.deviceId, profPath_);
    if (LLC_MODES.find(mode) == LLC_MODES.end())
    {
        ERROR("GetLLCProfiling failed in path: %, device id is %", profPath_, localtimeContext.deviceId);
        return false;
    }
    OriLLcData oriData = LoadData(llcDB, localtimeContext.deviceId);
    if (oriData.empty())
    {
        ERROR("Get % data failed, profPath is %, device is %", llcDB.tableName, profPath_, localtimeContext.deviceId);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext, mode);
    if (processedData.empty())
    {
        ERROR("Format LLC data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_LLC);

    auto summaryData = ProcessSummaryData(localtimeContext.deviceId, oriData, mode);
    if (summaryData.empty())
    {
        ERROR("Process LLC summary data error, dbPath is %.", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    allSummaryData.insert(allSummaryData.end(), summaryData.begin(), summaryData.end());
    return true;
}

std::vector<LLcSummaryData> LLcProcessor::ProcessSummaryData(uint16_t deviceId, const OriLLcData &oriData,
                                                             const std::string &mode)
{
    std::vector<LLcSummaryData> processedData;
    struct Totals
    {
        double hitRate = 0;
        double throughput = 0;
        std::size_t count = 0;
    };
    std::map<uint32_t, Totals> byLlcId;
    for (const auto &row : oriData)
    {
        uint32_t llcId;
        double timestamp, hitRate, throughput;
        std::tie(llcId, timestamp, hitRate, throughput) = row;
        auto &totals = byLlcId[llcId];
        totals.hitRate += hitRate;
        totals.throughput += throughput;
        ++totals.count;
    }
    if (byLlcId.empty())
    {
        return processedData;
    }
    LLcSummaryData average;
    average.deviceId = deviceId;
    average.mode = mode;
    average.isAverage = true;
    double hitRateSum = 0;
    double throughputSum = 0;
    for (const auto &entry : byLlcId)
    {
        LLcSummaryData item;
        item.deviceId = deviceId;
        item.mode = mode;
        item.llcId = entry.first;
        item.hitRate = entry.second.hitRate / entry.second.count;
        item.throughput = entry.second.throughput / entry.second.count;
        item.hitRate *= PERCENTAGE;
        hitRateSum += item.hitRate;
        throughputSum += item.throughput;
        processedData.emplace_back(item);
    }
    average.hitRate = hitRateSum / processedData.size();
    average.throughput = throughputSum / processedData.size();
    processedData.insert(processedData.begin(), average);
    return processedData;
}

OriLLcData LLcProcessor::LoadData(const DBInfo &llcDB, uint16_t deviceId)
{
    OriLLcData oriData;
    std::string sql{"SELECT l3tid, timestamp, hitrate, throughput FROM " + llcDB.tableName +
                    " WHERE device_id = " + std::to_string(deviceId)};
    if (!llcDB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", llcDB.tableName);
    }
    return oriData;
}

std::vector<LLcData> LLcProcessor::FormatData(const OriLLcData &oriData, const LocaltimeContext &localtimeContext,
                                              const std::string &mode)
{
    std::vector<LLcData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for LLC data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    LLcData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    tempData.mode = mode;
    double oriTimestamp;
    for (const auto &row : oriData)
    {
        std::tie(tempData.llcID, oriTimestamp, tempData.hitRate, tempData.throughput) = row;
        tempData.timestamp = HPFloat(oriTimestamp).Uint64();
        tempData.hitRate = tempData.hitRate * PERCENTAGE;
        formatData.emplace_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
