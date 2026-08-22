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

#include "analysis/csrc/domain/services/device_context/load_host_data.h"

#include <string>

#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

using namespace Analysis;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Utils;

namespace Analysis
{
namespace Domain
{

const std::string HOST_TASK_TABLE = "HostTask";
const std::string TASK_INFO_TABLE = "TaskInfo";
const std::string HCCL_OP_TABLE = "HCCLOP";
const std::string HCCL_TASK_TABLE = "HCCLTask";
const std::string GE_HASH_INFO_TABLE = "GeHashInfo";
using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
using RuntimeOriDataFormat = std::vector<std::tuple<uint32_t, int32_t, uint32_t, uint32_t, std::string, uint64_t,
                                                    std::string, std::string, int64_t, uint16_t>>;
using HcclTaskOriDataFormat =
    std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t, uint64_t, int64_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint16_t, uint16_t, int64_t, int64_t, uint32_t, std::string, double,
                           std::string, std::string, std::string, std::string, int64_t>>;
using GeHashFormat = std::vector<std::tuple<std::string, std::string>>;
using HcclOpOriDataFormat =
    std::vector<std::tuple<uint16_t, uint64_t, int32_t, uint32_t, std::string, std::string, std::string, int64_t,
                           int32_t, int32_t, std::string, std::string, uint64_t, std::string>>;
bool CheckPathAndTableExists(const std::string& path, DBRunner& dbRunner, const std::string& tableName)
{
    if (!File::Exist(path))
    {
        WARN("There is no %", path);
        return false;
    }
    if (!dbRunner.CheckTableExists(tableName))
    {
        WARN("There is no %", tableName);
        return false;
    }
    return true;
}

uint32_t ReadHostGEInfo(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    GEInfoDB geInfoDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "..", HOST, SQLITE, geInfoDb.GetDBName()});
    DBRunner hostGeInfoDBRunner(hostDbDirectory);
    if (!CheckPathAndTableExists(hostDbDirectory, hostGeInfoDBRunner, TASK_INFO_TABLE))
    {
        return ANALYSIS_OK;
    }
    std::string sql{
        "SELECT stream_id, batch_id, task_id, context_id, block_num, "
        "mix_block_num FROM TaskInfo where device_id = " +
        std::to_string(deviceInfo.deviceId)};
    OriDataFormat result;
    bool rc = hostGeInfoDBRunner.QueryData(sql, result);
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", geInfoDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    int noExistCNt = 0;
    auto deviceTaskMap = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!deviceTaskMap)
    {
        return ANALYSIS_ERROR;
    }
    for (auto row : result)
    {
        uint32_t stream_id, batch_id, task_id, context_id, blockNum, mixBlockNum;
        std::tie(stream_id, batch_id, task_id, context_id, blockNum, mixBlockNum) = row;
        TaskId id = {stream_id, batch_id, task_id, context_id};
        auto item = deviceTaskMap->find(id);
        if (item != deviceTaskMap->end())
        {
            std::vector<DeviceTask>& deviceTasks = item->second;
            for (auto& deviceTask : deviceTasks)
            {
                deviceTask.blockNum = (uint16_t)blockNum;
                deviceTask.mixBlockNum = (uint16_t)mixBlockNum;
            }
        }
        else
        {
            noExistCNt++;
        }
    }
    INFO("Read host GE info % not in table.", noExistCNt);
    return ANALYSIS_OK;
}

bool LoadHostData::ReadHostRuntimeFromDB(std::string& profPath, TaskId2HostTask& hostRuntime,
                                         const std::vector<std::string>& deviceIds)
{
    RuntimeDB runtimeDb;
    std::string hostDbDirectory = File::PathJoin({profPath, HOST, SQLITE, runtimeDb.GetDBName()});
    DBRunner hostRuntimeDBRunner(hostDbDirectory);

    if (!CheckPathAndTableExists(hostDbDirectory, hostRuntimeDBRunner, HOST_TASK_TABLE))
    {
        return true;  // 如果路径和表不存在，返回true表示没有错误，但hostRuntime为空
    }

    std::string sql{
        "SELECT stream_id, request_id, batch_id, task_id, context_ids, "
        "model_id, task_type, kernel_name, connection_id, device_id FROM HostTask"};
    if (!deviceIds.empty())
    {
        sql += " WHERE device_id IN (" + Join(deviceIds, ",") + ")";
    }
    RuntimeOriDataFormat result(0);
    bool rc = hostRuntimeDBRunner.QueryData(sql, result);
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", runtimeDb.GetDBName());
        return false;
    }

    for (const auto& row : result)
    {
        uint32_t context_id_u32;
        std::string task_type, kernel_name, context_id;
        HostTask hostTask;
        std::tie(hostTask.streamId, hostTask.requestId, hostTask.batchId, hostTask.taskId, context_id, hostTask.modelId,
                 hostTask.taskTypeStr, hostTask.kernelNameStr, hostTask.connectionId, hostTask.deviceId) = row;
        StrToU32(context_id_u32, context_id);
        TaskId id = {hostTask.streamId, hostTask.batchId, hostTask.taskId, context_id_u32};
        hostTask.contextId = context_id_u32;
        hostRuntime[id].push_back(hostTask);
    }
    return true;
}

uint32_t ReadHostRuntime(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    TaskId2HostTask hostRuntime;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    std::string profPath = File::PathJoin({deviceContext.GetDeviceFilePath(), ".."});
    if (!LoadHostData::ReadHostRuntimeFromDB(profPath, hostRuntime, {std::to_string(deviceInfo.deviceId)}))
    {
        return ANALYSIS_ERROR;
    }
    std::shared_ptr<HostStreamInfo> streamIdInfo;
    MAKE_SHARED_RETURN_VALUE(streamIdInfo, HostStreamInfo, ANALYSIS_ERROR);
    if (deviceContext.GetChipID() == CHIP_V6_1_0 || deviceContext.GetChipID() == CHIP_V6_2_0)
    {
        for (auto& task : hostRuntime)
        {
            streamIdInfo->streamIdMap.emplace(task.first.taskId, task.first.streamId);
        }
    }
    dataInventory.Inject(streamIdInfo);
    std::shared_ptr<TaskId2HostTask> data;
    MAKE_SHARED_RETURN_VALUE(data, TaskId2HostTask, ANALYSIS_ERROR, std::move(hostRuntime));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t ReadHcclOp(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    HCCLDB hcclDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "..", HOST, SQLITE, hcclDb.GetDBName()});
    DBRunner hostHcclDBRunner(hostDbDirectory);
    std::string sql{
        "SELECT device_id, model_id, index_id, thread_id, op_name, task_type, op_type, "
        "connection_id, relay, retry, data_type, alg_type, count, group_name from HCCLOP where device_id = "};
    sql += std::to_string(deviceInfo.deviceId);
    std::vector<HcclOp> ans;
    std::shared_ptr<std::vector<HcclOp>> data;
    if (!CheckPathAndTableExists(hostDbDirectory, hostHcclDBRunner, HCCL_OP_TABLE))
    {
        MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclOp>, ANALYSIS_ERROR, std::move(ans));
        dataInventory.Inject(data);
        return ANALYSIS_OK;
    }
    HcclOpOriDataFormat result(0);
    bool rc = hostHcclDBRunner.QueryData(sql, result);
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", hcclDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    for (auto row : result)
    {
        uint16_t deviceId;
        uint64_t modelId, count;
        int32_t indexId, relay, retry;
        uint32_t threadId;
        std::string opName, taskType, opType, dataType, algType, groupName;
        int64_t connectionId;
        std::tie(deviceId, modelId, indexId, threadId, opName, taskType, opType, connectionId, relay, retry, dataType,
                 algType, count, groupName) = row;
        HcclOp hcclOp{deviceId,     modelId, indexId, threadId, opName,  taskType, opType,
                      connectionId, relay,   retry,   dataType, algType, count,    groupName};
        ans.emplace_back(std::move(hcclOp));
    }
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclOp>, ANALYSIS_ERROR, std::move(ans));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t ReadHcclTask(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    HCCLDB hcclDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "..", HOST, SQLITE, hcclDb.GetDBName()});
    DBRunner hostHcclDBRunner(hostDbDirectory);
    std::string sql{
        "SELECT model_id, index_id, name, group_name, plane_id, timestamp, op_id, stream_id, task_id, "
        "context_id, batch_id, device_id, is_master, local_rank, remote_rank, thread_id, transport_type, "
        "size, data_type, link_type, notify_id, rdma_type, rank_size from HCCLTask where device_id = "};
    sql += std::to_string(deviceInfo.deviceId);
    std::vector<HcclTask> ans;
    std::shared_ptr<std::vector<HcclTask>> data;
    if (!CheckPathAndTableExists(hostDbDirectory, hostHcclDBRunner, HCCL_TASK_TABLE))
    {
        MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclTask>, ANALYSIS_ERROR, std::move(ans));
        dataInventory.Inject(data);
        return ANALYSIS_OK;
    }
    HcclTaskOriDataFormat result(0);
    bool rc = hostHcclDBRunner.QueryData(sql, result);
    if (!rc)
    {
        ERROR("Failed to obtain data from the % table.", hcclDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    for (const auto& row : result)
    {
        uint64_t modelId, timestamp;
        int32_t indexId, planeId;
        std::string name, groupName, transportType, dataType, linkType, rdmaType, notifyId;
        double size;
        uint32_t streamId, contextId, threadId, taskId, batchId;
        int64_t localRank, remoteRank, rankSize;
        uint16_t deviceId, isMaster;
        int64_t opId;
        std::tie(modelId, indexId, name, groupName, planeId, timestamp, opId, streamId, taskId, contextId, batchId,
                 deviceId, isMaster, localRank, remoteRank, threadId, transportType, size, dataType, linkType, notifyId,
                 rdmaType, rankSize) = row;
        HcclTask hcclTask{modelId,   indexId,  name,     groupName, planeId,   timestamp,  streamId, taskId,
                          contextId, batchId,  deviceId, isMaster,  localRank, remoteRank, threadId, transportType,
                          size,      dataType, linkType, notifyId,  rdmaType,  rankSize,   opId};
        ans.emplace_back(std::move(hcclTask));
    }
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HcclTask>, ANALYSIS_ERROR, std::move(ans));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

uint32_t ReadHash(DataInventory& dataInventory, const DeviceContext& deviceContext)
{
    HashDB hashDb;
    DeviceInfo deviceInfo{};
    deviceContext.Getter(deviceInfo);
    auto hostPath = deviceContext.GetDeviceFilePath();
    std::string hostDbDirectory = Utils::File::PathJoin({hostPath, "..", HOST, SQLITE, hashDb.GetDBName()});
    DBRunner hostHashDBRunner(hostDbDirectory);
    std::string sql{"SELECT hash_key, hash_value FROM " + GE_HASH_INFO_TABLE};
    GeHashMap hashMap;
    std::shared_ptr<GeHashMap> res;
    if (!CheckPathAndTableExists(hostDbDirectory, hostHashDBRunner, GE_HASH_INFO_TABLE))
    {
        MAKE_SHARED_RETURN_VALUE(res, GeHashMap, ANALYSIS_ERROR, std::move(hashMap));
        dataInventory.Inject(res);
        return ANALYSIS_OK;
    }
    GeHashFormat hashData;
    if (!hostHashDBRunner.QueryData(sql, hashData))
    {
        ERROR("Failed to obtain data from the % table.", hashDb.GetDBName());
        return ANALYSIS_ERROR;
    }
    for (auto& item : hashData)
    {
        hashMap[std::get<0>(item)] = std::get<1>(item);
    }
    MAKE_SHARED_RETURN_VALUE(res, GeHashMap, ANALYSIS_ERROR, std::move(hashMap));
    dataInventory.Inject(res);
    return ANALYSIS_OK;
}

uint32_t LoadHostData::ProcessEntry(DataInventory& dataInventory, const Infra::Context& context)
{
    auto devTaskSummary = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!devTaskSummary)
    {
        ERROR("LoadHostData get devTaskSummary fail.");
        return ANALYSIS_ERROR;
    }
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    return ReadHostRuntime(dataInventory, deviceContext) | ReadHostGEInfo(dataInventory, deviceContext) |
           ReadHcclTask(dataInventory, deviceContext) | ReadHcclOp(dataInventory, deviceContext) |
           ReadHash(dataInventory, deviceContext);
}

REGISTER_PROCESS_SEQUENCE(LoadHostData, false, Analysis::Domain::LogModeling, Analysis::Domain::LogModelingV6);
REGISTER_PROCESS_DEPENDENT_DATA(LoadHostData, std::map<TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(LoadHostData, CHIP_ID_ALL);

}  // namespace Domain
}  // namespace Analysis
