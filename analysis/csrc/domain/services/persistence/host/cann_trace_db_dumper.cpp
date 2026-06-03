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

#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"

namespace Analysis
{
namespace Domain
{
using Context = Analysis::Domain::Environment::Context;
using ThreadPool = Analysis::Utils::ThreadPool;
using RTAddInfoCenter = Analysis::Domain::Host::Cann::RTAddInfoCenter;
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
using MappingType = NumberMapping::MappingType;
using namespace Infra;

using HCCLOpsDumpData = std::vector<
    std::tuple<uint32_t, uint64_t, int32_t, uint32_t, std::string, std::string, std::string, uint64_t, uint64_t,
               std::string, int64_t, int64_t, int32_t, int32_t, std::string, std::string, uint64_t, std::string>>;

using HostTasksDumpData = std::vector<std::tuple<uint32_t, int64_t, uint32_t, uint32_t, std::string, uint32_t,
                                                 std::string, std::string, uint32_t, std::string, int64_t, uint32_t>>;

using HcclTasksDumpData =
    std::vector<std::tuple<uint32_t, int64_t, std::string, std::string, int64_t, std::string, double, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, std::string, double,
                           std::string, std::string, std::string, std::string, uint32_t, uint32_t>>;

using GeFusionOpsDumpData = std::vector<std::tuple<uint64_t, std::string, uint32_t, std::string, std::string,
                                                   std::string, std::string, std::string, std::string>>;

namespace
{
const int32_t INVALID_VALUE = -1;
const std::string NA = "N/A";
const uint32_t INPUT_FORMAT_INDEX = 0;
const uint32_t OUTPUT_FORMAT_INDEX = 1;
const std::string KERNEL_MIX_AIC_TASK_TYPE = "KERNEL_MIX_AIC";
const std::string KERNEL_MIX_AIV_TASK_TYPE = "KERNEL_MIX_AIV";
const std::string KERNEL_SIMT_TASK_TYPE = "KERNEL_SIMT";
const std::unordered_map<std::string, uint32_t> RtsTaskTypeMap = {
    {"KERNEL_AICORE", 0},  {"KERNEL_AICPU", 1},   {"KERNEL_AIVEC", 2},
    {"KERNEL_MIX_AIC", 4}, {"KERNEL_MIX_AIV", 5}, {"KERNEL_SIMT", 2},
};  // KERNEL_SIMT类型算子是aiv类型的

std::string TransTaskTypeFromRtsToGe(uint64_t rtsTaskType)
{
    std::string taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, rtsTaskType);
    auto it = RtsTaskTypeMap.find(taskType);
    return (it != RtsTaskTypeMap.end()) ? NumberMapping::Get(MappingType::GE_TASK_TYPE, it->second) : taskType;
}

void AddHcclOpDumpData(HCCLOpsDumpData &data, const std::shared_ptr<Analysis::Domain::Operator> &op)
{
    auto bigOpDesc = op->hcclBigOpDesc;
    // several attributes can not get currently, use default value
    uint64_t modelId = bigOpDesc->modelId;
    int32_t indexId = bigOpDesc->indexId;
    std::string isDynamic = NA;
    int64_t connectionId = bigOpDesc->connectionId;
    int64_t kfcConnectionId = bigOpDesc->kfcConnectionId;
    uint32_t thread_id = bigOpDesc->thread_id;
    std::string taskType = "COMMUNICATION";
    std::string opType = NA;
    auto nodeDesc = bigOpDesc->nodeDesc;
    if (nodeDesc != nullptr)
    {
        opType = HashData::GetInstance().Get(nodeDesc->data.nodeBasicInfo.opType);
        isDynamic = std::to_string(nodeDesc->data.nodeBasicInfo.opState);
    }
    int32_t relay = -1;
    int32_t retry = -1;
    std::string dataType = NA;
    std::string algType = NA;
    uint64_t count = UINT64_MAX;
    std::string groupName = NA;
    auto opInfoDesc = bigOpDesc->opInfoDesc;
    if (opInfoDesc != nullptr)
    {
        relay = opInfoDesc->data.hcclopInfo.relay;
        retry = opInfoDesc->data.hcclopInfo.retry;
        dataType = NumberMapping::Get(MappingType::HCCL_DATA_TYPE, opInfoDesc->data.hcclopInfo.dataType);
        algType = HashData::GetInstance().Get(opInfoDesc->data.hcclopInfo.algType);
        count = opInfoDesc->data.hcclopInfo.count;
        groupName = std::to_string(opInfoDesc->data.hcclopInfo.groupName);
    }
    data.emplace_back(bigOpDesc->deviceId, modelId, indexId, thread_id, HashData::GetInstance().Get(op->name), taskType,
                      opType, bigOpDesc->beginTime, bigOpDesc->endTime, isDynamic, connectionId, kfcConnectionId, relay,
                      retry, dataType, algType, count, groupName);
}
}  // namespace
CANNTraceDBDumper::CANNTraceDBDumper(std::string hostFilePath) : hostFilePath_(std::move(hostFilePath)), result_(true)
{
}

bool CANNTraceDBDumper::DumpData(TreeAnalyzer analyzer)
{
    Utils::TimeLogger t{"Dump CANN data"};
    ThreadPool pool(poolSize_);
    pool.Start();
    pool.AddTask(
        [this, &analyzer]()
        {
            HCCLBigOpDescs hcclBigOps = analyzer.GetHcclBigOps();
            HostTasks hcclTasks = analyzer.GetHCCLTasks();
            DumpHcclOps(hcclBigOps);
            DumpHcclTasks(hcclTasks);
        });
    pool.AddTask(
        [this, &analyzer]()
        {
            HostTasks hostTasks = analyzer.GetTasks();
            DumpHostTasks(hostTasks);
        });
    pool.AddTask(
        [this, &analyzer]()
        {
            HostTasks computeTasks = analyzer.GetComputeTasks();
            DumpOpDesc(computeTasks);
        });
    pool.AddTask(
        [this, &analyzer]()
        {
            GeFusionOpInfos geFusionOps = analyzer.GetGeFusionOpInfos();
            DumpGeFusionOps(geFusionOps);
        });
    pool.WaitAllTasks();
    pool.Stop();
    if (!result_)
    {
        ERROR("Dump CANN data failed!");
        return false;
    }
    return true;
}

void CANNTraceDBDumper::DumpHcclOps(const HCCLBigOpDescs &hcclOps)
{
    if (hcclOps.empty())
    {
        INFO("Empty hccl OPs");
        return;
    }
    Utils::TimeLogger t{"Dump hccl OPs"};
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    if (!hcclOpDBRunner.CreateTable("HCCLOP", hcclDB.GetTableCols("HCCLOP")))
    {
        ERROR("DumpHcclOps: create table HCCLOP failed");
        result_ = false;
        return;
    }
    HCCLOpsDumpData data;
    if (!Utils::Reserve(data, hcclOps.size()))
    {
        result_ = false;
        return;
    }
    for (const auto &op : hcclOps)
    {
        if (op == nullptr || op->hcclBigOpDesc == nullptr)
        {
            ERROR("DumpHcclOps: Empty op or desc");
            continue;
        }
        AddHcclOpDumpData(data, op);
    }
    if (!hcclOpDBRunner.InsertData("HCCLOP", data))
    {
        ERROR("DumpHcclOps: Insert into db failed");
        result_ = false;
    }
}

void CANNTraceDBDumper::DumpHostTasks(const HostTasks &hostTasks)
{
    if (hostTasks.empty())
    {
        INFO("Empty host tasks");
        return;
    }
    Utils::TimeLogger t{"Dump host tasks"};
    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    if (!hostTaskDBRunner.CreateTable("HostTask", runtimeDB.GetTableCols("HostTask")))
    {
        result_ = false;
        ERROR("DumpHostTasks: create table HostTask failed");
        return;
    }
    HostTasksDumpData data;
    if (!Utils::Reserve(data, hostTasks.size()))
    {
        result_ = false;
        return;
    }

    for (const auto &task : hostTasks)
    {
        auto taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, task->taskType);
        auto kernelName = task->kernelName == 0 ? NA : HashData::GetInstance().Get(task->kernelName);
        data.emplace_back(task->modelId, task->requestId, task->streamId, task->taskId, std::to_string(task->contextId),
                          task->batchId, taskType, kernelName, task->deviceId, std::to_string(task->timeStamp),
                          task->connection_id, task->thread_id);
    }
    if (!hostTaskDBRunner.InsertData("HostTask", data))
    {
        result_ = false;
        ERROR("DumpHostTasks: Insert into db failed");
    }
}

void CANNTraceDBDumper::DumpOpDesc(const HostTasks &computeTasks)
{
    if (computeTasks.empty())
    {
        INFO("Empty kernel tasks");
        return;
    }
    Utils::TimeLogger t{"DumpOpDesc Start"};
    GEInfoDB geInfoDb;
    std::string opDescDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", geInfoDb.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    if (!opDescDBRunner.CreateTable("TaskInfo", geInfoDb.GetTableCols("TaskInfo")))
    {
        result_ = false;
        ERROR("DumpOpDesc: Create table TaskInfo failed");
        return;
    }
    TaskInfoData data;
    if (!Utils::Reserve(data, computeTasks.size()))
    {
        result_ = false;
        return;
    }
    bool isLevel0 = Context::GetInstance().IsLevel0(Utils::File::PathJoin({hostFilePath_, ".."}));
    for (const auto &task : computeTasks)
    {
        if (task)
        {
            AddTaskInfo(task, data, isLevel0);
        }
    }
    if (!opDescDBRunner.InsertData("TaskInfo", data))
    {
        result_ = false;
        ERROR("DumpOpDesc: Insert into db failed");
    }
}

void CANNTraceDBDumper::AddTensorShapeInfo(const std::shared_ptr<ConcatTensorInfo> &tensorDesc,
                                           MsprofNodeBasicInfo nodeBasicInfo, TaskInfoData &data,
                                           const std::shared_ptr<HostTask> &task)
{
    auto tensorNum = tensorDesc->tensorNum;
    std::vector<std::string> inputFormat;
    std::vector<std::string> inputDataType;
    std::vector<std::string> inputShape;
    std::vector<std::string> outputFormat;
    std::vector<std::string> outputDataType;
    std::vector<std::string> outputShape;
    for (uint32_t i = 0; i < tensorNum; i++)
    {
        std::vector<std::string> shapes;
        auto tensorData = tensorDesc->tensorData[i];
        for (const auto shape : tensorData.shape)
        {
            if (shape == 0)
            {
                break;
            }
            shapes.emplace_back(std::to_string(shape));
        }
        if (tensorData.tensorType == INPUT_FORMAT_INDEX)
        {
            inputFormat.emplace_back(GetFormat(tensorData.format));
            inputDataType.emplace_back(NumberMapping::Get(MappingType::GE_DATA_TYPE, tensorData.dataType));
            inputShape.emplace_back(Utils::Join(shapes, ","));
        }
        else if (tensorData.tensorType == OUTPUT_FORMAT_INDEX)
        {
            outputFormat.emplace_back(GetFormat(tensorData.format));
            outputDataType.emplace_back(NumberMapping::Get(MappingType::GE_DATA_TYPE, tensorData.dataType));
            outputShape.emplace_back(Utils::Join(shapes, ","));
        }
    }
    auto desc = task->op->opDesc;
    auto attr = desc->nodeAttr;
    auto hashId = attr ? std::to_string(attr->data.nodeAttrInfo.hashId) : NA;
    uint32_t blockNum = nodeBasicInfo.blockNum & 0xffff;
    auto mixBlockNum = blockNum * (nodeBasicInfo.blockNum >> 16);
    auto opFlag = nodeBasicInfo.opFlag ? "YES" : "NO";
    auto opState = std::to_string(nodeBasicInfo.opState);
    auto runtimeTrackDesc = desc->runtimeTrackDesc;
    std::string gridDim = NA;
    std::string blockDim = NA;
    ProcessRuntimeTrackInfo(runtimeTrackDesc, blockNum, mixBlockNum, gridDim, blockDim);
    auto inputFormatStr = inputFormat.empty() ? NA : Utils::Join(inputFormat, ";");
    auto inputDataTypeStr = inputDataType.empty() ? NA : Utils::Join(inputDataType, ";");
    auto inputShapeStr = inputShape.empty() ? NA : Utils::AddQuotation(Utils::Join(inputShape, ";"));
    auto outputFormatStr = outputFormat.empty() ? NA : Utils::Join(outputFormat, ";");
    auto outputDataTypeStr = outputDataType.empty() ? NA : Utils::Join(outputDataType, ";");
    auto outputShapeStr = outputShape.empty() ? NA : Utils::AddQuotation(Utils::Join(outputShape, ";"));
    data.emplace_back(
        task->modelId, HashData::GetInstance().Get(nodeBasicInfo.opName), task->streamId, task->taskId, blockNum,
        mixBlockNum, opState, NumberMapping::Get(MappingType::GE_TASK_TYPE, nodeBasicInfo.taskType),
        HashData::GetInstance().Get(nodeBasicInfo.opType), task->requestId, task->thread_id, task->timeStamp,
        task->batchId, tensorNum, inputFormatStr, inputDataTypeStr, inputShapeStr, outputFormatStr, outputDataTypeStr,
        outputShapeStr, task->deviceId, task->contextId, opFlag, hashId, gridDim, blockDim);
}

std::string CANNTraceDBDumper::GetFormat(uint32_t oriFormat)
{
    // 若format是默认值(UINT32_MAX) 直接使用枚举UNDEFINED即可 无需换算subFormat
    auto format = (oriFormat == UINT32_MAX) ? UINT32_MAX : (oriFormat & 0xff);
    auto subFormat = (oriFormat == UINT32_MAX) ? 0 : (oriFormat & 0xffff00) >> 8;
    std::string enumFormat = NumberMapping::Get(MappingType::GE_FORMAT, format);
    std::string enumSubFormat = std::to_string(subFormat);
    std::vector<std::string> vec{enumFormat, enumSubFormat};
    if (subFormat > 0)
    {
        enumFormat = Utils::Join(vec, ":");
    }
    return enumFormat;
}

void CANNTraceDBDumper::AddTaskInfoForOnlyTaskTrack(const std::shared_ptr<HostTask> &task, TaskInfoData &data,
                                                    bool isLevel0)
{
    // treeAnalyze中通过白名单控制,此时无kernelName算子为非预期算子
    auto info = RTAddInfoCenter::GetInstance().Get(task->deviceId, task->streamId, task->taskId);
    if (task->kernelName == 0 && !info.isValid)
    {
        WARN("Can't get task's kernel name, streamId is %, taskId is %, batch id is %, timestamp is %.", task->streamId,
             task->taskId, task->batchId, task->timeStamp);
        return;
    }

    std::string taskType;
    std::string opType;
    std::string opName;
    // opFlag在level0 或者无补充信息时填NA
    auto opFlag = (isLevel0 || !info.isValid) ? NA : (info.opFlag ? "YES" : "NO");
    if (info.isValid)
    {
        taskType = isLevel0 ? NA : info.taskType;
        opType = isLevel0 ? NA : info.opType;
        opName = info.opName;
    }
    else
    {
        std::string kernelName = HashData::GetInstance().Get(task->kernelName);
        taskType = isLevel0 ? NA : TransTaskTypeFromRtsToGe(task->taskType);
        opType = isLevel0 ? NA : kernelName;
        opName = kernelName;
    }
    std::string gridDim = NA;
    std::string blockDim = NA;
    auto runtimeTrackDesc = task->op->opDesc->runtimeTrackDesc;
    if (isLevel0)
    {
        uint32_t blockNum = 0;
        uint32_t mixBlockNum = 0;
        ProcessRuntimeTrackInfo(runtimeTrackDesc, blockNum, mixBlockNum, gridDim, blockDim);
        data.emplace_back(info.modelId, opName, task->streamId, task->taskId, blockNum, mixBlockNum, NA, taskType,
                          opType, task->requestId, task->thread_id, task->timeStamp, task->batchId, 0, NA, NA, NA, NA,
                          NA, NA, task->deviceId, task->contextId, opFlag, info.hashId, NA, NA);
    }
    else
    {
        uint32_t blockNum = info.blockNum;
        uint32_t mixBlockNum = info.mixBlockNum;
        ProcessRuntimeTrackInfo(runtimeTrackDesc, blockNum, mixBlockNum, gridDim, blockDim);
        data.emplace_back(info.modelId, opName, task->streamId, task->taskId, blockNum, mixBlockNum, info.isDynamic,
                          taskType, opType, task->requestId, task->thread_id, task->timeStamp, task->batchId,
                          info.tensorNum, info.inputFormats, info.inputDataTypes, info.inputShapes, info.outputFormats,
                          info.outputDataTypes, info.outputShapes, task->deviceId, task->contextId, opFlag, info.hashId,
                          gridDim, blockDim);
    }
}

void CANNTraceDBDumper::ProcessRuntimeTrackInfo(const std::shared_ptr<MsprofCompactInfo> &runtimeTrack,
                                                uint32_t &blockNum, uint32_t &mixBlockNum, std::string &gridDim,
                                                std::string &blockDim)
{
    if (!runtimeTrack || runtimeTrack->dataLen != MSPROF_COMPACT_INFO_DATA_LENGTH)
    {
        return;
    }
    auto taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, runtimeTrack->data.runtimeTrack.taskType);
    if (taskType == KERNEL_SIMT_TASK_TYPE)
    {
        auto &simtInfo = runtimeTrack->data.runtimeTrack.extInfo.simtKernelInfo;
        gridDim =
            Utils::Join(std::vector<std::string>{std::to_string(simtInfo.gridDim.x), std::to_string(simtInfo.gridDim.y),
                                                 std::to_string(simtInfo.gridDim.z)},
                        ",");
        blockDim = Utils::Join(
            std::vector<std::string>{std::to_string(simtInfo.blockDim.x), std::to_string(simtInfo.blockDim.y),
                                     std::to_string(simtInfo.blockDim.z)},
            ",");
    }
    else
    {
        auto ratio = runtimeTrack->data.runtimeTrack.extInfo.kernelInfo.ratio;
        auto numBlocks = runtimeTrack->data.runtimeTrack.extInfo.kernelInfo.numBlocks;
        blockNum = numBlocks;
        mixBlockNum = numBlocks * ratio;
    }
}

void CANNTraceDBDumper::AddTaskInfo(const std::shared_ptr<HostTask> &task, TaskInfoData &data, bool isLevel0)
{
    if (!task->op || !task->op->opDesc)
    {
        return;
    }
    if (task->op->type == OpType::OPTYPE_RESERVED)
    {
        AddTaskInfoForOnlyTaskTrack(task, data, isLevel0);
        return;
    }
    if (isLevel0)
    {
        uint32_t blockNum = 0;
        uint32_t mixBlockNum = 0;
        std::string gridDim = NA;
        std::string blockDim = NA;
        auto name = HashData::GetInstance().Get(task->op->name);
        ProcessRuntimeTrackInfo(task->op->opDesc->runtimeTrackDesc, blockNum, mixBlockNum, gridDim, blockDim);
        data.emplace_back(task->modelId, name, task->streamId, task->taskId, blockNum, mixBlockNum, NA, NA, NA,
                          task->requestId, task->thread_id, task->timeStamp, task->batchId, 0, NA, NA, NA, NA, NA, NA,
                          task->deviceId, task->contextId, NA, NA, gridDim, blockDim);
        return;
    }

    auto desc = task->op->opDesc;
    if (!desc or !desc->nodeDesc)
    {
        ERROR("Can't find node desc for api: %, timestamp is %", task->kernelName, task->timeStamp);
        return;
    }
    auto node = desc->nodeDesc;
    auto attr = desc->nodeAttr;
    auto nodeBasicInfo = node->data.nodeBasicInfo;
    auto tensorDesc = desc->tensorDesc;
    if (!tensorDesc)
    {
        auto hashId = attr ? std::to_string(attr->data.nodeAttrInfo.hashId) : NA;
        auto blockNum = nodeBasicInfo.blockNum & 0xffff;
        auto mixBlockNum = blockNum * (nodeBasicInfo.blockNum >> 16);
        auto opFlag = nodeBasicInfo.opFlag ? "YES" : "NO";
        auto opState = std::to_string(nodeBasicInfo.opState);
        auto runtimeTrackDesc = desc->runtimeTrackDesc;
        std::string gridDim = NA;
        std::string blockDim = NA;
        ProcessRuntimeTrackInfo(runtimeTrackDesc, blockNum, mixBlockNum, gridDim, blockDim);
        data.emplace_back(task->modelId, HashData::GetInstance().Get(nodeBasicInfo.opName), task->streamId,
                          task->taskId, blockNum, mixBlockNum, opState,
                          NumberMapping::Get(MappingType::GE_TASK_TYPE, nodeBasicInfo.taskType),
                          HashData::GetInstance().Get(nodeBasicInfo.opType), task->requestId, task->thread_id,
                          task->timeStamp, task->batchId, 0, NA, NA, NA, NA, NA, NA, task->deviceId, task->contextId,
                          opFlag, hashId, gridDim, blockDim);
        return;
    }
    AddTensorShapeInfo(tensorDesc, nodeBasicInfo, data, task);
}

void CANNTraceDBDumper::DumpHcclTasks(const HostTasks &hcclTasks)
{
    if (hcclTasks.empty())
    {
        INFO("Empty hccl tasks");
        return;
    }
    Utils::TimeLogger t{"DumpHcclTasks start"};
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({hostFilePath_, "sqlite", hcclDB.GetDBName()});
    DBRunner hcclTaskDBRunner(hcclTaskDBPath);
    if (!hcclTaskDBRunner.CreateTable("HCCLTask", hcclDB.GetTableCols("HCCLTask")))
    {
        result_ = false;
        ERROR("DumpHcclTasks: create table HCCLTask failed");
        return;
    }
    HcclTasksDumpData data;
    if (!Utils::Reserve(data, hcclTasks.size()))
    {
        result_ = false;
        return;
    }
    for (const auto &task : hcclTasks)
    {
        if (!task || !task->op || !task->op->hcclSmallOpDesc)
        {
            ERROR("Empty op or desc");
            continue;
        }
        auto desc = task->op->hcclSmallOpDesc;
        if (!desc->hcclInfo || !desc->hcclInfo->data)
        {
            data.emplace_back(task->modelId, task->requestId, NA, NA, INVALID_VALUE, std::to_string(task->timeStamp),
                              INVALID_VALUE, task->streamId, task->taskId, task->contextId, task->batchId,
                              task->deviceId, desc->isMaster, INVALID_VALUE, INVALID_VALUE, NA, INVALID_VALUE, NA, NA,
                              NA, NA, task->thread_id, 0);
            continue;
        }
        auto hcclTrace = Utils::ReinterpretConvert<MsprofHcclInfo *>(desc->hcclInfo->data);
        data.emplace_back(
            task->modelId, task->requestId, HashData::GetInstance().Get(hcclTrace->itemId),
            std::to_string(hcclTrace->groupName), hcclTrace->planeID, std::to_string(task->timeStamp),
            hcclTrace->durationEstimated, task->streamId, task->taskId, task->contextId, task->batchId, task->deviceId,
            desc->isMaster, hcclTrace->localRank, hcclTrace->remoteRank,
            NumberMapping::Get(MappingType::HCCL_TRANSPORT_TYPE, hcclTrace->transportType),
            static_cast<double>(hcclTrace->dataSize),
            NumberMapping::Get(MappingType::HCCL_DATA_TYPE, hcclTrace->dataType),
            NumberMapping::Get(MappingType::HCCL_LINK_TYPE, hcclTrace->linkType), std::to_string(hcclTrace->notifyID),
            NumberMapping::Get(MappingType::HCCL_RDMA_TYPE, hcclTrace->rdmaType), task->thread_id, hcclTrace->rankSize);
    }
    if (!hcclTaskDBRunner.InsertData("HCCLTask", data))
    {
        result_ = false;
        ERROR("DumpHcclTasks: Insert into db failed");
    }
}

void CANNTraceDBDumper::DumpGeFusionOps(const GeFusionOpInfos &geFusionOps)
{
    if (geFusionOps.empty())
    {
        INFO("Empty geFusionOps");
        return;
    }
    Utils::TimeLogger t{"DumpGeFusionOps start"};
    GeModelInfoDB geModelInfoDb;
    std::string geModelInfoDbPath = Utils::File::PathJoin({hostFilePath_, "sqlite", geModelInfoDb.GetDBName()});
    DBRunner geFusionOpDBRunner(geModelInfoDbPath);
    if (!geFusionOpDBRunner.CreateTable("GeFusionOpInfo", geModelInfoDb.GetTableCols("GeFusionOpInfo")))
    {
        result_ = false;
        ERROR("DumpGeFusionOps: create table GeFusionOpInfo failed");
        return;
    }
    GeFusionOpsDumpData data;
    if (!Utils::Reserve(data, geFusionOps.size()))
    {
        result_ = false;
        return;
    }
    for (const auto &geFusionOp : geFusionOps)
    {
        auto fusionStruct = geFusionOp->fusionOpInfo;
        std::vector<std::string> fusionOpNames;
        if (!Utils::Reserve(fusionOpNames, fusionStruct->fusionOpNum))
        {
            break;
        }
        for (uint32_t i = 0; i < fusionStruct->fusionOpNum; i++)
        {
            fusionOpNames.emplace_back(HashData::GetInstance().Get(fusionStruct->fusionOpId[i]));
        }
        data.emplace_back(geFusionOp->modelId, HashData::GetInstance().Get(fusionStruct->opName),
                          fusionStruct->fusionOpNum, Utils::Join(fusionOpNames, ","),
                          std::to_string(fusionStruct->inputMemsize), std::to_string(fusionStruct->outputMemsize),
                          std::to_string(fusionStruct->weightMemSize), std::to_string(fusionStruct->workspaceMemSize),
                          std::to_string(fusionStruct->totalMemSize));
    }
    if (!geFusionOpDBRunner.InsertData("GeFusionOpInfo", data))
    {
        result_ = false;
        ERROR("DumpGeFusionOps: Insert into db failed");
    }
}
}  // namespace Domain
}  // namespace Analysis
