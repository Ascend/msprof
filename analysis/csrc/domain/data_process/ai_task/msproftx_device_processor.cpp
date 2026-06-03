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

#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
MsprofTxDeviceProcessor::MsprofTxDeviceProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool MsprofTxDeviceProcessor::ProcessOneDevice(std::vector<MsprofTxDeviceData> &res, const std::string &devPath)
{
    uint16_t deviceId = GetDeviceIdByDevicePath(devPath);
    if (deviceId == INVALID_DEVICE_ID)
    {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
        return false;
    }
    Utils::SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_))
    {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    DBInfo stepTraceDB("step_trace.db", "StepTrace");
    std::string dbPath = Utils::File::PathJoin({devPath, SQLITE, stepTraceDB.dbName});
    if (!stepTraceDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, stepTraceDB, false);
    if (status != CHECK_SUCCESS)
    {
        if (status == CHECK_FAILED)
        {
            return false;
        }
        return true;
    }
    auto oriData = LoadData(stepTraceDB, dbPath);
    if (oriData.empty())
    {
        WARN("StepTrace for msprofTx original data is empty. DBPath is %", dbPath);
        return true;
    }
    auto formatData = FormatData(oriData, record, deviceId, params);
    if (formatData.empty())
    {
        ERROR("StepTrace for msprofTx data format failed, DBPath is %", dbPath);
        return false;
    }
    FilterDataByStartTime(formatData, record.startTimeNs, PROCESSOR_NAME_TASK);
    res.insert(res.end(), formatData.begin(), formatData.end());
    return true;
}

bool MsprofTxDeviceProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<MsprofTxDeviceData> res;
    for (const auto &devicePath : deviceList)
    {
        flag = ProcessOneDevice(res, devicePath) && flag;
    }
    if (!SaveToDataInventory<MsprofTxDeviceData>(std::move(res), dataInventory, PROCESSOR_NAME_TASK))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
        flag = false;
    }
    return flag;
}

OriMsprofTxDeviceData MsprofTxDeviceProcessor::LoadData(const DBInfo &stepTraceDB, const std::string &dbPath)
{
    OriMsprofTxDeviceData oriData;
    if (stepTraceDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT model_id, index_id, stream_id, task_id, timestamp, tag_id FROM " + stepTraceDB.tableName +
                    " WHERE tag_id = 11 or tag_id = 12"};  // 11: mark data; 12: range data
    if (!stepTraceDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", stepTraceDB.tableName);
    }
    return oriData;
}

std::vector<MsprofTxDeviceData> MsprofTxDeviceProcessor::FormatData(OriMsprofTxDeviceData &oriData,
                                                                    const ProfTimeRecord &record,
                                                                    const uint16_t deviceId,
                                                                    const SyscntConversionParams &params)
{
    std::vector<MsprofTxDeviceData> processedData;
    if (!Utils::Reserve(processedData, oriData.size()))
    {
        ERROR("Reserve for AscendTask data failed.");
        return processedData;
    }
    auto cannVersion = Context::GetInstance().GetCannVersion(deviceId, profPath_);
    bool isLegacy = cannVersion.empty() || (cannVersion[0] < 9) || (cannVersion[0] == 9 && cannVersion[1] < 1);
    if (isLegacy)
    {
        FormatDataByLegacyRule(processedData, oriData, record, deviceId, params);
    }
    else
    {
        OriMsprofTxDeviceData oriMarkData;
        OriMsprofTxDeviceData oriRangeData;
        for (const auto &data : oriData)
        {
            uint32_t tagId = std::get<5>(data);
            if (tagId == 11)
            {
                oriMarkData.push_back(data);
            }
            else if (tagId == 12)
            {
                oriRangeData.push_back(data);
            }
            else
            {
                ERROR("Unexpected tag_id % in msprofTx device data, profPath is %.", tagId, profPath_);
            }
        }
        FormatDataByMarkData(processedData, oriMarkData, record, deviceId, params);
        FormatDataByRangeData(processedData, oriRangeData, record, deviceId, params);
    }

    return processedData;
}

void MsprofTxDeviceProcessor::FormatDataByLegacyRule(std::vector<MsprofTxDeviceData> &processedData,
                                                     OriMsprofTxDeviceData &oriData, const ProfTimeRecord &record,
                                                     const uint16_t deviceId, const SyscntConversionParams &params)
{
    MsprofTxDeviceData data;
    uint64_t start;
    data.deviceId = deviceId;
    std::sort(oriData.begin(), oriData.end(),
              [](TxDeviceData &lData, TxDeviceData rData)
              {
                  if (std::get<1>(lData) != std::get<1>(rData))
                  {  // 按照index_id、timestamp排序，即第1、4位
                      return std::get<1>(lData) < std::get<1>(rData);  // 第1为为index_id
                  }
                  else
                  {
                      return std::get<4>(lData) < std::get<4>(rData);  // 第4位为timestamp
                  }
              });
    for (const auto &row : oriData)
    {
        std::tie(data.modelId, data.indexId, data.streamId, data.taskId, start, std::ignore) = row;
        data.connectionId = data.indexId + START_CONNECTION_ID_MSTX;
        HPFloat startTimestamp = Utils::GetTimeFromSyscnt(start, params);
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        if (!processedData.empty() && data.indexId == processedData.back().indexId)
        {
            processedData.back().duration = static_cast<double>(data.timestamp - processedData.back().timestamp);
        }
        else
        {
            processedData.push_back(data);
        }
    }
}

void MsprofTxDeviceProcessor::FormatDataByMarkData(std::vector<MsprofTxDeviceData> &processedData,
                                                   OriMsprofTxDeviceData &oriData, const ProfTimeRecord &record,
                                                   const uint16_t deviceId, const SyscntConversionParams &params)
{
    MsprofTxDeviceData data;
    data.deviceId = deviceId;
    uint64_t start = 0;
    for (const auto &row : oriData)
    {
        std::tie(data.modelId, data.indexId, data.streamId, data.taskId, start, std::ignore) = row;
        data.connectionId = data.indexId + START_CONNECTION_ID_MSTX;
        HPFloat startTimestamp = Utils::GetTimeFromSyscnt(start, params);
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        processedData.push_back(data);
    }
}

void MsprofTxDeviceProcessor::FormatDataByRangeData(std::vector<MsprofTxDeviceData> &processedData,
                                                    OriMsprofTxDeviceData &oriData, const ProfTimeRecord &record,
                                                    const uint16_t deviceId, const SyscntConversionParams &params)
{
    // sort range data by index_id and timestamp
    std::sort(oriData.begin(), oriData.end(),
              [](const TxDeviceData &l, const TxDeviceData &r)
              {
                  if (std::get<1>(l) != std::get<1>(r))
                  {
                      return std::get<1>(l) < std::get<1>(r);  // 1: index_id
                  }
                  return std::get<4>(l) < std::get<4>(r);  // 4: timestamp
              });

    // Traverse the data and pair them two by two (grouping adjacent entries with the same indexId)
    size_t idx = 0;
    const size_t total = oriData.size();
    while (idx < total)
    {
        const auto &rowFirst = oriData[idx];
        uint32_t currIndexId = std::get<1>(rowFirst);
        if (idx + 1 >= total || std::get<1>(oriData[idx + 1]) != currIndexId)
        {
            idx++;
            WARN("Unpaired range data with index_id % for device id %.", currIndexId, deviceId);
            continue;
        }
        const auto &rowSecond = oriData[idx + 1];
        MsprofTxDeviceData data;
        uint64_t startFirst = 0;
        std::tie(data.modelId, data.indexId, data.streamId, data.taskId, startFirst, std::ignore) = rowFirst;
        data.deviceId = deviceId;
        data.connectionId = data.indexId + START_CONNECTION_ID_MSTX;

        // range start time
        HPFloat tsFirst = Utils::GetTimeFromSyscnt(startFirst, params);
        uint64_t timeFirst = GetLocalTime(tsFirst, record).Uint64();

        // range end time
        uint64_t startSecond = std::get<4>(rowSecond);
        HPFloat tsSecond = Utils::GetTimeFromSyscnt(startSecond, params);
        uint64_t timeSecond = GetLocalTime(tsSecond, record).Uint64();

        data.timestamp = timeFirst;
        data.duration = static_cast<double>(timeSecond - timeFirst);

        processedData.push_back(data);

        idx += 2;  // move to the next pair
    }
}
}  // namespace Domain
}  // namespace Analysis
