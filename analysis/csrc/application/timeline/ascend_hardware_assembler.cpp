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

#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"

#include "analysis/csrc/application//credential/id_pool.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Common;
namespace
{
using MEMCPY_INFO_FORMAT = std::map<TaskId, MemcpyInfoData>;
const std::vector<std::string> MEMCPY_OPERATIONS{"host to host",      "host to device",   "device to host",
                                                 "device to device",  "managed memory",   "addr device to device",
                                                 "host to device ex", "device to host ex"};
}  // namespace

MEMCPY_INFO_FORMAT GenerateMemcpyInfoDataMap(const std::shared_ptr<std::vector<MemcpyInfoData>> &res)
{
    MEMCPY_INFO_FORMAT memcpyInfoDataMap;
    if (res != nullptr)
    {
        for (const auto &item : *res)
        {
            memcpyInfoDataMap[item.taskId] = std::move(item);
        }
    }
    return memcpyInfoDataMap;
}

AscendHardwareAssembler::AscendHardwareAssembler()
    : JsonAssembler(PROCESS_TASK, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{
}

void TaskTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Model Id"] << modelId_;
    ostream["Task Type"] << taskType_;
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
    ostream["Batch Id"] << batchId_;
    ostream["Subtask Id"] << contextId_;
    ostream["connection_id"] << connectionId_;
}

void MemcpyAsyncEvent::ProcessArgs(JsonWriter &ostream)
{
    TaskTraceEvent::ProcessArgs(ostream);
    if (showFlag_)
    {
        ostream["size(B)"] << dataSize_;
        ostream["bandwidth(GB/s)"] << bandwidth_;
        ostream["operation"] << memcpyDirection_;
    }
}

void SimtTaskEvent::ProcessArgs(JsonWriter &ostream)
{
    TaskTraceEvent::ProcessArgs(ostream);
    ostream["Grid Dim"] << gridDim_;
    ostream["Block Dim"] << blockDim_;
}

void KfcTurnTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
}

void AscendHardwareAssembler::InitData(DataInventory &dataInventory, std::vector<AscendTaskData> &taskData)
{
    logicStream_ = dataInventory.GetPtr<std::unordered_map<uint32_t, uint32_t>>();
    auto taskInfo = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    if (taskInfo != nullptr)
    {
        for (const auto &node : *taskInfo)
        {
            const TaskId &taskId = TaskId{static_cast<uint16_t>(node.streamId), static_cast<uint16_t>(node.batchId),
                                          node.taskId, node.contextId, node.deviceId};
            opName_.emplace(taskId, node.opName);
            taskType_.emplace(taskId, node.taskType);
            simtInfoMap_.emplace(taskId, std::make_pair(node.gridDim, node.blockDim));
        }
    }
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData != nullptr)
    {
        for (const auto &node : *apiData)
        {
            if (RECORD_EVENT == node.id || WAIT_EVENT == node.id)
            {
                aclEvent_.emplace(node.connectionId);
            }
        }
    }
    for (const auto &data : taskData)
    {
        if (data.contextId != UINT32_MAX)
        {
            ffts_.emplace(TaskId{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId), data.taskId,
                                 UINT32_MAX, data.deviceId});
        }
        if (data.hostType == MEMCPY_ASYNC)
        {
            memcpyAsyncDeviceTasks_.push_back(data);
        }
    }
}

std::string AscendHardwareAssembler::GetOpName(const AscendTaskData &data)
{
    TaskId id{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId), data.taskId, data.contextId,
              data.deviceId};
    auto it = opName_.find(id);
    if (it != opName_.end())
    {
        return it->second;
    }
    if (data.hostType == TASK_TYPE_FFTS_PLUS || data.hostType == UNKNOWN)
    {
        return data.deviceType;
    }
    return data.hostType;
}

std::string AscendHardwareAssembler::GetTaskType(const AscendTaskData &data)
{
    TaskId id{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId), data.taskId, data.contextId,
              data.deviceId};
    auto it = taskType_.find(id);
    if (it != taskType_.end() && it->second != NA)
    {
        return it->second;
    }
    return data.taskType;
}

uint32_t AscendHardwareAssembler::GetPhysicStreamId(const uint32_t streamId)
{
    if (logicStream_ == nullptr)
    {
        return streamId;
    }
    auto it = logicStream_->find(streamId);
    if (it != logicStream_->end())
    {
        return it->second;
    }
    return streamId;
}

void AscendHardwareAssembler::GenerateTaskTrace(const std::vector<AscendTaskData> &taskData,
                                                const std::string &profPath, const LayerInfo &layer,
                                                std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    std::string taskTypeName;
    TaskId id;
    for (const auto &data : taskData)
    {
        if (data.hostType == MEMCPY_ASYNC)
        {
            continue;  // MEMCPY_ASYNC类型的task有新增args,需要单独处理
        }
        id = {static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId), data.taskId, data.contextId,
              data.deviceId};
        if (ffts_.find(id) != ffts_.end())
        {  // 当前task存在ffts+任务，只呈现ffts+任务即可
            continue;
        }
        traceName = GetOpName(data);
        taskTypeName = GetTaskType(data);
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        pidTidSet_.insert({formatPid, tid});
        if (data.taskType == Analysis::Common::KERNEL_SIMT_TASK_TYPE)
        {
            std::string gridDim = NA;
            std::string blockDim = NA;
            auto it = simtInfoMap_.find(id);
            if (it != simtInfoMap_.end())
            {
                gridDim = it->second.first;
                blockDim = it->second.second;
            }
            std::shared_ptr<SimtTaskEvent> event;
            MAKE_SHARED_RETURN_VOID(
                event, SimtTaskEvent, formatPid, tid, static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, data.modelId, data.streamId, data.taskId,
                data.batchId, data.contextId, data.connectionId, taskTypeName, gridDim, blockDim);
            res_.push_back(event);
        }
        else
        {
            std::shared_ptr<TaskTraceEvent> event;
            MAKE_SHARED_RETURN_VOID(
                event, TaskTraceEvent, formatPid, tid, static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                DivideByPowersOfTenWithPrecision(data.timestamp), traceName, data.modelId, data.streamId, data.taskId,
                data.batchId, data.contextId, data.connectionId, taskTypeName);
            res_.push_back(event);
        }
        GenerateTaskConnectionTrace(data, formatPid, id);
    }
}

void AscendHardwareAssembler::GenerateKfcTrace(const std::vector<KfcTurnData> &kfcData, const std::string &profPath,
                                               const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    for (const auto &datum : kfcData)
    {
        std::string traceName = datum.opName;
        formatPid = GetDevicePid(pidMap, datum.deviceId, profPath, layer.sortIndex);
        int formatTid = static_cast<int>(GetPhysicStreamId(datum.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, formatTid});
        std::shared_ptr<KfcTurnTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, KfcTurnTraceEvent, formatPid, formatTid,
                                static_cast<double>(datum.duration) / Analysis::Common::NS_TO_US,
                                DivideByPowersOfTenWithPrecision(datum.timestamp), traceName, datum.streamId,
                                datum.taskId);
        res_.push_back(event);
    }
}

void AscendHardwareAssembler::GenerateMemcpyAsyncTrace(DataInventory &dataInventory, const std::string &profPath,
                                                       const LayerInfo &layer,
                                                       std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    uint64_t dataSize;
    double bandwidth;
    std::string memcpyDirection;
    bool showFlag = true;
    auto memcpyInfo = dataInventory.GetPtr<std::vector<MemcpyInfoData>>();
    if (memcpyInfo == nullptr)
    {
        showFlag = false;
    }
    MEMCPY_INFO_FORMAT memcpyInfoDataMap = GenerateMemcpyInfoDataMap(memcpyInfo);
    for (const auto &data : memcpyAsyncDeviceTasks_)
    {
        dataSize = 0;
        memcpyDirection = OTHER_DIRECTION;
        bandwidth = 0.0;
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<MemcpyAsyncEvent> event;
        // 计算拷贝数据量和带宽
        if (showFlag)
        {
            TaskId taskId(data.streamId, data.batchId, data.taskId, data.contextId, data.deviceId);
            auto it = memcpyInfoDataMap.find(taskId);
            if (it != memcpyInfoDataMap.end())
            {
                dataSize = it->second.dataSize;
                memcpyDirection = it->second.memcpyOperation > VALID_MEMCPY_OPERATION
                                      ? OTHER_DIRECTION
                                      : MEMCPY_OPERATIONS[it->second.memcpyOperation];
            }
            else
            {
                ERROR("MEMCPY_ASYNC task lost memcpyInfo, connectionId is %", data.connectionId);
            }
            if (!IsDoubleEqual(data.duration, 0.0) && data.duration > 0)
            {
                bandwidth = static_cast<double>(dataSize) / data.duration;  // GB/s, 全部按照1000计算
            }
        }
        MAKE_SHARED_RETURN_VOID(event, MemcpyAsyncEvent, formatPid, tid,
                                static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.timestamp), data.hostType, data.modelId,
                                data.streamId, data.taskId, data.batchId, data.contextId, data.connectionId,
                                data.deviceType, dataSize, bandwidth, memcpyDirection, showFlag);
        res_.push_back(event);
        GenerateMemcpyAsyncConnectionTrace(data, formatPid);
    }
}

void AscendHardwareAssembler::GenerateTaskConnectionTrace(const AscendTaskData &data, uint32_t formatPid, TaskId &id)
{
    std::string connId;
    std::string name;
    int tid;
    if (opName_.find(id) != opName_.end() || aclEvent_.find(data.connectionId) != aclEvent_.end())
    {
        connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
        name = HOST_TO_DEVICE + connId;
        tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        std::shared_ptr<FlowEvent> end;
        MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, DivideByPowersOfTenWithPrecision(data.timestamp),
                                HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
        res_.push_back(end);
    }
}

void AscendHardwareAssembler::GenerateMemcpyAsyncConnectionTrace(const AscendTaskData &data, uint32_t formatPid)
{
    std::string connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
    std::string name = HOST_TO_DEVICE + connId;
    int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
    std::shared_ptr<FlowEvent> end;
    MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, DivideByPowersOfTenWithPrecision(data.timestamp),
                            HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
    res_.push_back(end);
}

uint8_t AscendHardwareAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream,
                                              const std::string &profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    auto kfcTurnData = dataInventory.GetPtr<std::vector<KfcTurnData>>();
    if (taskData == nullptr && kfcTurnData == nullptr)
    {
        WARN("Can't get task data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_TASK);
    if (taskData != nullptr)
    {
        InitData(dataInventory, *taskData);
        GenerateTaskTrace(*taskData, profPath, layer, devicePid);
        if (!memcpyAsyncDeviceTasks_.empty())
        {
            GenerateMemcpyAsyncTrace(dataInventory, profPath, layer, devicePid);
        }
    }
    if (kfcTurnData != nullptr)
    {
        GenerateKfcTrace(*kfcTurnData, profPath, layer, devicePid);
    }
    GenerateTaskMetaData(devicePid, layer, res_, pidTidSet_);
    if (res_.empty())
    {
        ERROR("Can't Generate any Ascend process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
