/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
#include "analysis/csrc/application/timeline/block_detail_assembler.h"

#include <unordered_map>
#include <vector>

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
namespace
{
const std::string AIC_EARLIEST = "AIC Earliest";
const std::string AIC_LATEST = "AIC Latest";
const std::string AIV_EARLIEST = "AIV Earliest";
const std::string AIV_LATEST = "AIV Latest";
const std::vector<std::string> THREAD_ARGS_NAMES = {AIC_EARLIEST, AIC_LATEST, AIV_EARLIEST, AIV_LATEST};
const size_t THREAD_ARGS_NAMES_SIZE = 4;
}  // namespace

void BlockDetailEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
    ostream["Batch Id"] << batchId_;
    ostream["Subtask Id"] << subTaskId_;
    ostream["Core Id"] << coreId_;
}

BlockDetailAssembler::BlockDetailAssembler()
    : JsonAssembler(PROCESS_BLOCK_DETAIL, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

uint8_t BlockDetailAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream,
                                           const std::string& profPath)
{
    auto blockDetailData = dataInventory.GetPtr<std::vector<BlockDetailData>>();
    auto taskInfoData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (blockDetailData == nullptr)
    {
        WARN("Can't get blockDetailData from dataInventory.");
        return DATA_NOT_EXIST;
    }
    if (ascendTaskData == nullptr)
    {
        WARN("AscendTask not exists, can't export task time data.");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint32_t, std::vector<BlockDetailData>> blockDetailDataMap;
    if (!Reserve(*blockDetailData, blockDetailData->size()))
    {
        ERROR("Reserve for block detail data failed.");
        return ASSEMBLE_FAILED;
    }
    for (const auto& data : *blockDetailData)
    {
        blockDetailDataMap[data.taskId].emplace_back(std::move(data));
    }
    if (taskInfoData == nullptr)
    {
        WARN("No ge data collected, maybe the TaskInfo table is not created, now try to export data with no ge data.");
    }
    else
    {
        FormatTaskInfoData(*taskInfoData);
    }
    auto layerInfo = GetLayerInfo(PROCESS_BLOCK_DETAIL);
    auto pidMap = GenerateBlockDetailTrace(*ascendTaskData, blockDetailDataMap, layerInfo.sortIndex, profPath, res_);
    if (res_.empty())
    {
        ERROR("Can't Generate any block detail process data.");
        return ASSEMBLE_FAILED;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto& node : res_)
    {
        node->DumpJson(ostream);
    }
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}

std::unordered_map<uint16_t, uint32_t> BlockDetailAssembler::GenerateBlockDetailTrace(
    std::vector<AscendTaskData>& ascendTaskData,
    std::unordered_map<uint32_t, std::vector<BlockDetailData>>& blockDetailDataMap, uint32_t sortIndex,
    const std::string& profPath, std::vector<std::shared_ptr<TraceEvent>>& res)
{
    std::unordered_map<uint16_t, uint32_t> pidMap;
    for (const auto& taskData : ascendTaskData)
    {
        auto blockDetailIt = blockDetailDataMap.find(taskData.taskId);
        if (blockDetailIt == blockDetailDataMap.end())
        {
            continue;
        }
        const auto& pmuDataList = blockDetailIt->second;
        auto pmuGroup = GetBlockPmuGroup(pmuDataList, taskData);
        if (pmuGroup.empty())
        {
            continue;
        }
        std::string opName;
        TaskId taskId{taskData.streamId, taskData.batchId, taskData.taskId, taskData.contextId, taskData.deviceId};
        auto taskDataIt = formatedTaskInfo_.find(taskId);
        if (taskDataIt != formatedTaskInfo_.end())
        {
            opName = taskDataIt->second.first;
        }
        else
        {
            WARN("Failed to find opName for deviceId[%u], taskId[%u]", taskData.deviceId, taskData.taskId);
        }
        auto pid = GetDevicePid(pidMap, taskData.deviceId, profPath, sortIndex);
        std::vector<PmuTimelineRow> block_pmu_timeline_data;
        SetPmuResultList(pmuGroup, pid, block_pmu_timeline_data, opName, res);
        GenerateBlockPmuTimelineData(block_pmu_timeline_data, res);
    }
    return pidMap;
}

std::vector<const BlockDetailData*> BlockDetailAssembler::GetBlockPmuGroup(
    const std::vector<BlockDetailData>& pmuDataList, const AscendTaskData& taskData)
{
    std::vector<const BlockDetailData*> result;
    if (!Reserve(result, (pmuDataList.size() / THREAD_ARGS_NAMES_SIZE)))
    {
        ERROR("Reserve for pmu data list failed.");
        return result;
    }
    uint64_t taskStart = taskData.timestamp;
    uint64_t taskEnd = taskData.timestamp + taskData.duration;
    for (const auto& pmuData : pmuDataList)
    {
        uint64_t pmuStart = pmuData.timestamp;
        uint64_t pmuEnd = pmuData.timestamp + pmuData.duration;
        // 判断是否有交集：!(taskEnd < pmuStart || pmuEnd < taskStart)
        if (!(taskEnd < pmuStart || pmuEnd < taskStart))
        {
            result.push_back(&pmuData);
        }
    }
    return result;
}

void BlockDetailAssembler::FormatTaskInfoData(const std::vector<TaskInfoData>& taskInfoData)
{
    if (taskInfoData.empty())
    {
        WARN("task info data is empty, no task info data for task time, check ge info please.");
        return;
    }
    for (const auto& taskInfoDatum : taskInfoData)
    {
        TaskId taskId{taskInfoDatum.streamId, taskInfoDatum.batchId, taskInfoDatum.taskId, taskInfoDatum.contextId,
                      taskInfoDatum.deviceId};
        formatedTaskInfo_.insert({taskId, {taskInfoDatum.opName, taskInfoDatum.taskType}});
    }
}

void BlockDetailAssembler::SetPmuResultList(const std::vector<const BlockDetailData*>& datas, uint32_t pid,
                                            std::vector<PmuTimelineRow>& block_pmu_timeline_data,
                                            const std::string& opName, std::vector<std::shared_ptr<TraceEvent>>& res)
{
    // 分离并排序 aic (core_type == 0) 和 aiv (core_type == 1)
    std::vector<const BlockDetailData*> aic_datas;
    std::vector<const BlockDetailData*> aiv_datas;
    for (const auto* data : datas)
    {
        if (!data) continue;
        if (data->coreType == 0)
        {
            aic_datas.push_back(data);
        }
        else if (data->coreType == 1)
        {
            aiv_datas.push_back(data);
        }
    }
    // 按 timestamp 排序
    auto sortByTimeStamp = [](const BlockDetailData* a, const BlockDetailData* b)
    { return a->timestamp < b->timestamp; };
    std::sort(aic_datas.begin(), aic_datas.end(), sortByTimeStamp);
    std::sort(aiv_datas.begin(), aiv_datas.end(), sortByTimeStamp);
    // 获取最早和最晚的数据
    const BlockDetailData* aic_earliest = aic_datas.empty() ? nullptr : aic_datas.front();
    const BlockDetailData* aic_latest = aic_datas.empty() ? nullptr : aic_datas.back();
    const BlockDetailData* aiv_earliest = aiv_datas.empty() ? nullptr : aiv_datas.front();
    const BlockDetailData* aiv_latest = aiv_datas.empty() ? nullptr : aiv_datas.back();
    // 处理四个数据
    const BlockDetailData* data_list[THREAD_ARGS_NAMES_SIZE] = {aic_earliest, aic_latest, aiv_earliest, aiv_latest};
    for (size_t i = 0; i < THREAD_ARGS_NAMES_SIZE; ++i)
    {
        const auto* data = data_list[i];
        if (!data) continue;
        PmuTimelineRow row;
        row.name = "Stream " + std::to_string(data->streamId) + " " + opName;
        row.pid = pid;
        row.tid = i + 1;
        row.timestamp = data->timestamp;
        row.duration = data->duration;
        row.streamId = data->streamId;
        row.taskId = data->taskId;
        row.batchId = data->batchId;
        row.subtaskId = data->subtaskId;
        row.coreId = data->coreId;
        std::shared_ptr<MetaDataNameEvent> threadNameEvent;
        MAKE_SHARED_RETURN_VOID(threadNameEvent, MetaDataNameEvent, pid, row.tid, META_DATA_THREAD_NAME,
                                THREAD_ARGS_NAMES[i]);
        res.push_back(std::shared_ptr<TraceEvent>(std::move(threadNameEvent)));
        block_pmu_timeline_data.push_back(std::move(row));
    }
}

void BlockDetailAssembler::GenerateBlockPmuTimelineData(const std::vector<PmuTimelineRow>& block_pmu_timeline_data,
                                                        std::vector<std::shared_ptr<TraceEvent>>& res)
{
    for (const auto& data : block_pmu_timeline_data)
    {
        std::shared_ptr<BlockDetailEvent> event;
        MAKE_SHARED_RETURN_VOID(event, BlockDetailEvent, data.pid, data.tid, data.duration,
                                DivideByPowersOfTenWithPrecision(data.timestamp), data.name, data.streamId, data.taskId,
                                data.batchId, data.subtaskId, data.coreId);
        res.push_back(event);
    }
}
}  // namespace Application
}  // namespace Analysis
