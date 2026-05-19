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
const size_t OPNAME_INDEX = 1;
const size_t RELAY_INDEX = 2;
const size_t RETRY_INDEX = 3;
struct CommunicationOpEndpointsTime
{
    double firstTaskStartTime = std::numeric_limits<double>::max();
    double lastTaskStartTime = 0.0;
    double lastTaskDuration = 0.0;
};

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

std::unordered_map<std::string, std::pair<int32_t, int32_t>> GetkfcOpInfoMap(const OriOpDataFormat& oriKfcOpData)
{
    std::unordered_map<std::string, std::pair<int32_t, int32_t>> kfcOpInfoMap;
    for (auto& row : oriKfcOpData)
    {
        kfcOpInfoMap.emplace(std::get<OPNAME_INDEX>(row),
                             std::make_pair(std::get<RELAY_INDEX>(row), std::get<RETRY_INDEX>(row)));
    }
    return kfcOpInfoMap;
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
    std::string sourceCondition = "";
    if (taskSingleDevice.tableName == "KfcTask")
    {
        sourceCondition = " WHERE source = " + std::to_string(static_cast<int>(DeviceHcclOpSource::HCCL));
    }
    std::string sql{
        "SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
        "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
        "rdma_type, timestamp, duration, connection_id, duration_estimated, bandwidth, is_master "
        "FROM " +
        taskSingleDevice.tableName + sourceCondition};
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
        "model_id, rank_size FROM " +
        opSingleDevice.tableName};
    if (opSingleDevice.tableName == "KfcOP")
    {
        sql.append(" WHERE source = " + std::to_string(static_cast<int>(DeviceHcclOpSource::HCCL)));
    }
    if (!opSingleDevice.dbRunner->QueryData(sql, oriOpData))
    {
        ERROR("Failed to obtain data from the % table.", opSingleDevice.tableName);
    }
    return oriOpData;
}

bool CommunicationInfoProcessor::FormatData(std::vector<CommunicationTaskData>& taskFormatData,
                                            std::vector<CommunicationOpData>& opFormatData,
                                            CommunicationData& communicationData)
{
    if (!Utils::Reserve(taskFormatData, communicationData.oriTaskData.size()))
    {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    CommunicationTaskData taskData;
    HcclTaskSingleDeviceData hcclData;
    std::unordered_map<std::string, CommunicationOpData> opDataMap;
    std::unordered_map<std::string, CommunicationOpEndpointsTime> endpoints;
    std::unordered_map<uint32_t, size_t> opInfoIdxMap = GenOpInfoIdxMap(communicationData.oriOpData);
    for (auto& row : communicationData.oriTaskData)
    {
        Update(row, hcclData, taskData, communicationData);
        taskFormatData.push_back(taskData);
        if (opDataMap.find(taskData.opKey) == opDataMap.end())
        {
            opDataMap[taskData.opKey].opKey = taskData.opKey;
            opDataMap[taskData.opKey].opName = taskData.opName;
            opDataMap[taskData.opKey].connectionId = hcclData.connectionId;
            opDataMap[taskData.opKey].groupName = taskData.groupName;
            opDataMap[taskData.opKey].deviceId = taskData.deviceId;
            UpdateOpInfo(opDataMap[taskData.opKey], hcclData.connectionId, opInfoIdxMap, communicationData.oriOpData,
                         communicationData);
        }

        if (taskData.isMaster)
        {
            endpoints[taskData.opKey].firstTaskStartTime =
                std::min(endpoints[taskData.opKey].firstTaskStartTime, hcclData.timestamp);
            if (hcclData.timestamp + hcclData.duration >
                endpoints[taskData.opKey].lastTaskStartTime + endpoints[taskData.opKey].lastTaskDuration)
            {
                endpoints[taskData.opKey].lastTaskStartTime = hcclData.timestamp;
                endpoints[taskData.opKey].lastTaskDuration = hcclData.duration;
            }
        }
    }
    for (auto& item : opDataMap)
    {
        auto key = item.first;
        auto data = item.second;
        if (endpoints.find(key) == endpoints.end())
        {
            ERROR("Can't match any master task in this op, key is %.", key);
            continue;
        }
        HPFloat start{endpoints[key].firstTaskStartTime};
        HPFloat end = HPFloat(endpoints[key].lastTaskStartTime) + HPFloat(endpoints[key].lastTaskDuration);
        data.timestamp = Utils::GetLocalTime(start, communicationData.timeRecord).Uint64();
        data.end = Utils::GetLocalTime(end, communicationData.timeRecord).Uint64();
        opFormatData.push_back(data);
    }
    return true;
}

bool CommunicationInfoProcessor::FormatKfcData(std::vector<CommunicationTaskData>& taskFormatData,
                                               std::vector<CommunicationOpData>& opFormatData,
                                               CommunicationData& communicationData)
{
    if (!Utils::Reserve(taskFormatData, communicationData.oriKfcTaskData.size()))
    {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    CommunicationTaskData taskData;
    HcclTaskSingleDeviceData hcclData;
    for (auto& row : communicationData.oriKfcTaskData)
    {
        Update(row, hcclData, taskData, communicationData);
        taskFormatData.emplace_back(taskData);
    }
    // 替换通信大算子的retry字段为KfcOp表里面的对应值
    auto kfcOpInfoMap = GetkfcOpInfoMap(communicationData.oriKfcOpData);
    for (auto& row : opFormatData)
    {
        auto it = kfcOpInfoMap.find(row.opName);
        if (it != kfcOpInfoMap.end())
        {
            row.relay = it->second.first;
            row.retry = it->second.second;
        }
        else
        {
            WARN("% is not exist in KfcOP", row.opName);
        }
    }
    return true;
}

void CommunicationInfoProcessor::Update(const HcclTaskFormat& oriData, HcclTaskSingleDeviceData& hcclData,
                                        CommunicationTaskData& taskData, CommunicationData& communicationData)
{
    std::tie(taskData.modelId, hcclData.opName, hcclData.HCCLName, hcclData.groupName, taskData.planeId,
             taskData.streamId, taskData.taskId, taskData.srcRank, taskData.dstRank, hcclData.transportType,
             taskData.size, hcclData.dataType, hcclData.linkType, taskData.contextId, taskData.notifyId,
             taskData.batchId, hcclData.rdmaType, hcclData.timestamp, hcclData.duration, hcclData.connectionId,
             taskData.durationEstimated, taskData.bandwidth, taskData.isMaster) = oriData;
    taskData.opName = hcclData.opName;
    taskData.deviceId = communicationData.deviceId;
    taskData.taskType = hcclData.HCCLName;
    taskData.duration = hcclData.duration;
    HPFloat timestamp{hcclData.timestamp};
    taskData.timestamp = GetLocalTime(timestamp, communicationData.timeRecord).Uint64();
    taskData.groupName = GetGroupNameValue(hcclData.groupName, communicationData.hashMap);
    taskData.rdmaType = GetEnumTypeValue(hcclData.rdmaType, NAME_STR(HCCL_RDMA_TYPE_TABLE), HCCL_RDMA_TYPE_TABLE);
    taskData.transportType =
        GetEnumTypeValue(hcclData.transportType, NAME_STR(HCCL_TRANSPORT_TYPE_TABLE), HCCL_TRANSPORT_TYPE_TABLE);
    taskData.dataType = GetEnumTypeValue(hcclData.dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
    taskData.linkType = GetEnumTypeValue(hcclData.linkType, NAME_STR(HCCL_LINK_TYPE_TABLE), HCCL_LINK_TYPE_TABLE);
    taskData.opKey = Utils::Join("_", hcclData.opName, hcclData.groupName, communicationData.deviceId);
}

void CommunicationInfoProcessor::UpdateOpInfo(CommunicationOpData& opData, uint32_t connectionId,
                                              const std::unordered_map<uint32_t, size_t>& opInfoIdxMap,
                                              const OriOpDataFormat& oriOpData, CommunicationData& communicationData)
{
    auto indexIt = opInfoIdxMap.find(connectionId);
    if (indexIt != opInfoIdxMap.end())
    {
        const auto& oriData = oriOpData[indexIt->second];
        std::string opName;
        std::string dataType;
        std::string algType;
        std::string groupName;
        std::string opType;
        std::tie(connectionId, opName, opData.relay, opData.retry, dataType, algType, opData.count, groupName, opType,
                 opData.modelId, opData.rankSize) = oriData;
        opData.dataType = GetEnumTypeValue(dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        opData.algType = algType;
        opData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
        opData.opType = opType;
        opData.deviceId = communicationData.deviceId;
    }
}

std::unordered_map<uint32_t, size_t> CommunicationInfoProcessor::GenOpInfoIdxMap(const OriOpDataFormat& oriOpData)
{
    std::unordered_map<uint32_t, size_t> opInfoIdxMap;
    uint32_t connectionId = 0;
    for (size_t i = 0; i < oriOpData.size(); ++i)
    {
        connectionId = std::get<0>(oriOpData[i]);
        opInfoIdxMap[connectionId] = i;
    }
    return opInfoIdxMap;
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
    if (communicationData.oriKfcTaskData.empty() || communicationData.oriKfcOpData.empty())
    {
        return true;
    }
    if (!FormatKfcData(taskData, opData, communicationData))
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
    if (communicationData.oriTaskData.empty())
    {
        ERROR("Get % data failed in %.", taskDBInfo.tableName, taskDBPath);
        return false;
    }
    communicationData.oriOpData = LoadOpData(opDBInfo);
    if (communicationData.oriOpData.empty())
    {
        ERROR("Get % data failed in %.", opDBInfo.tableName, opDBPath);
        return false;
    }
    if (!FormatData(taskData, opData, communicationData))
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
