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

#include "analysis/csrc/domain/services/persistence/device/ts_track_persistence.h"

#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
// timestamp stream_id task_id task_type task_state
using TaskTypeDataFormat = std::tuple<uint64_t, uint32_t, uint32_t, uint64_t, uint16_t>;
// index_id model_id timestamp stream_id task_id tag_id
using StepTraceDataFormat = std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint32_t, uint16_t>;
// timestamp stream_id task_id task_state
using TaskMemcpyDataFormat = std::tuple<uint64_t, uint32_t, uint32_t, uint64_t>;
// timestamp stream_id task_id block_num
using BlockNumDataFormat = std::tuple<uint64_t, uint32_t, uint32_t, uint32_t>;
// stream_id timestamp task_id flip_num
using TaskFlipDataFormat = std::tuple<uint32_t, double, uint32_t, uint16_t>;

template <typename DataFormat>
bool ReplaceAndSaveData(const std::vector<DataFormat>& data, DBInfo& dbInfo, std::string& dbPath)
{
    if (dbInfo.dbRunner->CheckTableExists(dbInfo.tableName) && !dbInfo.dbRunner->DropTable(dbInfo.tableName))
    {
        ERROR("Drop table % failed", dbInfo.tableName);
        return false;
    }
    return SaveData(data, dbInfo, dbPath);
}

bool SaveTaskTypeData(const std::vector<HalTrackData>& dataS, const DeviceContext& deviceContext)
{
    DBInfo tsTrackDB("step_trace.db", "TaskType");
    MAKE_SHARED0_RETURN_VALUE(tsTrackDB.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, tsTrackDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(tsTrackDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    std::vector<TaskTypeDataFormat> taskTypeS;
    for (const auto& data : dataS)
    {
        HalTaskType taskType = data.taskType;
        taskTypeS.emplace_back(data.hd.timestamp, data.hd.taskId.streamId, data.hd.taskId.taskId, taskType.taskType,
                               taskType.taskStatus);
    }
    INFO("Process % done!", tsTrackDB.tableName);
    return ReplaceAndSaveData(taskTypeS, tsTrackDB, dbPath);
}

bool SaveStepTraceData(const std::vector<HalTrackData>& dataS, const DeviceContext& deviceContext)
{
    DBInfo tsTrackDB("step_trace.db", "StepTrace");
    MAKE_SHARED0_RETURN_VALUE(tsTrackDB.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, tsTrackDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(tsTrackDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    std::vector<StepTraceDataFormat> stepTraceTasks;
    for (const auto& data : dataS)
    {
        HalStepTrace stepTraceTask = data.stepTrace;
        stepTraceTasks.emplace_back(stepTraceTask.indexId, stepTraceTask.modelId, data.hd.timestamp,
                                    data.hd.taskId.streamId, data.hd.taskId.taskId, stepTraceTask.tagId);
    }
    INFO("Process % done!", tsTrackDB.tableName);
    return ReplaceAndSaveData(stepTraceTasks, tsTrackDB, dbPath);
}

bool SaveTsMemcpyData(const std::vector<HalTrackData>& dataS, const DeviceContext& deviceContext)
{
    DBInfo tsTrackDB("step_trace.db", "TsMemcpy");
    MAKE_SHARED0_RETURN_VALUE(tsTrackDB.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, tsTrackDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(tsTrackDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    std::vector<TaskMemcpyDataFormat> tsMemecpyTasks;
    for (const auto& data : dataS)
    {
        HalTaskMemcpy taskMemcpy = data.taskMemcpy;
        tsMemecpyTasks.emplace_back(data.hd.timestamp, data.hd.taskId.streamId, data.hd.taskId.taskId,
                                    taskMemcpy.taskStatus);
    }
    return ReplaceAndSaveData(tsMemecpyTasks, tsTrackDB, dbPath);
}

bool SaveBlockNumData(const std::vector<HalTrackData>& dataS, const DeviceContext& deviceContext)
{
    DBInfo tsTrackDB("step_trace.db", "TsBlockNum");
    MAKE_SHARED0_RETURN_VALUE(tsTrackDB.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, tsTrackDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(tsTrackDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    std::vector<BlockNumDataFormat> blockNumTaskS;
    for (const auto& data : dataS)
    {
        HalBlockNum blockNum = data.blockNum;
        blockNumTaskS.emplace_back(data.hd.timestamp, data.hd.taskId.streamId, data.hd.taskId.taskId,
                                   blockNum.blockNum);
    }
    return ReplaceAndSaveData(blockNumTaskS, tsTrackDB, dbPath);
}

bool SaveTaskFlipData(const std::vector<HalTrackData>& dataS, const DeviceContext& deviceContext)
{
    DBInfo tsTrackDB("step_trace.db", "DeviceTaskFlip");
    MAKE_SHARED0_RETURN_VALUE(tsTrackDB.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, tsTrackDB.dbName});
    MAKE_SHARED_RETURN_VALUE(tsTrackDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto params = GenerateSyscntConversionParams(deviceContext);
    std::vector<TaskFlipDataFormat> taskFlips;
    for (const auto& data : dataS)
    {
        auto timestamp = GetTimeFromSyscnt(data.hd.timestamp, params);
        taskFlips.emplace_back(data.hd.taskId.streamId, timestamp.Double(), data.hd.taskId.taskId, data.flip.flipNum);
    }
    return ReplaceAndSaveData(taskFlips, tsTrackDB, dbPath);
}

static const std::unordered_map<int, std::function<bool(const std::vector<HalTrackData>&, const DeviceContext&)>>
    type2SaveFunc{{HalTrackType::TS_TASK_FLIP, SaveTaskFlipData},
                  {HalTrackType::STEP_TRACE, SaveStepTraceData},
                  {HalTrackType::TS_TASK_TYPE, SaveTaskTypeData},
                  {HalTrackType::TS_MEMCPY, SaveTsMemcpyData},
                  {HalTrackType::BLOCK_NUM, SaveBlockNumData}};

bool SaveTrackData(const std::unordered_map<HalTrackType, std::vector<HalTrackData>>& type2Data,
                   const DeviceContext& deviceContext)
{
    bool saveStatus = true;
    for (const auto& it : type2Data)
    {
        const auto saveFunc = type2SaveFunc.find(it.first);
        if (saveFunc == type2SaveFunc.end())
        {
            WARN("Skip unsupported ts track persistence type: %", it.first);
            continue;
        }
        saveStatus &= saveFunc->second(it.second, deviceContext);
        INFO("Process % done!, type is %, status is %", it.first, saveStatus);
    }
    return saveStatus;
}

std::unordered_map<HalTrackType, std::vector<HalTrackData>> groupByType(std::vector<HalTrackData> dataS)
{
    std::unordered_map<HalTrackType, std::vector<HalTrackData>> afterGroupData;
    for (const auto& data : dataS)
    {
        afterGroupData[data.type].emplace_back(std::move(data));
    }
    return afterGroupData;
}

uint32_t TsTrackPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto halTrackTask = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!halTrackTask)
    {
        ERROR("hal track task data is null.");
        return ANALYSIS_ERROR;
    }
    auto data = groupByType(*halTrackTask);
    auto res = SaveTrackData(data, deviceContext);
    if (res)
    {
        INFO("Save tsTrack data success, size is %", halTrackTask->size());
        return ANALYSIS_OK;
    }
    ERROR("Save tsTrack data failed");
    return ANALYSIS_ERROR;
}

}  // namespace Domain
}  // namespace Analysis
