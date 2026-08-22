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
#include "communication_info_processor.h"

#include <limits>

#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis
{
namespace Domain
{
using namespace Environment;
using namespace Analysis::Utils;
namespace
{
// groupName 依据hash进行转换，对于无hash的数据，直接取用hash值（即groupName）进行转换
std::string GetGroupNameValue(const std::string& groupName, GeHashMap& hashMap)
{
    if (groupName != NA && Utils::IsNumber(groupName))
    {
        if (hashMap.find(groupName) != hashMap.end())
        {
            return hashMap[groupName];
        }
    }
    return groupName;
}
}  // namespace

CommunicationInfoProcessor::CommunicationInfoProcessor(const std::string& profPaths) : DataProcessor(profPaths) {}

bool CommunicationInfoProcessor::Process(DataInventory& dataInventory)
{
    CommunicationData communicationData;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    bool flag = true;
    auto hashMap = dataInventory.GetPtr<GeHashMap>();
    if (hashMap == nullptr)
    {
        ERROR("Can't get hash data.");
        return false;
    }
    communicationData.hashMap = *hashMap;
    for (const auto& devicePath : deviceList)
    {
        communicationData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(communicationData.timeRecord, profPath_,
                                                          communicationData.deviceId))
        {
            ERROR(
                "Failed to obtain the time in start_info and end_info. "
                "Path is %, device id is %.",
                profPath_, communicationData.deviceId);
            flag = false;
            continue;
        }
        flag = ProcessOneDevice(devicePath, communicationData) && flag;
    }
    if (!SaveToDataInventory<CommunicationTaskData>(std::move(communicationData.resTaskData), dataInventory,
                                                    TABLE_NAME_COMMUNICATION_TASK_INFO))
    {
        ERROR("Save data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    if (!SaveToDataInventory<CommunicationOpData>(std::move(communicationData.resOpData), dataInventory,
                                                  TABLE_NAME_COMMUNICATION_OP))
    {
        ERROR("Save data failed, %.", TABLE_NAME_COMMUNICATION_OP);
        return false;
    }
    return flag;
}

OriTaskDataFormat CommunicationInfoProcessor::LoadTaskData(const DBInfo& taskSingleDevice)
{
    OriTaskDataFormat oriTaskData;
    std::string sql{
        "SELECT model_id, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
        "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
        "rdma_type, timestamp, duration, op_id, bandwidth, is_master, iter_id "
        "FROM " +
        taskSingleDevice.tableName};
    if (!taskSingleDevice.dbRunner->QueryData(sql, oriTaskData))
    {
        ERROR("Failed to obtain data from the % table.", taskSingleDevice.tableName);
    }
    return oriTaskData;
}

OriOpDataFormat CommunicationInfoProcessor::LoadOpData(const DBInfo& opSingleDevice)
{
    OriOpDataFormat oriOpData;
    std::string sql{
        "SELECT connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, "
        "model_id, rank_size, start, end, iter_id FROM " +
        opSingleDevice.tableName};
    if (!opSingleDevice.dbRunner->QueryData(sql, oriOpData))
    {
        ERROR("Failed to obtain data from the % table.", opSingleDevice.tableName);
    }
    return oriOpData;
}

bool CommunicationInfoProcessor::FormatData(const OriTaskDataFormat& oriTaskData, const OriOpDataFormat& oriOpData,
                                            std::vector<CommunicationTaskData>& taskFormatData,
                                            std::vector<CommunicationOpData>& opFormatData,
                                            CommunicationData& communicationData, HcclType type)
{
    if (!Utils::Reserve(taskFormatData, oriTaskData.size()))
    {
        ERROR("Reserve for communication task data failed.");
        return false;
    }

    for (auto& row : oriTaskData)
    {
        taskFormatData.push_back(UpdateTaskInfo(row, communicationData, type));
    }

    if (!Utils::Reserve(opFormatData, oriOpData.size()))
    {
        ERROR("Reserve for communication op data failed.");
        return false;
    }

    for (auto& row : oriOpData)
    {
        opFormatData.push_back(UpdateOpInfo(row, communicationData, type));
    }
    return true;
}

CommunicationTaskData CommunicationInfoProcessor::UpdateTaskInfo(const HcclTaskFormat& oriData,
                                                                 CommunicationData& communicationData, HcclType type)
{
    CommunicationTaskData taskData;
    double timestamp, duration;
    std::string groupName;
    std::tie(taskData.modelId, taskData.hcclName, groupName, taskData.planeId, taskData.streamId, taskData.taskId,
             taskData.srcRank, taskData.dstRank, taskData.transportType, taskData.size, taskData.dataType,
             taskData.linkType, taskData.contextId, taskData.notifyId, taskData.batchId, taskData.rdmaType, timestamp,
             duration, taskData.opId, taskData.bandwidth, taskData.isMaster, taskData.iterId) = oriData;
    HPFloat timestampFp{timestamp};
    HPFloat durationFp{duration};
    taskData.timestamp = GetLocalTime(timestampFp, communicationData.timeRecord).Uint64();
    taskData.duration = durationFp.Uint64();
    taskData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
    taskData.taskType = taskData.hcclName;
    taskData.source = type;
    taskData.deviceId = communicationData.deviceId;
    return taskData;
}

CommunicationOpData CommunicationInfoProcessor::UpdateOpInfo(const HcclOpFormat& oriData,
                                                             CommunicationData& communicationData, HcclType type)
{
    CommunicationOpData opData;
    double start, end;
    std::string groupName;
    std::tie(opData.connectionId, opData.opName, opData.relay, opData.retry, opData.dataType, opData.algType,
             opData.count, groupName, opData.opType, opData.modelId, opData.rankSize, start, end, opData.iterId) =
        oriData;
    HPFloat startFp{start};
    HPFloat endFp{end};
    opData.timestamp = Utils::GetLocalTime(startFp, communicationData.timeRecord).Uint64();
    opData.end = Utils::GetLocalTime(endFp, communicationData.timeRecord).Uint64();
    opData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
    opData.source = type;
    opData.deviceId = communicationData.deviceId;
    return opData;
}

bool CommunicationInfoProcessor::ProcessKfcData(const std::string& devicePath,
                                                std::vector<CommunicationTaskData>& taskData,
                                                std::vector<CommunicationOpData>& opData,
                                                CommunicationData& communicationData)
{
    DBInfo kfcTaskDBInfo("hccl_single_device.db", "KfcTask");
    DBInfo kfcOpDBInfo("hccl_single_device.db", "KfcOP");
    std::string kfcTaskDBPath = Utils::File::PathJoin({devicePath, SQLITE, kfcTaskDBInfo.dbName});
    std::string kfcOpDBPath = Utils::File::PathJoin({devicePath, SQLITE, kfcOpDBInfo.dbName});
    if (!kfcTaskDBInfo.ConstructDBRunner(kfcTaskDBPath) || !kfcOpDBInfo.ConstructDBRunner(kfcOpDBPath))
    {
        ERROR("Construct KfcTask table failed.");
        return false;
    }
    auto status = CheckPathAndTable(kfcTaskDBPath, kfcTaskDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    status = CheckPathAndTable(kfcOpDBPath, kfcOpDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    communicationData.oriKfcTaskData = LoadTaskData(kfcTaskDBInfo);
    communicationData.oriKfcOpData = LoadOpData(kfcOpDBInfo);
    if (!FormatData(communicationData.oriKfcTaskData, communicationData.oriKfcOpData, taskData, opData,
                    communicationData, HcclType::MC2))
    {
        ERROR("Format kfc task data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    return true;
}

bool CommunicationInfoProcessor::ProcessHcclData(const std::string& devicePath,
                                                 std::vector<CommunicationTaskData>& taskData,
                                                 std::vector<CommunicationOpData>& opData,
                                                 CommunicationData& communicationData)
{
    DBInfo taskDBInfo("hccl_single_device.db", "HCCLTaskSingleDevice");
    DBInfo opDBInfo("hccl_single_device.db", "HCCLOpSingleDevice");
    std::string taskDBPath = Utils::File::PathJoin({devicePath, SQLITE, taskDBInfo.dbName});
    std::string opDBPath = Utils::File::PathJoin({devicePath, SQLITE, opDBInfo.dbName});
    if (!taskDBInfo.ConstructDBRunner(taskDBPath) || !opDBInfo.ConstructDBRunner(opDBPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(taskDBPath, taskDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    status = CheckPathAndTable(opDBPath, opDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        return status != CHECK_FAILED;
    }
    communicationData.oriTaskData = LoadTaskData(taskDBInfo);
    communicationData.oriOpData = LoadOpData(opDBInfo);
    if (!FormatData(communicationData.oriTaskData, communicationData.oriOpData, taskData, opData, communicationData,
                    HcclType::HCCL))
    {
        ERROR("Format data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    return true;
}

bool CommunicationInfoProcessor::ProcessOneDevice(const std::string& devicePath, CommunicationData& communicationData)
{
    bool flag = true;
    std::vector<CommunicationTaskData> taskData;
    std::vector<CommunicationOpData> opData;
    if (!ProcessHcclData(devicePath, taskData, opData, communicationData))
    {
        ERROR("Process hccl data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        flag = false;
    }
    if (!ProcessKfcData(devicePath, taskData, opData, communicationData))
    {
        ERROR("Process kfc data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        flag = false;
    }
    FilterDataByStartTime(taskData, communicationData.timeRecord.startTimeNs, TABLE_NAME_COMMUNICATION_TASK_INFO);
    communicationData.resTaskData.insert(communicationData.resTaskData.end(), taskData.begin(), taskData.end());

    FilterDataByStartTime(opData, communicationData.timeRecord.startTimeNs, TABLE_NAME_COMMUNICATION_TASK_INFO);
    communicationData.resOpData.insert(communicationData.resOpData.end(), opData.begin(), opData.end());
    return flag;
}
}  // namespace Domain
}  // namespace Analysis
