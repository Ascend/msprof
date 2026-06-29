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

#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
ComputeTaskInfoProcessor::ComputeTaskInfoProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool ComputeTaskInfoProcessor::Process(DataInventory &dataInventory)
{
    DBInfo geInfoDB("ge_info.db", "TaskInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, geInfoDB.dbName});
    if (!geInfoDB.ConstructDBRunner(dbPath))
    {
        return false;
    }
    // 并不是所有场景都有ge info数据
    auto status = CheckPathAndTable(dbPath, geInfoDB);
    if (status == CHECK_FAILED)
    {
        return false;
    }
    else if (status == NOT_EXIST)
    {
        return true;
    }
    auto hashMap = dataInventory.GetPtr<std::unordered_map<std::string, std::string>>();
    if (hashMap == nullptr)
    {
        ERROR("Can't get hash data.");
        return false;
    }
    auto formatData = LoadData(geInfoDB, dbPath, *hashMap);
    if (formatData.empty())
    {
        ERROR("TaskInfo format data is empty. DBPath is %", dbPath);
        return false;
    }
    if (!SaveToDataInventory<TaskInfoData>(std::move(formatData), dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_COMPUTE_TASK_INFO);
        return false;
    }
    return true;
}

std::vector<TaskInfoData> ComputeTaskInfoProcessor::LoadData(const DBInfo &taskInfoDB, const std::string &dbPath,
                                                             std::unordered_map<std::string, std::string> &hashMap)
{
    std::vector<TaskInfoFormat> oriData;
    std::vector<TaskInfoData> res;
    if (taskInfoDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return res;
    }
    std::string sql{
        "SELECT hashid, model_id, op_name, stream_id, task_id, block_num, mix_block_num, task_type, "
        "op_type, op_flag, batch_id, IFNULL(input_formats, 'NULL'), IFNULL(input_data_types, 'NULL'), "
        "IFNULL(input_shapes, 'NULL'), IFNULL(output_formats, 'NULL'), IFNULL(output_data_types, 'NULL'), "
        "IFNULL(output_shapes, 'NULL'), device_id, context_id, (case when op_state is 'N/A' then 'N/A' "
        "when op_state is '1' then 'dynamic' when op_state is '0' then 'static' end), "
        "IFNULL(grid_dim, 'N/A'), IFNULL(block_dim, 'N/A') FROM " +
        taskInfoDB.tableName};
    if (!taskInfoDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Failed to obtain data from the % table.", taskInfoDB.tableName);
    }
    if (oriData.empty())
    {
        ERROR("TaskInfo original data is empty, DBPath is %", dbPath);
        return res;
    }
    if (!Utils::Reserve(res, oriData.size()))
    {
        ERROR("Reserve for TaskInfo data failed");
        return res;
    }
    TaskInfoData data;
    ShapeInfo info;
    std::string hashId;
    for (auto &node : oriData)
    {
        std::tie(hashId, data.modelId, data.opName, data.streamId, data.taskId, data.blockNum, data.mixBlockNum,
                 data.taskType, data.opType, data.opFlag, data.batchId, info.inputFormats, info.inputDataTypes,
                 info.inputShapes, info.outputFormats, info.outputDataTypes, info.outputShapes, data.deviceId,
                 data.contextId, data.opState, data.gridDim, data.blockDim) = node;
        if (hashMap.find(hashId) != hashMap.end())
        {
            data.hashId = hashMap[hashId];
        }
        data = info;
        res.push_back(data);
    }
    return res;
}
}  // namespace Domain
}  // namespace Analysis
