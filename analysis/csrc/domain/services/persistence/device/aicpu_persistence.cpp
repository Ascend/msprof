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
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
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
    std::vector<std::tuple<double, uint16_t, uint32_t, uint16_t, uint32_t, uint32_t, uint32_t, uint16_t>>;

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

    DBInfo dbInfo("ai_cpu.db", "AiCpuData");
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

    DBInfo dbInfo("ai_cpu.db", "AiCpuDP");
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
                          mainStreamTask.mainStreamTask.aicpuTaskId, mainStreamTask.aicpuTaskId.batchId,
                          mainStreamTask.mainStreamTask.streamId, mainStreamTask.mainStreamTask.taskId,
                          mainStreamTask.taskId.batchId, mainStreamTask.mainStreamTask.type);
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

    // 注意：不能声明为 static。static 会导致首次调用时捕获的 this/deviceFilePath
    // 被所有 device 实例共享，后续卡的落盘路径全部错写成第一张卡的目录。
    const std::vector<std::pair<const std::vector<AicpuData> AicpuPersistence::*, func>> dataProcessMap = {
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

void AicpuPersistence::ComputeAicpuBatchId(std::vector<HalTrackData>* halTrackData)
{
    using BatchTaskData = HalUniData;
    struct TaskEntry
    {
        BatchTaskData task;
        uint32_t* batchIdDest;  // 指向原始 AicpuData::taskId.batchId / aicpuTaskId.batchId (uint16_t),以uint32_t保存
    };

    // 1. 主流侧：用 aicpu flip(flipTaskData_) 计算 taskId.batchId
    if (!flipTaskData_.empty() && (!mainStreamTaskData_.empty() || !kfcInfosData_.empty()))
    {
        // 按 stream_id 收集 flip 数据
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

        // 按 stream_id 收集 task 数据
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

        // 按 stream_id 分组计算
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

    // 2. aicpu 侧：用 device flip(halTrackData，TS_TASK_FLIP) 计算 aicpuTaskId.batchId。
    //    分组 key 用原始 aicpuStreamId（mainStreamTask.mainStreamTask.aicpuStreamId），与 device flip 的
    //    hd.taskId.streamId（原始 device stream id）同空间；flip 数据由 TsTrackParser 注入，可能为空则跳过。
    if (halTrackData == nullptr || halTrackData->empty() || mainStreamTaskData_.empty())
    {
        return;
    }
    auto flipByAicpuStream = GetFlipData(*halTrackData);  // 按 hd.taskId.streamId 分组
    std::unordered_map<uint32_t, std::vector<TaskEntry>> aicpuTaskByStream;
    for (auto& mainStream : mainStreamTaskData_)
    {
        TaskEntry entry;
        entry.task.taskId = mainStream.aicpuTaskId;
        entry.task.timestamp = mainStream.timeStamp;
        entry.batchIdDest = &mainStream.aicpuTaskId.batchId;
        aicpuTaskByStream[mainStream.mainStreamTask.aicpuStreamId].push_back(entry);
    }
    for (auto& pair : aicpuTaskByStream)
    {
        auto flipIt = flipByAicpuStream.find(pair.first);
        if (flipIt == flipByAicpuStream.end())
        {
            continue;  // 该 stream 没有 device flip 数据，无法计算 aicpuBatchId
        }

        auto& taskEntries = pair.second;
        auto& flipPtrs = flipIt->second;

        // 按 timestamp 排序
        std::sort(taskEntries.begin(), taskEntries.end(),
                  [](const TaskEntry& a, const TaskEntry& b) { return a.task.timestamp < b.task.timestamp; });
        std::sort(flipPtrs.begin(), flipPtrs.end(),
                  [](const HalTrackData* a, const HalTrackData* b) { return a->hd.timestamp < b->hd.timestamp; });

        // 构造 HalUniData* 指针数组（HalUniData 为 HalTrackData 首成员，可直接重解释）
        std::vector<HalUniData*> taskPtrs;
        taskPtrs.reserve(taskEntries.size());
        for (auto& entry : taskEntries)
        {
            taskPtrs.push_back(&entry.task);
        }

        ModelingComputeBatchIdBinary(taskPtrs.data(), static_cast<uint32_t>(taskPtrs.size()),
                                     ReinterpretConvert<HalUniData**>(flipPtrs.data()),
                                     static_cast<uint16_t>(flipPtrs.size()));

        // 回写 aicpuBatchId
        for (auto& entry : taskEntries)
        {
            *entry.batchIdDest = entry.task.taskId.batchId;
        }
    }
}

std::vector<KfcInfoData> AicpuPersistence::BuildKfcInfoData() const
{
    // 与 GenerateAndSaveKfcInfos 的 KfcInfo 落盘行同源同值：
    // 同序遍历 kfcInfosData_.KfcInfos.infos（跳过 groupName==0），字段取值与落盘行一致
    std::vector<KfcInfoData> data;
    if (!Utils::Reserve(data, kfcInfosData_.size() * 2))
    {
        ERROR("Reserve for aicpu kfc infos data failed.");
        return data;
    }
    for (const auto& aicpuData : kfcInfosData_)
    {
        for (const auto& info : aicpuData.KfcInfos.infos)
        {
            if (info.groupName == 0)
            {
                continue;
            }
            KfcInfoData item;
            // 与 GenerateAndSaveKfcInfos 同源：geHashMap_ 按 itemId 查 op_name，缺省为空串
            const auto geHashIt = geHashMap_.find(std::to_string(info.itemId));
            item.hcclName = (geHashIt != geHashMap_.end()) ? geHashIt->second : std::string();  // op_name
            item.streamId = info.streamId;
            item.taskId = info.taskId;
            item.contextId = UINT32_MAX;  // 与落盘行 context_id 一致（GE 默认 context）
            item.batchId = aicpuData.taskId.batchId;
            item.localRank = info.localRank;
            item.remoteRank = info.remoteRank;
            item.rankSize = info.rankSize;
            item.planeId = info.planeID;
            item.notifyId = std::to_string(info.notifyID);
            item.size = static_cast<double>(info.dataSize);
            item.opType = NumberMapping::Get(NumberMapping::MappingType::HCCL_OP_TYPE, info.opType);
            item.dataType = NumberMapping::Get(NumberMapping::MappingType::HCCL_DATA_TYPE, info.dataType);
            item.linkType = NumberMapping::Get(NumberMapping::MappingType::HCCL_LINK_TYPE, info.linkType);
            item.transportType =
                NumberMapping::Get(NumberMapping::MappingType::HCCL_TRANSPORT_TYPE, info.transportType);
            item.rdmaType = NumberMapping::Get(NumberMapping::MappingType::HCCL_RDMA_TYPE, info.rdmaType);
            data.emplace_back(std::move(item));
        }
    }
    return data;
}

std::vector<MasterStreamTaskData> AicpuPersistence::BuildMasterStreamTaskData() const
{
    // 与 GenerateAndSaveMainStreamTask 的 AicpuMasterStreamHcclTask 落盘行同源同值：
    // 字段取值与落盘行一致（timestamp / aicpu_stream_id / aicpu_task_id / aicpu_batch_id /
    // stream_id / task_id / batch_id / type）
    std::vector<MasterStreamTaskData> data;
    if (!Utils::Reserve(data, mainStreamTaskData_.size()))
    {
        ERROR("Reserve for aicpu main stream task data failed.");
        return data;
    }
    for (const auto& mainStreamTask : mainStreamTaskData_)
    {
        MasterStreamTaskData item;
        item.taskType = mainStreamTask.mainStreamTask.type;
        item.streamId = mainStreamTask.mainStreamTask.streamId;
        item.taskId = mainStreamTask.mainStreamTask.taskId;
        item.batchId = mainStreamTask.taskId.batchId;
        item.aicpuStreamId = mainStreamTask.mainStreamTask.aicpuStreamId;
        item.aicpuTaskId = mainStreamTask.mainStreamTask.aicpuTaskId;
        item.aicpuBatchId = mainStreamTask.aicpuTaskId.batchId;
        item.timeStamp = GetTimeFromSyscnt(mainStreamTask.timeStamp, params_).Double();
        data.emplace_back(std::move(item));
    }
    return data;
}

uint32_t AicpuPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    INFO("Start to process aicpu data for device, path %.", deviceContext.GetDeviceFilePath());
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
    // device flip(TS_TASK_FLIP) 由 TsTrackParser 注入，用于计算 aicpuStreamId/aicpuTaskId 的 aicpuBatchId；
    // 未注入时为空指针，ComputeAicpuBatchId 内部跳过 aicpu 侧
    auto halTrackData = dataInventory.GetPtr<std::vector<HalTrackData>>();
    ComputeAicpuBatchId(halTrackData.get());

    // 注入 kfc 上游实体：KfcCalculator 与 AicpuPersistence 同进程，直接消费内存数据（越过 kfc_info.db 读取）。
    // 注入与落盘同源同值，落盘仍由 GenerateAndSaveData 完成，两者都不可丢失。
    std::vector<KfcInfoData> kfcInfoData = BuildKfcInfoData();
    std::shared_ptr<std::vector<KfcInfoData>> kfcInfoDataPtr;
    MAKE_SHARED0_NO_OPERATION(kfcInfoDataPtr, std::vector<KfcInfoData>, std::move(kfcInfoData));
    if (kfcInfoDataPtr == nullptr || !dataInventory.Inject(kfcInfoDataPtr))
    {
        ERROR("Inject kfc info data failed.");
    }
    std::vector<MasterStreamTaskData> masterStreamTaskData = BuildMasterStreamTaskData();
    std::shared_ptr<std::vector<MasterStreamTaskData>> masterStreamTaskDataPtr;
    MAKE_SHARED0_NO_OPERATION(masterStreamTaskDataPtr, std::vector<MasterStreamTaskData>,
                              std::move(masterStreamTaskData));
    if (masterStreamTaskDataPtr == nullptr || !dataInventory.Inject(masterStreamTaskDataPtr))
    {
        ERROR("Inject master stream task data failed.");
    }
    return GenerateAndSaveData(deviceContext.GetDeviceFilePath());
}
REGISTER_PROCESS_SEQUENCE(AicpuPersistence, true, AicpuParser, LoadHostData, TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(AicpuPersistence, std::vector<AicpuData>, DeviceStreamInfo, HostStreamInfo, GeHashMap,
                                std::vector<HalTrackData>, std::vector<KfcInfoData>, std::vector<MasterStreamTaskData>);
REGISTER_PROCESS_SUPPORT_CHIP(AicpuPersistence, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
