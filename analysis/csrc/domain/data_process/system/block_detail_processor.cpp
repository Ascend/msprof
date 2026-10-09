/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include "analysis/csrc/domain/data_process/system/block_detail_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;

BlockDetailProcessor::BlockDetailProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool BlockDetailProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<BlockDetailData> blockDetailRes;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList)
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
        OriBlockDetailDataFormat oriBlockDetailData;
        uint8_t status = ProcessData(devicePath, oriBlockDetailData);
        if (status == CHECK_FAILED)
        {
            ERROR("Get original data failed.");
            flag = false;
            continue;
        }
        else if (status == NOT_EXIST || oriBlockDetailData.empty())
        {
            continue;
        }
        auto processedData = FormatData(oriBlockDetailData, localtimeContext);
        if (processedData.empty())
        {
            ERROR("Format block detail data error.");
            return false;
        }
        blockDetailRes.insert(blockDetailRes.end(), processedData.begin(), processedData.end());
    }
    if (!SaveToDataInventory<BlockDetailData>(std::move(blockDetailRes), dataInventory, PROCESSOR_NAME_BLOCK_DETAIL))
    {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_BLOCK_DETAIL);
    }
    return flag;
}

uint8_t BlockDetailProcessor::ProcessData(const std::string& devicePath, OriBlockDetailDataFormat& oriData)
{
    DBInfo dbInfo("metric_summary.db", "V6BlockPmu");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
    if (!dbInfo.ConstructDBRunner(dbPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, dbInfo, false);
    if (status == CHECK_SUCCESS)
    {
        oriData = LoadData(dbPath, dbInfo);
        if (oriData.empty())
        {
            WARN("Original data is empty. DBPath is %.", dbPath);
        }
    }
    return status;
}

OriBlockDetailDataFormat BlockDetailProcessor::LoadData(const std::string& dbPath, DBInfo& blockDetailDB)
{
    INFO("BlockDetailProcessor GetData, dir is %.", dbPath);
    OriBlockDetailDataFormat oriData;
    if (blockDetailDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql =
        "SELECT stream_id, task_id, subtask_id, batch_id, start_time, duration, core_type, core_id"
        " FROM " +
        blockDetailDB.tableName;
    if (!blockDetailDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query block detail data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<BlockDetailData> BlockDetailProcessor::FormatData(const OriBlockDetailDataFormat& oriData,
                                                              const LocaltimeContext& localtimeContext)
{
    std::vector<BlockDetailData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserve for block detail data failed, profPath is %, deviceId is %.", profPath_,
              localtimeContext.deviceId);
        return formatData;
    }
    BlockDetailData tempData;
    double oriTimestamp;
    for (const auto& row : oriData)
    {
        std::tie(tempData.streamId, tempData.taskId, tempData.subtaskId, tempData.batchId, oriTimestamp,
                 tempData.duration, tempData.coreType, tempData.coreId) = row;
        HPFloat timestamp{oriTimestamp};
        tempData.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
