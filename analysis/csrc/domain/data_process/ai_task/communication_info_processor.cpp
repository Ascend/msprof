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

OriTaskDataFormat CommunicationInfoProcessor::LoadTaskData(const DBInfo& taskSingleDevice, bool needSource)
{
    OriTaskDataFormat oriTaskData;
    // needSource=true（KfcTask）时 SELECT 追加 source 列，逐行记录该算子是 HCCL 还是 MC2；
    // HCCL 表无 source 列，不加该列，tuple 末尾 source 元素越界读恒为 0(=HCCL)
    std::string sql{
        "SELECT model_id, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
        "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
        "rdma_type, timestamp, duration, op_id, bandwidth, is_master, iter_id" +
        std::string(needSource ? ", source" : "") + " FROM " + taskSingleDevice.tableName};
    if (!taskSingleDevice.dbRunner->QueryData(sql, oriTaskData))
    {
        ERROR("Failed to obtain data from the % table.", taskSingleDevice.tableName);
    }
    return oriTaskData;
}

OriOpDataFormat CommunicationInfoProcessor::LoadOpData(const DBInfo& opSingleDevice, bool needSource)
{
    OriOpDataFormat oriOpData;
    // needSource=true（KfcOP）时 SELECT 追加 source 列；HCCL 表无该列，越界读恒为 0(=HCCL)
    std::string sql{
        "SELECT connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, "
        "model_id, rank_size, start, end, iter_id" +
        std::string(needSource ? ", source" : "") + " FROM " + opSingleDevice.tableName};
    if (!opSingleDevice.dbRunner->QueryData(sql, oriOpData))
    {
        ERROR("Failed to obtain data from the % table.", opSingleDevice.tableName);
    }
    return oriOpData;
}

bool CommunicationInfoProcessor::FormatTaskData(const OriTaskDataFormat& oriTaskData,
                                                std::vector<CommunicationTaskData>& taskFormatData,
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
    return true;
}

bool CommunicationInfoProcessor::FormatOpData(const OriOpDataFormat& oriOpData,
                                              std::vector<CommunicationOpData>& opFormatData,
                                              CommunicationData& communicationData, HcclType type)
{
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
    int32_t source;
    std::string groupName;
    std::tie(taskData.modelId, taskData.hcclName, groupName, taskData.planeId, taskData.streamId, taskData.taskId,
             taskData.srcRank, taskData.dstRank, taskData.transportType, taskData.size, taskData.dataType,
             taskData.linkType, taskData.contextId, taskData.notifyId, taskData.batchId, taskData.rdmaType, timestamp,
             duration, taskData.opId, taskData.bandwidth, taskData.isMaster, taskData.iterId, source) = oriData;
    HPFloat timestampFp{timestamp};
    HPFloat durationFp{duration};
    taskData.timestamp = GetLocalTime(timestampFp, communicationData.timeRecord).Uint64();
    taskData.duration = durationFp.Uint64();
    taskData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
    taskData.taskType = taskData.hcclName;
    // source 从数据中来：KfcTask 的 source 列记录该行是 HCCL 还是 MC2，不能一律按默认类型硬标；
    // 仅当数据中 source 缺失(INVALID)时才回退到入参 type
    taskData.source = (source == static_cast<int32_t>(HcclType::INVALID)) ? type : static_cast<HcclType>(source);
    taskData.deviceId = communicationData.deviceId;
    return taskData;
}

CommunicationOpData CommunicationInfoProcessor::UpdateOpInfo(const HcclOpFormat& oriData,
                                                             CommunicationData& communicationData, HcclType type)
{
    CommunicationOpData opData;
    double start, end;
    int32_t source;
    std::string groupName;
    std::tie(opData.connectionId, opData.opName, opData.relay, opData.retry, opData.dataType, opData.algType,
             opData.count, groupName, opData.opType, opData.modelId, opData.rankSize, start, end, opData.iterId,
             source) = oriData;
    HPFloat startFp{start};
    HPFloat endFp{end};
    opData.timestamp = Utils::GetLocalTime(startFp, communicationData.timeRecord).Uint64();
    opData.end = Utils::GetLocalTime(endFp, communicationData.timeRecord).Uint64();
    opData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
    // source 从数据中来：KfcOP 的 source 列记录该行是 HCCL 还是 MC2，不能一律按默认类型硬标
    opData.source = (source == static_cast<int32_t>(HcclType::INVALID)) ? type : static_cast<HcclType>(source);
    opData.deviceId = communicationData.deviceId;
    return opData;
}

bool CommunicationInfoProcessor::ProcessTaskTable(const std::string& devicePath, const std::string& dbName,
                                                  const std::string& tableName, OriTaskDataFormat& oriTaskData,
                                                  std::vector<CommunicationTaskData>& taskFormatData,
                                                  CommunicationData& communicationData, HcclType type, bool needSource)
{
    DBInfo taskDBInfo(dbName, tableName);
    std::string taskDBPath = Utils::File::PathJoin({devicePath, SQLITE, taskDBInfo.dbName});
    if (!taskDBInfo.ConstructDBRunner(taskDBPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(taskDBPath, taskDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        // 表缺失/为空视为正常，不影响后续业务；损坏才视为失败
        return status == NOT_EXIST;
    }
    oriTaskData = LoadTaskData(taskDBInfo, needSource);
    return FormatTaskData(oriTaskData, taskFormatData, communicationData, type);
}

bool CommunicationInfoProcessor::ProcessOpTable(const std::string& devicePath, const std::string& dbName,
                                                const std::string& tableName, OriOpDataFormat& oriOpData,
                                                std::vector<CommunicationOpData>& opFormatData,
                                                CommunicationData& communicationData, HcclType type, bool needSource)
{
    DBInfo opDBInfo(dbName, tableName);
    std::string opDBPath = Utils::File::PathJoin({devicePath, SQLITE, opDBInfo.dbName});
    if (!opDBInfo.ConstructDBRunner(opDBPath))
    {
        return false;
    }
    auto status = CheckPathAndTable(opDBPath, opDBInfo, false);
    if (status != CHECK_SUCCESS)
    {
        // 表缺失/为空视为正常，不影响后续业务；损坏才视为失败
        return status == NOT_EXIST;
    }
    oriOpData = LoadOpData(opDBInfo, needSource);
    return FormatOpData(oriOpData, opFormatData, communicationData, type);
}

bool CommunicationInfoProcessor::ProcessKfcData(const std::string& devicePath,
                                                std::vector<CommunicationTaskData>& taskData,
                                                std::vector<CommunicationOpData>& opData,
                                                CommunicationData& communicationData)
{
    // 先清空原始数据，避免多 device 场景下残留上一 device 的数据
    communicationData.oriKfcTaskData.clear();
    communicationData.oriKfcOpData.clear();
    // KfcTask / KfcOP 两表独立处理，谁有谁加载；缺表/空表不阻塞另一张表
    // source 从数据中来（needSource=true）：KfcTask 中既有 source=hccl 也有 source=mc2 的行，
    // 不能一律硬标成 MC2；type 仅作为数据中 source 缺失(INVALID)时的兜底，传 INVALID 表示不强制默认
    return ProcessTaskTable(devicePath, "hccl_single_device.db", "KfcTask", communicationData.oriKfcTaskData, taskData,
                            communicationData, HcclType::INVALID, true) &&
           ProcessOpTable(devicePath, "hccl_single_device.db", "KfcOP", communicationData.oriKfcOpData, opData,
                          communicationData, HcclType::INVALID, true);
}

bool CommunicationInfoProcessor::ProcessHcclData(const std::string& devicePath,
                                                 std::vector<CommunicationTaskData>& taskData,
                                                 std::vector<CommunicationOpData>& opData,
                                                 CommunicationData& communicationData)
{
    // 先清空原始数据，避免多 device 场景下残留上一 device 的数据
    communicationData.oriTaskData.clear();
    communicationData.oriOpData.clear();
    // HCCLTaskSingleDevice / HCCLOpSingleDevice 两表独立处理，谁有谁加载；缺表/空表不阻塞另一张表
    // 这两张表无 source 列，needSource=false：SELECT 不含 source，tuple 末尾越界读为 0(=HCCL)
    return ProcessTaskTable(devicePath, "hccl_single_device.db", "HCCLTaskSingleDevice", communicationData.oriTaskData,
                            taskData, communicationData, HcclType::HCCL, false) &&
           ProcessOpTable(devicePath, "hccl_single_device.db", "HCCLOpSingleDevice", communicationData.oriOpData,
                          opData, communicationData, HcclType::HCCL, false);
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
