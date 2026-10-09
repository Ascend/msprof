/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/data_process/ai_task/kfc_task_processor.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Domain::Environment;

// 创建 KfcTurnData 的函数类型
using CreateKfcTurnDataFunc = std::function<KfcTurnData(const KfcCommTurn&, const SyscntConversionParams&)>;

KfcTaskProcessor::KfcTaskProcessor(const std::string& profPath) : DataProcessor(profPath) {}

bool KfcTaskProcessor::Process(DataInventory& dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<KfcTurnData> result;
    bool flag = true;
    for (const auto& devicePath : deviceList)
    {
        DBInfo kfcCommTurnDB("kfc_info.db", "KfcCommTurn");
        DBInfo kfcComputeTurnDB("kfc_info.db", "KfcComputeTurn");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, kfcComputeTurnDB.dbName});
        if (!kfcCommTurnDB.ConstructDBRunner(dbPath) || !kfcComputeTurnDB.ConstructDBRunner(dbPath))
        {
            flag = false;
            continue;
        }

        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_))
        {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
            return {};
        }
        ProfTimeRecord record;
        if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId))
        {
            ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
            return {};
        }

        auto computeStatus = CheckPathAndTable(dbPath, kfcComputeTurnDB, false);
        if (computeStatus == CHECK_SUCCESS)
        {
            auto computeData = LoadComputeData(kfcComputeTurnDB, dbPath);
            std::vector<KfcTurnData> kfcComputeTurnData = FormatComputeData(computeData, deviceId, params, record);
            FilterDataByStartTime(kfcComputeTurnData, record.startTimeNs, PROCESSOR_NAME_KFC_TASK);
            result.insert(result.end(), kfcComputeTurnData.begin(), kfcComputeTurnData.end());
        }
        else
        {
            flag = (computeStatus == NOT_EXIST) && flag;
        }

        auto commStatus = CheckPathAndTable(dbPath, kfcCommTurnDB, false);
        if (commStatus == CHECK_SUCCESS)
        {
            auto commData = LoadCommData(kfcCommTurnDB, dbPath, deviceId);
            std::vector<KfcTurnData> kfcCommTurnData = FormatCommData(commData, params, record);
            FilterDataByStartTime(kfcCommTurnData, record.startTimeNs, PROCESSOR_NAME_KFC_TASK);
            result.insert(result.end(), kfcCommTurnData.begin(), kfcCommTurnData.end());
        }
        else
        {
            flag = (commStatus == NOT_EXIST) && flag;
        }
    }

    if (!SaveToDataInventory<KfcTurnData>(std::move(result), dataInventory, PROCESSOR_NAME_KFC_TASK))
    {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_TASK);
        flag = false;
    }
    return flag;
}

std::vector<KfcCommTurn> KfcTaskProcessor::LoadCommData(const DBInfo& kfcDB, const std::string& dbPath,
                                                        uint16_t deviceId)
{
    std::vector<KfcCommTurn> kfcCommTurnData;
    OriKfcCommData oriData;
    if (kfcDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return kfcCommTurnData;
    }
    // 原taskTurn里的deviceId不可用 底层感知不到真实deviceId 上报的是rankId
    std::string sql = "SELECT " + std::to_string(deviceId) +
                      " AS device_id, stream_id, task_id, comm_turn, "
                      "current_turn, server_start_time, wait_msg_start_time, kfc_alg_exe_start_time, "
                      "send_task_start_time, send_sqe_finish_time, rtsq_exe_end_time, server_end_time "
                      "FROM " +
                      kfcDB.tableName;
    if (!kfcDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query KfcCommTurn data failed, db path is %.", dbPath);
        return kfcCommTurnData;
    }
    for (auto row : oriData)
    {
        KfcCommTurn data{};
        std::tie(data.deviceId, data.streamId, data.taskId, data.commTurn, data.currentTurn, data.serverStartTime,
                 data.waitMsgStartTime, data.kfcAlgExeStartTime, data.sendTaskStartTime, data.sendSqeFinishTime,
                 data.rtsqExeEndTime, data.serverEndTime) = row;
        kfcCommTurnData.push_back(data);
    }

    return kfcCommTurnData;
}

std::vector<KfcComputeTurn> KfcTaskProcessor::LoadComputeData(const DBInfo& kfcDB, const std::string& dbPath)
{
    OriKfcComputeData oriData;
    std::vector<KfcComputeTurn> kfcComputeDatas;
    if (kfcDB.dbRunner == nullptr)
    {
        ERROR("Create % connection failed.", dbPath);
        return kfcComputeDatas;
    }
    std::string sql =
        "SELECT device_id, stream_id, task_id, compute_turn, current_turn, wait_compute_start_time, "
        "compute_start_time, compute_exe_end_time "
        "FROM " +
        kfcDB.tableName;
    if (!kfcDB.dbRunner->QueryData(sql, oriData))
    {
        ERROR("Query api data failed, db path is %.", dbPath);
        return kfcComputeDatas;
    }
    for (auto row : oriData)
    {
        KfcComputeTurn data{};
        std::tie(data.deviceId, data.streamId, data.taskId, data.computeTurn, data.currentTurn,
                 data.waitComputeStartTime, data.computeStartTime, data.computeExeEndTime) = row;
        kfcComputeDatas.push_back(data);
    }
    return kfcComputeDatas;
}

std::vector<KfcTurnData> KfcTaskProcessor::FormatCommData(const std::vector<KfcCommTurn>& oriCommData,
                                                          const SyscntConversionParams& params,
                                                          const ProfTimeRecord& record)
{
    std::vector<KfcTurnData> ans;
    HPFloat startTimestamp;
    for (const auto& commData : oriCommData)
    {
        startTimestamp = GetTimeFromSyscnt(commData.serverStartTime, params);
        ans.emplace_back("StartServer " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.waitMsgStartTime - commData.serverStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.waitMsgStartTime, params);
        ans.emplace_back(
            "TaskWaitRequest " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
            commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
            GetDurTimeFromSyscnt(commData.kfcAlgExeStartTime - commData.waitMsgStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.kfcAlgExeStartTime, params);
        ans.emplace_back(
            "TaskOrchestration " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
            commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
            GetDurTimeFromSyscnt(commData.sendTaskStartTime - commData.kfcAlgExeStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.sendTaskStartTime, params);
        ans.emplace_back(
            "TaskLaunch " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId, commData.taskId,
            GetLocalTime(startTimestamp, record).Uint64(),
            GetDurTimeFromSyscnt(commData.sendSqeFinishTime - commData.sendTaskStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.sendSqeFinishTime, params);
        ans.emplace_back("TaskExecute " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.rtsqExeEndTime - commData.sendSqeFinishTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.rtsqExeEndTime, params);
        ans.emplace_back("Finalize " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.serverEndTime - commData.rtsqExeEndTime, params).Double());
    }
    return ans;
}

std::vector<KfcTurnData> KfcTaskProcessor::FormatComputeData(const std::vector<KfcComputeTurn>& oriComputeData,
                                                             uint16_t deviceId, const SyscntConversionParams& params,
                                                             const ProfTimeRecord& record)
{
    std::vector<KfcTurnData> ans;
    HPFloat startTimestamp;
    for (auto& data : oriComputeData)
    {
        startTimestamp = GetTimeFromSyscnt(data.waitComputeStartTime, params);
        ans.emplace_back("WaitCompute " + std::to_string(data.currentTurn), deviceId, data.streamId, data.taskId,
                         GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(data.computeStartTime - data.waitComputeStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(data.computeStartTime, params);
        ans.emplace_back("Compute " + std::to_string(data.currentTurn), deviceId, data.streamId, data.taskId,
                         GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(data.computeExeEndTime - data.computeStartTime, params).Double());
    }
    return ans;
}
}  // namespace Domain
}  // namespace Analysis
