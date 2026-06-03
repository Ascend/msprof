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
#include "analysis/csrc/domain/data_process/system/sio_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
namespace
{
constexpr double TMP_UINT = static_cast<double>(NANO_SECOND) / (BYTE_SIZE * BYTE_SIZE);
const std::unordered_map<uint16_t, std::string> SERIES_MAP{{0, "die 0"}, {1, "die 1"}};
const std::unordered_map<uint16_t, std::string> SERIES_V6_MAP{{0, "D-DIE0"}, {2, "U-DIE0"}, {3, "U-DIE1"}};
}  // namespace

SioProcessor::SioProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool SioProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    std::vector<SioData> allProcessedData;
    auto version = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (Context::GetInstance().IsChipV6(version))
    {
        seriesMap_ = SERIES_V6_MAP;
    }
    else if (Context::GetInstance().IsChipV4(version) || version == static_cast<int>(Chip::CHIP_V1_1_1))
    {
        seriesMap_ = SERIES_MAP;
    }
    else
    {
        INFO("Current platform % does not support sioData", version);
        return true;
    }
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<SioData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_SIO))
    {
        flag = false;
        ERROR("Save sio Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool SioProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<SioData> &allProcessedData)
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
    DBInfo sioDB("sio.db", "Sio");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, sioDB.dbName});
    if (!sioDB.ConstructDBRunner(dbPath) || sioDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有sio数据
    auto status = CheckPathAndTable(dbPath, sioDB);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    SioProcessor::OriSioData oriData = LoadData(sioDB);
    if (oriData.empty())
    {
        ERROR("Get % data failed, profPath is %, device is %", sioDB.tableName, profPath_, localtimeContext.deviceId);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty())
    {
        ERROR("Format sio data error, dbPath is %.", dbPath);
        return false;
    }
    FilterDataByStartTime(processedData, localtimeContext.timeRecord.startTimeNs, PROCESSOR_NAME_SIO);
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

SioProcessor::OriSioData SioProcessor::LoadData(const DBInfo &sioDB)
{
    SioProcessor::OriSioData oriData;
    std::string sql{"SELECT acc_id, timestamp, req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx FROM " +
                    sioDB.tableName};
    if (!sioDB.dbRunner->QueryData(sql, oriData) || oriData.empty())
    {
        ERROR("Failed to obtain data from the % table.", sioDB.tableName);
    }
    return oriData;
}

std::vector<SioData> SioProcessor::FormatData(const SioProcessor::OriSioData &oriData,
                                              const LocaltimeContext &localtimeContext)
{
    std::vector<SioData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for sio data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    SioData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    std::unordered_map<uint16_t, double> timestampMap;
    double oriTimestamp;
    uint32_t reqRxDataSize;
    uint32_t rspRxDataSize;
    uint32_t snpRxDataSize;
    uint32_t datRxDataSize;
    uint32_t reqTxDataSize;
    uint32_t rspTxDataSize;
    uint32_t snpTxDataSize;
    uint32_t datTxDataSize;
    for (const auto &row : oriData)
    {
        std::tie(tempData.dieId, oriTimestamp, reqRxDataSize, rspRxDataSize, snpRxDataSize, datRxDataSize,
                 reqTxDataSize, rspTxDataSize, snpTxDataSize, datTxDataSize) = row;
        if (seriesMap_.find(tempData.dieId) != seriesMap_.end())
        {
            tempData.name = seriesMap_[tempData.dieId];
        }
        if (timestampMap.find(tempData.dieId) != timestampMap.end() && timestampMap[tempData.dieId] < oriTimestamp)
        {
            double lastTimeStamp = timestampMap[tempData.dieId];
            double duration = oriTimestamp - lastTimeStamp;
            tempData.reqRxBandwidth = reqRxDataSize * TMP_UINT / duration;
            tempData.rspRxBandwidth = rspRxDataSize * TMP_UINT / duration;
            tempData.snpRxBandwidth = snpRxDataSize * TMP_UINT / duration;
            tempData.datRxBandwidth = datRxDataSize * TMP_UINT / duration;
            tempData.reqTxBandwidth = reqTxDataSize * TMP_UINT / duration;
            tempData.rspTxBandwidth = rspTxDataSize * TMP_UINT / duration;
            tempData.snpTxBandwidth = snpTxDataSize * TMP_UINT / duration;
            tempData.datTxBandwidth = datTxDataSize * TMP_UINT / duration;
            timestampMap[tempData.dieId] = oriTimestamp;
        }
        else
        {
            timestampMap.insert({tempData.dieId, oriTimestamp});
            continue;
        }
        HPFloat timestamp = oriTimestamp;
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
