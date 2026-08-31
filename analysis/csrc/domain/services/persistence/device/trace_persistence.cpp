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

#include "analysis/csrc/domain/services/persistence/device/trace_persistence.h"

#include <algorithm>

#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "persistence_utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
namespace
{
const uint16_t FP_START_INDEX = 3;
const uint16_t STEP_END_INDEX = 5;
const uint16_t DATA_AUG_BOUND_INDEX = 9;
const uint16_t STEP_TIME_INDEX_ID_INDEX = 0;
const uint16_t STEP_TIME_MODEL_ID_INDEX = 1;
const uint16_t STEP_TIME_STEP_START_INDEX = 2;
const uint16_t STEP_TIME_STEP_END_INDEX = 3;
}  // namespace
// device_id, model_id, index_id, iteration_end, start, end
using AllReduceFormat = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t>>;
using AllReduceNullEndFormat =
    std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint64_t, uint64_t, std::nullptr_t>>;
struct AllReduceData
{
    AllReduceFormat completed;
    AllReduceNullEndFormat incomplete;
};
// model_id, index_id, start_time, end_time
using GetNextFormat = std::vector<std::tuple<uint64_t, uint32_t, uint64_t, uint64_t>>;
// device_id, model_id, iteration_id, FP_start, BP_end, iteration_end, iteration_time, fp_bp_time, grad_refresh_bound,
// data_aug_bound
using TrainingTraceFormat = std::vector<
    std::tuple<uint32_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
using TrainingTraceInnerFormat =
    std::tuple<uint32_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;

bool CompareStepEnd(const TrainingTraceInnerFormat &a, const TrainingTraceInnerFormat &b)
{
    return std::get<STEP_END_INDEX>(a) < std::get<STEP_END_INDEX>(b);
}

void ProcessSingleAllReduce(const StepTraceTasks &stepTraceTask, AllReduceData &processedData, uint32_t modelId,
                            uint32_t deviceId)
{
    for (auto reduceList = stepTraceTask.allReduceTable.begin(); reduceList != stepTraceTask.allReduceTable.end();
         reduceList++)
    {
        for (const auto &eachReduce : reduceList->second)
        {
            if (eachReduce.end)
            {
                processedData.completed.emplace_back(deviceId, modelId, stepTraceTask.indexId,
                                                     stepTraceTask.stepTrace.end, eachReduce.start, eachReduce.end);
            }
            else
            {
                processedData.incomplete.emplace_back(deviceId, modelId, stepTraceTask.indexId,
                                                      stepTraceTask.stepTrace.end, eachReduce.start, nullptr);
            }
        }
    }
}

AllReduceData GenerateAllReduce(std::map<uint32_t, std::vector<StepTraceTasks>> &stepTraceTask, uint32_t deviceId)
{
    AllReduceData processedData;
    for (auto &it : stepTraceTask)
    {
        auto modelId = it.first;
        for (const auto &element : it.second)
        {
            ProcessSingleAllReduce(element, processedData, modelId, deviceId);
        }
    }
    return processedData;
}

void ProcessSingleGetNext(const StepTraceTasks &stepTraceTask, GetNextFormat &processedData, uint32_t modelId)
{
    for (const auto &nextList : stepTraceTask.getNextTable)
    {
        const auto &starts = nextList.second.starts;
        const auto &ends = nextList.second.ends;
        size_t startIndex = 0;
        size_t endIndex = 0;
        while (startIndex < starts.size() && endIndex < ends.size())
        {
            auto start = starts[startIndex++];
            auto end = ends[endIndex++];
            while (endIndex < ends.size() && end < start)
            {
                end = ends[endIndex++];
            }
            while (startIndex < starts.size() && starts[startIndex] <= end)
            {
                start = starts[startIndex++];
            }
            processedData.emplace_back(modelId, stepTraceTask.indexId, start, end);
        }
        if (startIndex < starts.size() || endIndex < ends.size())
        {
            WARN("GetNext data mismatch, model id: %, index id: %, start remain: %, end remain: %", modelId,
                 stepTraceTask.indexId, starts.size() - startIndex, ends.size() - endIndex);
        }
    }
}

GetNextFormat GenerateGetNext(std::map<uint32_t, std::vector<StepTraceTasks>> &stepTraceTask)
{
    GetNextFormat processedData;
    for (auto &it : stepTraceTask)
    {
        auto modelId = it.first;
        for (const auto &element : it.second)
        {
            ProcessSingleGetNext(element, processedData, modelId);
        }
    }
    return processedData;
}

int64_t findClosestStepEndIndex(const TrainingTraceFormat &processedData, int64_t currentIterIndex)
{
    int64_t lastIterIndex = currentIterIndex - 1;
    while (lastIterIndex >= 0)
    {
        if (std::get<FP_START_INDEX>(processedData[currentIterIndex]) >
            std::get<STEP_END_INDEX>(processedData[lastIterIndex]))
        {
            break;
        }
        lastIterIndex--;
    }
    if (lastIterIndex < currentIterIndex - 1)
    {
        WARN(
            "The last iter of the % iter of "
            "total iters is not %, but is %",
            currentIterIndex, currentIterIndex - 1, lastIterIndex);
    }
    return lastIterIndex;
}

void ProcessSingleTrainingTrace(const StepTraceTasks &stepTraceTask, TrainingTraceFormat &processedData,
                                uint32_t modelId, uint32_t deviceId)
{
    if (!stepTraceTask.stepTrace.start || !stepTraceTask.stepTrace.end ||
        stepTraceTask.stepTrace.start == stepTraceTask.stepTrace.end)
    {
        ERROR("Invalid step time, model id: %, index id: %, start: %, end: %", modelId, stepTraceTask.indexId,
              stepTraceTask.stepTrace.start, stepTraceTask.stepTrace.end);
        return;
    }
    const auto &trainingTrace = stepTraceTask.trainingTrace;
    uint64_t iterationTime = stepTraceTask.stepTrace.end - stepTraceTask.stepTrace.start;
    uint64_t fpBpTime = trainingTrace.start && trainingTrace.end ? trainingTrace.end - trainingTrace.start : 0;
    uint64_t gradRefreshBound = trainingTrace.end ? stepTraceTask.stepTrace.end - trainingTrace.end : 0;
    processedData.emplace_back(deviceId, modelId, stepTraceTask.indexId, trainingTrace.start, trainingTrace.end,
                               stepTraceTask.stepTrace.end, iterationTime, fpBpTime, gradRefreshBound, 0);
}

TrainingTraceFormat UpdateDataAugBound(TrainingTraceFormat &processedData)
{
    // 更新data_aug_bound
    std::sort(processedData.begin(), processedData.end(), CompareStepEnd);
    int64_t currentIterIndex = 0;
    for (auto &data : processedData)
    {
        if (std::get<FP_START_INDEX>(data))
        {
            auto lastIterIndex = findClosestStepEndIndex(processedData, currentIterIndex);
            if (lastIterIndex >= 0)
            {
                std::get<DATA_AUG_BOUND_INDEX>(data) =
                    std::get<FP_START_INDEX>(data) - std::get<STEP_END_INDEX>(processedData[lastIterIndex]);
            }
        }
        currentIterIndex++;
    }

    return processedData;
}

TrainingTraceFormat GenerateTrainingTraceFromHalTrace(std::vector<HalTrackData> &halTraceTasks,
                                                      TrainingTraceFormat &processedData, uint32_t deviceId)
{
    auto stepTimes = GenerateStepTime(halTraceTasks);
    if (stepTimes.empty())
    {
        WARN("StepTime data is empty.");
        return processedData;
    }
    for (auto &stepTime : stepTimes)
    {
        processedData.emplace_back(
            deviceId, std::get<STEP_TIME_MODEL_ID_INDEX>(stepTime), std::get<STEP_TIME_INDEX_ID_INDEX>(stepTime), 0, 0,
            std::get<STEP_TIME_STEP_END_INDEX>(stepTime),
            std::get<STEP_TIME_STEP_END_INDEX>(stepTime) - std::get<STEP_TIME_STEP_START_INDEX>(stepTime), 0, 0, 0);
    }
    return processedData;
}

TrainingTraceFormat GenerateTrainingTraceFromStepTrace(std::map<uint32_t, std::vector<StepTraceTasks>> &stepTraceTask,
                                                       TrainingTraceFormat &processedData, uint32_t deviceId)
{
    for (auto &it : stepTraceTask)
    {
        auto modelId = it.first;
        for (const auto &element : it.second)
        {
            ProcessSingleTrainingTrace(element, processedData, modelId, deviceId);
        }
    }
    return processedData;
}

uint32_t ProcessAllReduceEntry(DataInventory &dataInventory, const DeviceContext &context, DBInfo &stepTraceDB)
{
    auto stepTraceTask = dataInventory.GetPtr<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    if (!stepTraceTask || stepTraceTask->empty())
    {
        WARN("StepTraceTasks is empty.");
        return ANALYSIS_OK;
    }
    std::string dbPath = Utils::File::PathJoin({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    DeviceInfo deviceInfo{};
    context.Getter(deviceInfo);
    auto data = GenerateAllReduce(*stepTraceTask, deviceInfo.deviceId);
    if (!stepTraceDB.dbRunner->CreateTableWithConstraints(stepTraceDB.tableName,
                                                          stepTraceDB.database->GetTableCols(stepTraceDB.tableName),
                                                          {"PRIMARY KEY(device_id, iteration_end, start)"}))
    {
        ERROR("Create table with primary key failed: %", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    if (data.completed.empty() && data.incomplete.empty())
    {
        WARN("AllReduce data is empty.");
        return ANALYSIS_OK;
    }
    bool res = true;
    if (!data.completed.empty())
    {
        res &= SaveData(data.completed, stepTraceDB, dbPath);
    }
    if (!data.incomplete.empty())
    {
        res &= SaveData(data.incomplete, stepTraceDB, dbPath);
    }
    if (!res)
    {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t ProcessGetNextEntry(DataInventory &dataInventory, const DeviceContext &context, DBInfo &stepTraceDB)
{
    auto stepTraceTask = dataInventory.GetPtr<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    if (!stepTraceTask || stepTraceTask->empty())
    {
        WARN("StepTraceTasks is empty.");
        return ANALYSIS_OK;
    }
    std::string dbPath = Utils::File::PathJoin({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    auto data = GenerateGetNext(*stepTraceTask);
    if (data.empty())
    {
        WARN("GetNext data is empty.");
        if (stepTraceDB.dbRunner->CreateTable(stepTraceDB.tableName,
                                              stepTraceDB.database->GetTableCols(stepTraceDB.tableName)))
        {
            return ANALYSIS_OK;
        }
        ERROR("Create empty table failed: %", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
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

uint32_t ProcessTrainingTraceEntry(DataInventory &dataInventory, const DeviceContext &context, DBInfo &stepTraceDB)
{
    auto stepTraceTask = dataInventory.GetPtr<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    auto halTrackData = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!halTrackData || halTrackData->empty())
    {
        WARN("halTrackData is empty.");
        return ANALYSIS_OK;
    }
    std::string dbPath = Utils::File::PathJoin({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    DeviceInfo deviceInfo{};
    context.Getter(deviceInfo);
    TrainingTraceFormat processedData;
    if (stepTraceTask && !stepTraceTask->empty())
    {
        GenerateTrainingTraceFromStepTrace(*stepTraceTask, processedData, deviceInfo.deviceId);
    }
    GenerateTrainingTraceFromHalTrace(*halTrackData, processedData, deviceInfo.deviceId);
    UpdateDataAugBound(processedData);
    if (processedData.empty())
    {
        WARN("TrainingTrace data is empty.");
        if (stepTraceDB.dbRunner->CreateTable(stepTraceDB.tableName,
                                              stepTraceDB.database->GetTableCols(stepTraceDB.tableName)))
        {
            return ANALYSIS_OK;
        }
        ERROR("Create empty table failed: %", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    auto res = SaveData(processedData, stepTraceDB, dbPath);
    if (!res)
    {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t TracePersistence::ProcessEntry(DataInventory &dataInventory, const Context &context)
{
    const auto &deviceContext = dynamic_cast<const DeviceContext &>(context);
    DBInfo stepTraceInfo("trace.db", "all_reduce");
    MAKE_SHARED0_NO_OPERATION(stepTraceInfo.database, TraceDB);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, stepTraceInfo.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(stepTraceInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    bool res = ANALYSIS_OK;
    res |= ProcessAllReduceEntry(dataInventory, deviceContext, stepTraceInfo);
    stepTraceInfo.tableName = "get_next";
    res |= ProcessGetNextEntry(dataInventory, deviceContext, stepTraceInfo);
    stepTraceInfo.tableName = "training_trace";
    res |= ProcessTrainingTraceEntry(dataInventory, deviceContext, stepTraceInfo);
    return res;
}

}  // namespace Domain
}  // namespace Analysis
