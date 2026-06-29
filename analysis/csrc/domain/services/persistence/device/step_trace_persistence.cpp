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
#include "analysis/csrc/domain/services/persistence/device/step_trace_persistence.h"

#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
namespace
{
const uint16_t STEP_START_TAG = 60000;
const uint16_t STEP_END_TAG = 60001;
}  // namespace

StepTraceDataVectorFormat GenerateStepTraceData(std::map<uint32_t, std::vector<StepTraceTasks>>& stepTraceTask)
{
    StepTraceDataVectorFormat processedData;
    for (auto& it : stepTraceTask)
    {
        auto model_id = it.first;
        uint32_t iter_id = 0;
        for (const auto& element : it.second)
        {
            processedData.emplace_back(element.indexId, model_id, element.stepTrace.start, element.stepTrace.end,
                                       iter_id);
            iter_id++;
        }
    }
    return processedData;
}

uint32_t ProcessStepTraceDataEntry(DataInventory& dataInventory, const DeviceContext& context, DBInfo& stepTraceDB)
{
    auto stepTraceTask = dataInventory.GetPtr<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    if (!stepTraceTask || stepTraceTask->empty())
    {
        WARN("StepTraceTasks is empty.");
        return ANALYSIS_OK;
    }
    std::string dbPath = Utils::File::PathJoin({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    auto data = GenerateStepTraceData(*stepTraceTask);
    auto res = SaveData(data, stepTraceDB, dbPath);
    if (!res)
    {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t ProcessStepTimeEntry(DataInventory& dataInventory, const DeviceContext& context, DBInfo& stepTraceDB)
{
    auto halTrackData = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!halTrackData || halTrackData->empty())
    {
        WARN("halTrackData is empty.");
        return ANALYSIS_OK;
    }
    std::string dbPath = Utils::File::PathJoin({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    auto data = GenerateStepTime(*halTrackData);
    if (data.empty())
    {
        WARN("StepTime data is empty.");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, stepTraceDB, dbPath);
    if (!res)
    {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t StepTracePersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    DBInfo stepTraceInfo("step_trace.db", "step_trace_data");
    MAKE_SHARED0_NO_OPERATION(stepTraceInfo.database, StepTraceDB);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, stepTraceInfo.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(stepTraceInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    bool res = ANALYSIS_OK;
    res |= ProcessStepTraceDataEntry(dataInventory, deviceContext, stepTraceInfo);
    stepTraceInfo.tableName = "StepTime";
    res |= ProcessStepTimeEntry(dataInventory, deviceContext, stepTraceInfo);
    return res;
}

}  // namespace Domain
}  // namespace Analysis
