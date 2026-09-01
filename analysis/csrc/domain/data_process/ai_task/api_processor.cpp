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

#include "analysis/csrc/domain/data_process/ai_task/api_processor.h"

#include <algorithm>

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

bool isContainedInRange(const std::vector<ApiData> &modelLoadDatas, const ApiData &data)
{
    for (const auto &modelLoadData : modelLoadDatas)
    {
        if (data.threadId == modelLoadData.threadId && data.timestamp >= modelLoadData.timestamp &&
            data.end <= modelLoadData.end)
        {
            return true;
        }
    }
    return false;
}

ApiProcessor::ApiProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool ApiProcessor::Process(DataInventory &dataInventory)
{
    DBInfo apiDB("api_event.db", "ApiData");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, apiDB.dbName});
    if (!apiDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    // 并不是所有场景都有api数据
    auto status = CheckPathAndTable(dbPath, apiDB);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }
    auto apiData = LoadData(apiDB, dbPath);
    if (apiData.empty())
    {
        ERROR("Api original data is empty. DBPath is %", dbPath);
        return false;
    }
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_))
    {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, profPath_))
    {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    auto processedData = FormatData(apiData, record, params);
    if (processedData.empty())
    {
        ERROR("format api data error");
        return false;
    }
    std::vector<ApiData> modelLoadDatas;
    for (const auto &data : processedData)
    {
        if (data.apiName == "ModelLoad")
        {
            modelLoadDatas.push_back(data);
        }
    }
    FilterDataByStartTime<ApiData>(processedData, record.startTimeNs, PROCESSOR_NAME_API,
                                   [modelLoadDatas](const ApiData &data, uint64_t startTimeNs)
                                   {
                                       bool inRange = isContainedInRange(modelLoadDatas, data);
                                       bool isFiltered = data.end >= startTimeNs;
                                       return !(inRange || isFiltered);
                                   });
    if (!SaveToDataInventory<ApiData>(std::move(processedData), dataInventory, PROCESSOR_NAME_API))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_API);
        return false;
    }
    return true;
}

OriApiDataFormat ApiProcessor::LoadData(const DBInfo &apiDB, const std::string &dbPath)
{
    OriApiDataFormat oriData;
    if (apiDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    auto tableColumns = apiDB.dbRunner->GetTableColumns(apiDB.tableName);
    bool hasKey = std::any_of(tableColumns.begin(), tableColumns.end(),
                              [](const Infra::TableColumn &column) { return column.name == "key"; });
    std::string keyColumn = hasKey ? "key" : "0 AS key";
    std::string sql = "SELECT struct_type, id, level, thread_id, item_id, start, end, connection_id, " + keyColumn +
                      " "
                      "FROM " +
                      apiDB.tableName;
    if (!apiDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query api data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<ApiData> ApiProcessor::FormatData(const OriApiDataFormat &oriData, const ProfTimeRecord &record,
                                              const SyscntConversionParams &params)
{
    std::vector<ApiData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserved for api data failed.");
        return formatData;
    }
    ApiData data;
    std::string level;
    for (const auto &row : oriData)
    {
        std::tie(data.structType, data.id, level, data.threadId, data.itemId, data.timestamp, data.end,
                 data.connectionId, data.event_id) = row;
        HPFloat startTimestamp = Utils::GetTimeFromHostCnt(data.timestamp, params);
        HPFloat endTimestamp = Utils::GetTimeFromHostCnt(data.end, params);
        data.level = GetEnumTypeValue(level, NAME_STR(API_LEVEL_TABLE), API_LEVEL_TABLE);
        data.apiName = data.structType;
        if (data.level == MSPROF_REPORT_ACL_LEVEL)
        {
            data.apiName = data.id;
        }
        else if (data.level == MSPROF_REPORT_HCCL_NODE_LEVEL)
        {
            data.apiName = data.itemId;
        }
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        data.end = GetLocalTime(endTimestamp, record).Uint64();
        formatData.push_back(data);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
