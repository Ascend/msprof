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

#include "analysis/csrc/domain/data_process/ai_task/fusion_op_processor.h"

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

FusionOpProcessor::FusionOpProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool FusionOpProcessor::Process(DataInventory &dataInventory)
{
    DBInfo fusionOpDB("ge_model_info.db", "GeFusionOpInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, fusionOpDB.dbName});
    if (!fusionOpDB.ConstructDBRunner(dbPath))
    {
        return false;
    }

    auto status = CheckPathAndTable(dbPath, fusionOpDB, false);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }
    auto fusionOpData = LoadData(fusionOpDB, dbPath);
    if (fusionOpData.empty())
    {
        ERROR("fusion op original data is empty. DBPath is %", dbPath);
        return false;
    }

    auto processedData = FormatData(fusionOpData);
    if (processedData.empty())
    {
        ERROR("format fusion op data error");
        return false;
    }

    if (!SaveToDataInventory<FusionOpInfo>(std::move(processedData), dataInventory, PROCESSOR_NAME_FUSION_OP))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_FUSION_OP);
        return false;
    }
    return true;
}

OriFusionOpDataFormat FusionOpProcessor::LoadData(const DBInfo &fusionOpDB, const std::string &dbPath)
{
    OriFusionOpDataFormat oriData;
    if (fusionOpDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql =
        "SELECT model_id, fusion_name, fusion_op_nums, op_names, memory_input/1024.0,"
        "memory_output/1024.0, memory_weight/1024.0, memory_workspace/1024.0,"
        "memory_total/1024.0 FROM " +
        fusionOpDB.tableName;
    if (!fusionOpDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query fusion op failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<FusionOpInfo> FusionOpProcessor::FormatData(const OriFusionOpDataFormat &oriData)
{
    std::vector<FusionOpInfo> formatData;
    if (!Reserve(formatData, oriData.size()))
    {
        ERROR("Reserved for fusion op data failed.");
        return formatData;
    }
    FusionOpInfo data;
    for (const auto &row : oriData)
    {
        std::tie(data.modelId, data.fusionName, data.fusionOpNums, data.opNames, data.memoryInput, data.memoryOutput,
                 data.memoryWeight, data.memoryWorkspace, data.memoryTotal) = row;
        formatData.push_back(data);
    }
    return formatData;
}
}  // namespace Domain
}  // namespace Analysis
