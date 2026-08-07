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

#include "analysis/csrc/domain/services/persistence/device/aicpu_persistence.h"

#include <algorithm>
#include <unordered_map>

#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/hp_float.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;
namespace
{
static const std::string MI_NAME = "GetNext_dequeue_wait";
static const std::string EMPTY_NAME;
SyscntConversionParams GetSyscntConversionParams(const DeviceContext& context)
{
    CpuInfo cpuInfo;
    context.Getter(cpuInfo);
    HostStartLog hostStartLog;
    context.Getter(hostStartLog);
    uint64_t hostMonotonic = hostStartLog.clockMonotonicRaw;
    DeviceInfo deviceInfo;
    context.Getter(deviceInfo);
    DeviceStartLog deviceStartLog;
    context.Getter(deviceStartLog);
    if (!IsDoubleEqual(cpuInfo.frequency, 0.0) && hostStartLog.cntVctDiff)
    {
        uint64_t diffTime = static_cast<uint64_t>(hostStartLog.cntVctDiff * MILLI_SECOND / cpuInfo.frequency);
        if (UINT64_MAX - hostStartLog.clockMonotonicRaw >= diffTime)
        {
            hostMonotonic = hostStartLog.clockMonotonicRaw + diffTime;
        }
    }
    SyscntConversionParams params{deviceInfo.hwtsFrequency, deviceStartLog.cntVct, hostMonotonic};
    return params;
}

// stream_id, task_id, sys_start, sys_end, node_name, compute_time, memcpy_time, task_time, dispatch_time, total_time
using NodeFormat = std::vector<
    std::tuple<uint32_t, uint16_t, double, double, std::string, uint64_t, uint64_t, double, uint64_t, double>>;

using DpFormat = std::vector<std::tuple<double, std::string, std::string, uint64_t>>;

using ModelFormat = std::vector<std::tuple<uint64_t, uint32_t, double, uint16_t, uint64_t>>;

using MiFormat = std::vector<std::tuple<std::string, uint64_t, uint64_t, uint64_t, uint64_t>>;

using CommTurnFormat = std::vector<std::tuple<uint32_t, uint16_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                              uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

using ComputeTurnFormat =
    std::vector<std::tuple<uint32_t, uint16_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

using FlipTaskFormat = std::vector<std::tuple<uint16_t, double, uint32_t, uint32_t>>;

using MainStreamTaskFormat =
    std::vector<std::tuple<double, uint16_t, uint32_t, uint16_t, uint32_t, uint32_t, uint16_t>>;

using OpInfoFormat = std::vector<std::tuple<double, uint16_t, uint16_t, std::string, std::string, uint64_t, std::string,
                                            uint16_t, uint32_t, uint32_t, uint16_t>>;

using KfcInfosFormat = std::vector<
    std::tuple<double, std::string, std::string, std::string, uint32_t, uint32_t, uint32_t, std::string, uint16_t,
               uint32_t, std::string, std::string, std::string, double, std::string, std::string, uint64_t, std::string,
               std::string, std::string, std::string, std::string, uint16_t, uint32_t, uint32_t>>;
}  // namespace

uint32_t AicpuPersistence::GenerateAndSaveNode(const std::string& deviceFilePath)
{
    NodeFormat data;
    if (!Utils::Reserve(data, nodeData_.size()))
    {
        ERROR("Reserve for aicpu node data failed.");
        return ANALYSIS_ERROR;
    }

    for (auto& node : nodeData_)
    {
        if (node.node.runStartTick == 0 || node.node.runEndTick == 0) continue;
        const auto it = hostStreamInfo_.streamIdMap.find(node.taskId.taskId);
        if (it != hostStreamInfo_.streamIdMap.end())
        {
            node.taskId.streamId = it->second;
        }
        auto start_time = GetTimeFromSyscnt(node.node.runStartTick, params_);
        auto end_time = GetTimeFromSyscnt(node.node.runEndTick, params_);
        auto compute_time = (node.node.memcpyStartTime - node.node.computeStartTime) / 1000;
        auto memcpy_time = (node.node.memcpyEndTime - node.node.memcpyStartTime) / 1000;
        auto dispatch_time = node.node.dispatchTime / 1000;
        auto total_time = GetDurTimeFromSyscnt(node.node.tickAfterRun - node.node.submitTick, params_);
        data.emplace_back(node.taskId.streamId, node.taskId.taskId, start_time.Double(), end_time.Double(), "",
                          compute_time, memcpy_time, end_time.Double() - start_time.Double(), dispatch_time,
                          total_time.Double());
    }

    DBInfo dbInfo("aicpu.db", "AiCpuData");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, AicpuDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveDp(const std::string& deviceFilePath)
{
    DpFormat data;
    if (!Utils::Reserve(data, dpData_.size()))
    {
        ERROR("Reserve for aicpu dp data failed.");
        return ANALYSIS_ERROR;
    }

    for (const auto& dp : dpData_)
    {
        auto timeStamp = GetTimeFromSyscnt(dp.timeStamp, params_);
        data.emplace_back(timeStamp.Double(), std::string(dp.dp.action), std::string(dp.dp.source), dp.dp.size);
    }

    DBInfo dbInfo("aicpu.db", "AiCpuDp");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, AicpuDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveModel(const std::string& deviceFilePath)
{
    ModelFormat data;
    if (!Utils::Reserve(data, modelData_.size()))
    {
        ERROR("Reserve for aicpu model data failed.");
        return ANALYSIS_ERROR;
    }

    for (const auto& model : modelData_)
    {
        auto timeStamp = GetTimeFromSyscnt(model.timeStamp, params_);
        data.emplace_back(model.model.indexId, model.model.modelId, timeStamp.Double(), model.model.tagId,
                          model.model.eventId);
    }

    DBInfo dbInfo("step_trace.db", "ModelWithQ");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, StepTraceDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveMi(const std::string& deviceFilePath)
{
    MiFormat data;
    if (!Utils::Reserve(data, miData_.size()))
    {
        ERROR("Reserve for aicpu mi data failed.");
        return ANALYSIS_ERROR;
    }

    for (const auto& mi : miData_)
    {
        data.emplace_back((mi.mi.nodeTag == 1) ? MI_NAME : EMPTY_NAME, mi.mi.queueSize, mi.mi.runStartTime,
                          mi.mi.runEndTime, mi.mi.runEndTime - mi.mi.runStartTime);
    }

    DBInfo dbInfo("data_preprocess.db", "DataQueue");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, DataPreprocessDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveCommTurn(const std::string& deviceFilePath)
{
    CommTurnFormat data;
    if (!Utils::Reserve(data, commTurnData_.size()))
    {
        ERROR("Reserve for aicpu comm turn data failed.");
        return ANALYSIS_ERROR;
    }

    for (auto& commTurn : commTurnData_)
    {
        const auto it = hostStreamInfo_.streamIdMap.find(commTurn.taskId.taskId);
        if (it != hostStreamInfo_.streamIdMap.end())
        {
            commTurn.taskId.streamId = it->second;
        }
        data.emplace_back(commTurn.commTurn.deviceId, commTurn.taskId.streamId, commTurn.taskId.taskId,
                          commTurn.commTurn.commTurn, commTurn.commTurn.currentTurn, commTurn.commTurn.serverStartTime,
                          commTurn.commTurn.waitMsgStartTime, commTurn.commTurn.kfcAlgExeStartTime,
                          commTurn.commTurn.sendTaskStartTime, commTurn.commTurn.sendSqeFinishTime,
                          commTurn.commTurn.rtsqExeEndTime, commTurn.commTurn.serverEndTime);
    }

    DBInfo dbInfo("kfc_info.db", "KfcCommTurn");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveComputeTurn(const std::string& deviceFilePath)
{
    ComputeTurnFormat data;
    if (!Utils::Reserve(data, computeTurnData_.size()))
    {
        ERROR("Reserve for aicpu compute turn data failed.");
        return ANALYSIS_ERROR;
    }

    for (auto& computeTurn : computeTurnData_)
    {
        const auto it = hostStreamInfo_.streamIdMap.find(computeTurn.taskId.taskId);
        if (it != hostStreamInfo_.streamIdMap.end())
        {
            computeTurn.taskId.streamId = it->second;
        }
        data.emplace_back(computeTurn.computeTurn.deviceId, computeTurn.taskId.streamId, computeTurn.taskId.taskId,
                          computeTurn.computeTurn.computeTurn, computeTurn.computeTurn.currentTurn,
                          computeTurn.computeTurn.waitComputeStartTime, computeTurn.computeTurn.computeStartTime,
                          computeTurn.computeTurn.computeExeEndTime);
    }

    DBInfo dbInfo("kfc_info.db", "KfcComputeTurn");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveFlipTask(const std::string& deviceFilePath)
{
    FlipTaskFormat data;
    if (!Utils::Reserve(data, flipTaskData_.size()))
    {
        ERROR("Reserve for aicpu flip task data failed.");
        return ANALYSIS_ERROR;
    }

    for (auto& flipTask : flipTaskData_)
    {
        const auto it = hostStreamInfo_.streamIdMap.find(flipTask.taskId.taskId);
        if (it != hostStreamInfo_.streamIdMap.end())
        {
            flipTask.taskId.streamId = it->second;
        }
        auto timeStamp = GetTimeFromSyscnt(flipTask.timeStamp, params_);
        data.emplace_back(flipTask.taskId.streamId, timeStamp.Double(), flipTask.taskId.taskId,
                          flipTask.flipTask.flipNum);
    }

    DBInfo dbInfo("kfc_info.db", "AicpuTaskFlip");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveMainStreamTask(const std::string& deviceFilePath)
{
    MainStreamTaskFormat data;
    if (!Utils::Reserve(data, mainStreamTaskData_.size()))
    {
        ERROR("Reserve for aicpu main stream task data failed.");
        return ANALYSIS_ERROR;
    }

    for (auto& mainStreamTask : mainStreamTaskData_)
    {
        const auto it = hostStreamInfo_.streamIdMap.find(mainStreamTask.taskId.taskId);
        if (it != hostStreamInfo_.streamIdMap.end())
        {
            mainStreamTask.taskId.streamId = it->second;
        }
        const auto aicpuIt = deviceStreamInfo_.streamIdMap.find(mainStreamTask.aicpuTaskId.taskId);
        if (aicpuIt != deviceStreamInfo_.streamIdMap.end())
        {
            mainStreamTask.aicpuTaskId.streamId = aicpuIt->second;
        }
        auto timeStamp = GetTimeFromSyscnt(mainStreamTask.timeStamp, params_);
        data.emplace_back(timeStamp.Double(), mainStreamTask.mainStreamTask.aicpuStreamId,
                          mainStreamTask.mainStreamTask.aicpuTaskId, mainStreamTask.mainStreamTask.streamId,
                          mainStreamTask.mainStreamTask.taskId, mainStreamTask.taskId.batchId,
                          mainStreamTask.mainStreamTask.type);
    }

    DBInfo dbInfo("kfc_info.db", "AicpuMasterStreamHcclTask");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveOpInfo(const std::string& deviceFilePath)
{
    OpInfoFormat data;
    if (!Utils::Reserve(data, opInfoData_.size()))
    {
        ERROR("Reserve for aicpu op info data failed.");
        return ANALYSIS_ERROR;
    }

    for (const auto& opInfo : opInfoData_)
    {
        auto timeStamp = GetTimeFromSyscnt(opInfo.timeStamp, params_);
        data.emplace_back(timeStamp.Double(), static_cast<uint16_t>(opInfo.opInfo.relay),
                          static_cast<uint16_t>(opInfo.opInfo.retry),
                          NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, opInfo.opInfo.dataType),
                          geHashMap_[std::to_string(opInfo.opInfo.algType)], opInfo.opInfo.count,
                          std::to_string(opInfo.opInfo.groupName), opInfo.taskId.streamId, opInfo.taskId.taskId,
                          opInfo.opInfo.rankSize,
                          0  // hccl source
        );
    }

    DBInfo dbInfo("kfc_info.db", "DeviceHcclOpInfo");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

uint32_t AicpuPersistence::GenerateAndSaveKfcInfos(const std::string& deviceFilePath)
{
    KfcInfosFormat data;
    if (!Utils::Reserve(data, kfcInfosData_.size() * 2))
    {
        ERROR("Reserve for aicpu kfc infos data failed.");
        return ANALYSIS_ERROR;
    }

    for (const auto& aicpuData : kfcInfosData_)
    {
        for (const auto& info : aicpuData.KfcInfos.infos)
        {
            if (info.groupName == 0) continue;
            auto timeStamp = GetTimeFromSyscnt(info.timeStamp, params_);
            data.emplace_back(timeStamp.Double(), geHashMap_[std::to_string(info.itemId)], std::to_string(info.cclTag),
                              std::to_string(info.groupName), info.localRank, info.remoteRank, info.rankSize,
                              std::to_string(info.workFlowMode), info.planeID, UINT32_MAX,
                              std::to_string(info.notifyID), std::to_string(info.stage),
                              std::to_string(info.role),  // role暂时用不上，未转换为枚举内容
                              info.durationEstimated, std::to_string(info.srcAddr), std::to_string(info.dstAddr),
                              info.dataSize, NumberMapping::Get(NumberMapping::MappingType::HCCL_OP_TYPE, info.opType),
                              NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, info.dataType),
                              NumberMapping::Get(NumberMapping::MappingType::HCCL_LINK_TYPE, info.linkType),
                              NumberMapping::Get(NumberMapping::MappingType::HCCL_TRANSPORT_TYPE, info.transportType),
                              NumberMapping::Get(NumberMapping::MappingType::HCCL_RDMA_TYPE, info.rdmaType),
                              info.streamId, info.taskId, aicpuData.taskId.batchId);
        }
    }

    DBInfo dbInfo("kfc_info.db", "KfcInfo");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, KfcInfoDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceFilePath, SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);

    INFO("% count=%.", dbInfo.tableName, data.size());
    if (SaveData(data, dbInfo, dbPath))
    {
        INFO("Process % done!", dbInfo.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed.", dbInfo.tableName);
    return ANALYSIS_ERROR;
}

bool AicpuPersistence::ProcessAicpuDataByDataType(const std::vector<AicpuData>& aicpuData)
{
    static const std::unordered_map<AicpuType, std::vector<AicpuData> AicpuPersistence::*> typeMap = {
        {AicpuType::AICPU_NODE, &AicpuPersistence::nodeData_},
        {AicpuType::AICPU_DP, &AicpuPersistence::dpData_},
        {AicpuType::AICPU_MODEL, &AicpuPersistence::modelData_},
        {AicpuType::AICPU_MI, &AicpuPersistence::miData_},
        {AicpuType::KFC_COMM_TURN, &AicpuPersistence::commTurnData_},
        {AicpuType::KFC_COMPUTE_TURN, &AicpuPersistence::computeTurnData_},
        {AicpuType::HCCL_OP_INFO, &AicpuPersistence::opInfoData_},
        {AicpuType::AICPU_FLIP_TASK, &AicpuPersistence::flipTaskData_},
        {AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, &AicpuPersistence::mainStreamTaskData_},
        {AicpuType::KFC_HCCL_INFO, &AicpuPersistence::kfcInfosData_}};

    // 先按类型计数，再做精准 reserve
    std::unordered_map<AicpuType, size_t> typeCounts;
    for (const auto& aicpu : aicpuData)
    {
        typeCounts[aicpu.type]++;
    }
    for (const auto& item : typeCounts)
    {
        if (!Reserve(this->*(typeMap.at(item.first)), item.second))
        {
            return false;
        }
    }

    for (const auto& aicpu : aicpuData)
    {
        (this->*(typeMap.at(aicpu.type))).emplace_back(aicpu);
    }
    return true;
}

uint32_t AicpuPersistence::GenerateAndSaveData(const std::string& deviceFilePath)
{
    // 表驱动方式：使用 std::function 适配不同签名的函数
    using func = std::function<uint32_t()>;

    static const std::vector<std::pair<const std::vector<AicpuData> AicpuPersistence::*, func>> dataProcessMap = {
        {&AicpuPersistence::nodeData_, [this, deviceFilePath]() { return GenerateAndSaveNode(deviceFilePath); }},
        {&AicpuPersistence::dpData_, [this, deviceFilePath]() { return GenerateAndSaveDp(deviceFilePath); }},
        {&AicpuPersistence::modelData_, [this, deviceFilePath]() { return GenerateAndSaveModel(deviceFilePath); }},
        {&AicpuPersistence::miData_, [this, deviceFilePath]() { return GenerateAndSaveMi(deviceFilePath); }},
        {&AicpuPersistence::commTurnData_,
         [this, deviceFilePath]() { return GenerateAndSaveCommTurn(deviceFilePath); }},
        {&AicpuPersistence::computeTurnData_,
         [this, deviceFilePath]() { return GenerateAndSaveComputeTurn(deviceFilePath); }},
        {&AicpuPersistence::opInfoData_, [this, deviceFilePath]() { return GenerateAndSaveOpInfo(deviceFilePath); }},
        {&AicpuPersistence::flipTaskData_,
         [this, deviceFilePath]() { return GenerateAndSaveFlipTask(deviceFilePath); }},
        {&AicpuPersistence::mainStreamTaskData_,
         [this, deviceFilePath]() { return GenerateAndSaveMainStreamTask(deviceFilePath); }},
        {&AicpuPersistence::kfcInfosData_,
         [this, deviceFilePath]() { return GenerateAndSaveKfcInfos(deviceFilePath); }}};

    uint32_t result = ANALYSIS_OK;
    for (const auto& item : dataProcessMap)
    {
        const auto& data = this->*(item.first);
        if (!data.empty())
        {
            uint32_t ret = item.second();
            if (ret != ANALYSIS_OK)
            {
                ERROR("Save data failed, return %.", ret);
                result = ANALYSIS_ERROR;
            }
        }
    }
    return result;
}

void AicpuPersistence::ComputeAicpuBatchId()
{
    if (flipTaskData_.empty() || (mainStreamTaskData_.empty() && kfcInfosData_.empty()))
    {
        return;
    }

    using BatchTaskData = HalUniData;
    struct TaskEntry
    {
        BatchTaskData task;
        uint32_t* batchIdDest;  // 指向原始 AicpuData::taskId.batchId (uint16_t),以uint32_t保存
    };

    // 1. 按 stream_id 收集 flip 数据
    std::unordered_map<uint32_t, std::vector<BatchTaskData*>> flipByStream;
    std::vector<BatchTaskData> flipStorage;
    flipStorage.reserve(flipTaskData_.size());
    for (auto& flip : flipTaskData_)
    {
        BatchTaskData tmp;
        tmp.taskId = flip.taskId;
        tmp.timestamp = flip.timeStamp;
        flipStorage.push_back(tmp);
        flipByStream[flip.taskId.streamId].push_back(&flipStorage.back());
    }

    // 2. 按 stream_id 收集 task 数据
    std::unordered_map<uint32_t, std::vector<TaskEntry>> taskByStream;

    for (auto& mainStream : mainStreamTaskData_)
    {
        TaskEntry entry;
        entry.task.taskId = mainStream.taskId;
        entry.task.timestamp = mainStream.timeStamp;
        entry.batchIdDest = &mainStream.taskId.batchId;
        taskByStream[mainStream.taskId.streamId].push_back(entry);
    }
    for (auto& kfc : kfcInfosData_)
    {
        for (auto& info : kfc.KfcInfos.infos)
        {
            if (info.groupName == 0) continue;
            TaskEntry entry;
            entry.task.taskId = kfc.taskId;
            entry.task.timestamp = info.timeStamp;
            entry.batchIdDest = &kfc.taskId.batchId;
            taskByStream[info.streamId].push_back(entry);
        }
    }

    // 3. 按 stream_id 分组计算
    for (auto& pair : taskByStream)
    {
        uint32_t streamId = pair.first;
        auto flipIt = flipByStream.find(streamId);
        if (flipIt == flipByStream.end())
        {
            continue;  // 该 stream 没有 flip 数据，无法计算 batchId
        }

        auto& taskEntries = pair.second;
        auto& flipPtrs = flipIt->second;

        // 按 timestamp 排序
        std::sort(taskEntries.begin(), taskEntries.end(),
                  [](const TaskEntry& a, const TaskEntry& b) { return a.task.timestamp < b.task.timestamp; });
        std::sort(flipPtrs.begin(), flipPtrs.end(),
                  [](const BatchTaskData* a, const BatchTaskData* b) { return a->timestamp < b->timestamp; });

        // 构造 HalUniData* 指针数组
        std::vector<HalUniData*> taskPtrs;
        taskPtrs.reserve(taskEntries.size());
        for (auto& entry : taskEntries)
        {
            taskPtrs.push_back(&entry.task);
        }

        ModelingComputeBatchIdBinary(taskPtrs.data(), static_cast<uint32_t>(taskPtrs.size()), flipPtrs.data(),
                                     static_cast<uint16_t>(flipPtrs.size()));

        // 回写 batchId
        for (auto& entry : taskEntries)
        {
            *entry.batchIdDest = entry.task.taskId.batchId;
        }
    }
}

uint32_t AicpuPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto aicpuData = dataInventory.GetPtr<std::vector<AicpuData>>();
    auto deviceStreamInfo = dataInventory.GetPtr<DeviceStreamInfo>();
    auto hostStreamInfo = dataInventory.GetPtr<HostStreamInfo>();
    auto geHashMap = dataInventory.GetPtr<GeHashMap>();
    if (aicpuData == nullptr || deviceStreamInfo == nullptr || hostStreamInfo == nullptr || geHashMap == nullptr)
    {
        ERROR("There is no aicpu data, don't need to persistence");
        return ANALYSIS_ERROR;
    }
    params_ = GetSyscntConversionParams(deviceContext);
    hostStreamInfo_ = *hostStreamInfo;
    deviceStreamInfo_ = *deviceStreamInfo;
    geHashMap_ = *geHashMap;
    if (!ProcessAicpuDataByDataType(*aicpuData))
    {
        ERROR("Process aicpu data failed");
        return ANALYSIS_ERROR;
    }
    ComputeAicpuBatchId();
    return GenerateAndSaveData(deviceContext.GetDeviceFilePath());
}
REGISTER_PROCESS_SEQUENCE(AicpuPersistence, true, AicpuParser, LoadHostData);
REGISTER_PROCESS_DEPENDENT_DATA(AicpuPersistence, std::vector<AicpuData>, DeviceStreamInfo, HostStreamInfo, GeHashMap);
REGISTER_PROCESS_SUPPORT_CHIP(AicpuPersistence, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
