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

#include "analysis/csrc/domain/services/persistence/device/ascend_task_persistence.h"

#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
// model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, host_task_type,
// device_task_type, connection_id
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t, double,
                                                   double, std::string, std::string, int64_t>>;

ProcessedDataFormat GenerateAscendTaskData(std::vector<TopDownTask>& ascendTask, bool dynamicFlag)
{
    ProcessedDataFormat processedData;
    for (auto& task : ascendTask)
    {
        if (dynamicFlag && task.isFirst)
        {  // 动态profiling且标识位true的数据不落盘
            continue;
        }
        double start_time = 0.0;
        double duration = 0.0;
        if (IsDoubleEqual(task.startTime, INVALID_TIME))
        {
            start_time = INVALID_TIME;
        }
        else
        {
            start_time = task.startTime;
        }
        if (IsDoubleEqual(task.endTime, INVALID_TIME))
        {
            duration = INVALID_TIME;
        }
        else
        {
            duration = task.endTime - start_time;
        }
        processedData.emplace_back(task.modelId, task.indexId, task.streamId, task.taskId, task.contextId, task.batchId,
                                   start_time, duration, task.hostTaskType, task.deviceTaskType, task.connectionId);
    }
    return processedData;
}

uint32_t AscendTaskPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto ascendTask = dataInventory.GetPtr<std::vector<TopDownTask>>();
    SampleInfo info;
    deviceContext.Getter(info);
    if (!ascendTask)
    {
        ERROR("ascend task data is null.");
        return ANALYSIS_ERROR;
    }
    if (ascendTask->empty())
    {
        WARN("no ascendTask don't persistence");
        return ANALYSIS_OK;
    }
    DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
    MAKE_SHARED0_RETURN_VALUE(ascendTaskDB.database, AscendTaskDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, ascendTaskDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(ascendTaskDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateAscendTaskData(*ascendTask, info.dynamic);
    if (data.empty() && info.dynamic)
    {
        WARN("The first task in dynamic is unbelievably, so remove the first data");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, ascendTaskDB, dbPath);
    if (res)
    {
        INFO("Process % done!", ascendTaskDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(AscendTaskPersistence, true, HcclCalculator);
REGISTER_PROCESS_DEPENDENT_DATA(AscendTaskPersistence, std::vector<TopDownTask>);
REGISTER_PROCESS_SUPPORT_CHIP(AscendTaskPersistence, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
