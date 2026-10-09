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

#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
const uint32_t DEFAULT_CONTEXT_ID = UINT32_MAX;

MemcpyInfoProcessor::MemcpyInfoProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool MemcpyInfoProcessor::Process(DataInventory &dataInventory)
{
    DBInfo runtimeDB("runtime.db", "MemcpyInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, runtimeDB.dbName});
    if (!runtimeDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    // 并不是所有场景都有memcpyInfo数据
    auto status = CheckPathAndTable(dbPath, runtimeDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }
    auto memcpyInfoData = LoadData(runtimeDB, dbPath);
    if (memcpyInfoData.empty())
    {
        ERROR("MemcpyInfo original data is empty. DBPath is %", dbPath);
        return false;
    }
    auto processedData = FormatData(memcpyInfoData);
    if (processedData.empty())
    {
        ERROR("format memcpyInfo data error");
        return false;
    }
    if (!SaveToDataInventory<MemcpyInfoData>(std::move(processedData), dataInventory, PROCESSOR_NAME_MEMCPY_INFO))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_MEMCPY_INFO);
        return false;
    }
    return true;
}

MemcpyInfoFormat MemcpyInfoProcessor::LoadData(const DBInfo &runtimeDB, const std::string &dbPath)
{
    MemcpyInfoFormat oriData;
    if (runtimeDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT stream_id, batch_id, task_id, context_id, device_id, data_size, memcpy_direction FROM " +
                      runtimeDB.tableName;
    if (!runtimeDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query memcpyInfo data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<MemcpyInfoData> MemcpyInfoProcessor::FormatData(const MemcpyInfoFormat &oriData)
{
    std::vector<MemcpyInfoData> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserved for api data failed.");
        return formatData;
    }
    MemcpyInfoData data;
    for (const auto &row : oriData)
    {
        std::tie(data.taskId.streamId, data.taskId.batchId, data.taskId.taskId, data.taskId.contextId,
                 data.taskId.deviceId, data.dataSize, data.memcpyOperation) = row;
        formatData.push_back(data);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
