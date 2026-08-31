/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"

#include <string>

#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
namespace Adapter
{
using namespace Analysis::Utils;
namespace
{
void AdapterRuntimeTrackV1(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.runtimeTrack.deviceId = compact->data.runtimeTrack.deviceId;
    parsed->data.runtimeTrack.streamId = compact->data.runtimeTrack.streamId;
    parsed->data.runtimeTrack.taskId = compact->data.runtimeTrack.taskId;
    parsed->data.runtimeTrack.taskType = compact->data.runtimeTrack.taskType;
    parsed->data.runtimeTrack.kernelName = compact->data.runtimeTrack.kernelName;
    parsed->data.runtimeTrack.kernelInfo.argsSize = compact->data.runtimeTrack.extInfo.kernelInfo.argsSize;
    parsed->data.runtimeTrack.kernelInfo.numBlocks = compact->data.runtimeTrack.extInfo.kernelInfo.numBlocks;
    parsed->data.runtimeTrack.kernelInfo.ratio = compact->data.runtimeTrack.extInfo.kernelInfo.ratio;
    parsed->data.runtimeTrack.kernelInfo.schedMode = compact->data.runtimeTrack.extInfo.kernelInfo.schedMode;
    parsed->data.runtimeTrack.simtKernelInfo.argsSize = compact->data.runtimeTrack.extInfo.simtKernelInfo.argsSize;
    parsed->data.runtimeTrack.simtKernelInfo.blockDim = compact->data.runtimeTrack.extInfo.simtKernelInfo.blockDim;
    parsed->data.runtimeTrack.simtKernelInfo.gridDim = compact->data.runtimeTrack.extInfo.simtKernelInfo.gridDim;
    parsed->data.runtimeTrack.simtKernelInfo.schedMode = compact->data.runtimeTrack.extInfo.simtKernelInfo.schedMode;
    parsed->data.runtimeTrack.modelInfo.modelId = compact->data.runtimeTrack.extInfo.modelInfo.modelId;
    parsed->data.runtimeTrack.eventInfo.key = compact->data.runtimeTrack.extInfo.eventInfo.key;
    parsed->data.runtimeTrack.notifyInfo.key = compact->data.runtimeTrack.extInfo.notifyInfo.key;
}

void AdapterRuntimeTrackV2(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.runtimeTrack.deviceId = compact->data.runtimeTrackV2.deviceId;
    parsed->data.runtimeTrack.streamId = compact->data.runtimeTrackV2.streamId;
    parsed->data.runtimeTrack.taskId = compact->data.runtimeTrackV2.taskId;
    parsed->data.runtimeTrack.taskType = compact->data.runtimeTrackV2.taskType;
    parsed->data.runtimeTrack.kernelName = compact->data.runtimeTrackV2.kernelName;
    parsed->data.runtimeTrack.kernelInfo.argsSize = compact->data.runtimeTrackV2.extInfo.kernelInfo.argsSize;
    parsed->data.runtimeTrack.kernelInfo.numBlocks = compact->data.runtimeTrackV2.extInfo.kernelInfo.numBlocks;
    parsed->data.runtimeTrack.kernelInfo.ratio = compact->data.runtimeTrackV2.extInfo.kernelInfo.ratio;
    parsed->data.runtimeTrack.kernelInfo.schedMode = compact->data.runtimeTrackV2.extInfo.kernelInfo.schedMode;
    parsed->data.runtimeTrack.simtKernelInfo.argsSize = compact->data.runtimeTrackV2.extInfo.simtKernelInfo.argsSize;
    parsed->data.runtimeTrack.simtKernelInfo.blockDim = compact->data.runtimeTrackV2.extInfo.simtKernelInfo.blockDim;
    parsed->data.runtimeTrack.simtKernelInfo.gridDim = compact->data.runtimeTrackV2.extInfo.simtKernelInfo.gridDim;
    parsed->data.runtimeTrack.simtKernelInfo.schedMode = compact->data.runtimeTrackV2.extInfo.simtKernelInfo.schedMode;
    parsed->data.runtimeTrack.modelInfo.modelId = compact->data.runtimeTrackV2.extInfo.modelInfo.modelId;
    parsed->data.runtimeTrack.eventInfo.key = compact->data.runtimeTrackV2.extInfo.eventInfo.key;
    parsed->data.runtimeTrack.notifyInfo.key = compact->data.runtimeTrackV2.extInfo.notifyInfo.key;
}
}  // namespace

bool ParserCompactInfoAdapter::AdapterCompactInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed,
                                                  CompactInfoFormat parserType)
{
    if (compact == nullptr || parsed == nullptr)
    {
        ERROR("adapter compactInfo data failed.");
        return false;
    }
    parsed->magicNumber = compact->magicNumber;
    parsed->level = compact->level;
    parsed->type = compact->type;
    parsed->threadId = compact->threadId;
    parsed->dataLen = compact->dataLen;
    parsed->timeStamp = compact->timeStamp;
    switch (parserType)
    {
        case CompactInfoFormat::DPU_TRACK_TYPE:
            AdapterDpuTrack(compact, parsed);
            return true;
        case CompactInfoFormat::NODE_BASIC_INFO_TYPE:
            AdapterNodeBasicInfo(compact, parsed);
            return true;
        case CompactInfoFormat::ATTR_INFO_TYPE:
            AdapterAttrInfo(compact, parsed);
            return true;
        case CompactInfoFormat::HCCL_OP_INFO_TYPE:
            AdapterHcclopInfo(compact, parsed);
            return true;
        case CompactInfoFormat::MEMCPY_INFO_TYPE:
            AdapterMemcpyInfo(compact, parsed);
            return true;
        default:
            ERROR("Unsupported CompactInfo format: %.", static_cast<uint32_t>(parserType));
            return false;
    }
}

bool ParserCompactInfoAdapter::AdapterRuntimeTrack(const MsprofCompactInfo* compact, RuntimeTrackFormat format,
                                                   ParserCompactInfo* parsed)
{
    if (compact == nullptr || parsed == nullptr)
    {
        ERROR("adapter runtimeTrack data failed.");
        return false;
    }
    parsed->magicNumber = compact->magicNumber;
    parsed->level = compact->level;
    parsed->type = compact->type;
    parsed->threadId = compact->threadId;
    parsed->dataLen = compact->dataLen;
    parsed->timeStamp = compact->timeStamp;
    switch (format)
    {
        case RuntimeTrackFormat::V1:
            AdapterRuntimeTrackV1(compact, parsed);
            return true;
        case RuntimeTrackFormat::V2:
            AdapterRuntimeTrackV2(compact, parsed);
            return true;
        default:
            ERROR("Unsupported runtime track format: %.", static_cast<uint32_t>(format));
            return false;
    }
}

bool ParserCompactInfoAdapter::AdapterCaptureStreamInfo(const MsprofCompactInfo* compact, CaptureStreamFormat format,
                                                        ParserCompactInfo* parsed)
{
    if (compact == nullptr || parsed == nullptr)
    {
        ERROR("adapter captureStreamInfo data failed.");
        return false;
    }
    parsed->magicNumber = compact->magicNumber;
    parsed->level = compact->level;
    parsed->type = compact->type;
    parsed->threadId = compact->threadId;
    parsed->dataLen = compact->dataLen;
    parsed->timeStamp = compact->timeStamp;
    switch (format)
    {
        case CaptureStreamFormat::V1:
            parsed->data.captureStreamInfo.deviceId = compact->data.captureStreamInfo.deviceId;
            parsed->data.captureStreamInfo.captureStatus = compact->data.captureStreamInfo.captureStatus;
            parsed->data.captureStreamInfo.streamId = compact->data.captureStreamInfo.modelStreamId;
            parsed->data.captureStreamInfo.originalStreamId = compact->data.captureStreamInfo.originalStreamId;
            parsed->data.captureStreamInfo.modelId = compact->data.captureStreamInfo.modelId;
            return true;
        case CaptureStreamFormat::V2:
            parsed->data.captureStreamInfo.deviceId = compact->data.captureStreamInfoV2.deviceId;
            parsed->data.captureStreamInfo.captureStatus = compact->data.captureStreamInfoV2.captureStatus;
            parsed->data.captureStreamInfo.streamId = compact->data.captureStreamInfoV2.streamId;
            parsed->data.captureStreamInfo.originalStreamId = compact->data.captureStreamInfoV2.originalStreamId;
            parsed->data.captureStreamInfo.modelId = compact->data.captureStreamInfoV2.modelId;
            return true;
        default:
            ERROR("Unsupported capture stream format: %.", static_cast<uint32_t>(format));
            return false;
    }
}

void ParserCompactInfoAdapter::AdapterDpuTrack(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.dpuTrack.deviceId = compact->data.dpuTrack.deviceId;
    parsed->data.dpuTrack.streamId = compact->data.dpuTrack.streamId;
    parsed->data.dpuTrack.taskId = compact->data.dpuTrack.taskId;
    parsed->data.dpuTrack.taskType = compact->data.dpuTrack.taskType;
    parsed->data.dpuTrack.res = compact->data.dpuTrack.res;
    parsed->data.dpuTrack.startTime = compact->data.dpuTrack.startTime;
}

void ParserCompactInfoAdapter::AdapterNodeBasicInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.nodeBasicInfo.opName = compact->data.nodeBasicInfo.opName;
    parsed->data.nodeBasicInfo.taskType = compact->data.nodeBasicInfo.taskType;
    parsed->data.nodeBasicInfo.opType = compact->data.nodeBasicInfo.opType;
    parsed->data.nodeBasicInfo.blockNum = compact->data.nodeBasicInfo.blockNum;
    parsed->data.nodeBasicInfo.opFlag = compact->data.nodeBasicInfo.opFlag;
    parsed->data.nodeBasicInfo.opState = compact->data.nodeBasicInfo.opState;
}

void ParserCompactInfoAdapter::AdapterAttrInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.nodeAttrInfo.opName = compact->data.nodeAttrInfo.opName;
    parsed->data.nodeAttrInfo.attrType = compact->data.nodeAttrInfo.attrType;
    parsed->data.nodeAttrInfo.hashId = compact->data.nodeAttrInfo.hashId;
}

void ParserCompactInfoAdapter::AdapterHcclopInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.hcclopInfo.relay = compact->data.hcclopInfo.relay;
    parsed->data.hcclopInfo.retry = compact->data.hcclopInfo.retry;
    parsed->data.hcclopInfo.dataType = compact->data.hcclopInfo.dataType;
    parsed->data.hcclopInfo.algType = compact->data.hcclopInfo.algType;
    parsed->data.hcclopInfo.count = compact->data.hcclopInfo.count;
    parsed->data.hcclopInfo.groupName = compact->data.hcclopInfo.groupName;
}

void ParserCompactInfoAdapter::AdapterMemcpyInfo(const MsprofCompactInfo* compact, ParserCompactInfo* parsed)
{
    parsed->data.memcpyInfo.dataSize = compact->data.memcpyInfo.dataSize;
    parsed->data.memcpyInfo.maxSize = compact->data.memcpyInfo.maxSize;
    parsed->data.memcpyInfo.memcpyDirection = compact->data.memcpyInfo.memcpyDirection;
}

bool ParserApiAdapter::AdapterApi(const MsprofApi* apiData, ParserApi* parsed)
{
    if (apiData == nullptr || parsed == nullptr)
    {
        ERROR("adapter api data failed.");
        return false;
    }
    parsed->magicNumber = apiData->magicNumber;
    parsed->level = apiData->level;
    parsed->itemId = apiData->itemId;
    parsed->endTime = apiData->endTime;
    parsed->beginTime = apiData->beginTime;
    parsed->threadId = apiData->threadId;
    parsed->type = apiData->type;
    return true;
}

bool ParserAdditionalInfoAdapter::AdapterAdditionalInfo(MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed,
                                                        AdditionalInfoFormat parserType)
{
    if (addition == nullptr || parsed == nullptr)
    {
        ERROR("adapter additionalInfo data failed.");
        return false;
    }
    parsed->magicNumber = addition->magicNumber;
    parsed->level = addition->level;
    parsed->type = addition->type;
    parsed->threadId = addition->threadId;
    parsed->dataLen = addition->dataLen;
    parsed->timeStamp = addition->timeStamp;
    switch (parserType)
    {
        case AdditionalInfoFormat::CONTEXT_ID_INFO_TYPE:
            AdapterContextIdInfo(addition, parsed);
            return true;
        case AdditionalInfoFormat::FUSION_OP_INFO_TYPE:
            AdapterFusionOpInfo(addition, parsed);
            return true;
        case AdditionalInfoFormat::GRAPH_ID_TYPE:
            return AdapterGraphId(addition, parsed);
        case AdditionalInfoFormat::HCCL_INFO_TYPE:
            AdapterHcclInfo(addition, parsed);
            return true;
        case AdditionalInfoFormat::MULTI_THREAD_TYPE:
            AdapterMultiThread(addition, parsed);
            return true;
        case AdditionalInfoFormat::TASK_MEMORY_INFO_TYPE:
            AdapterMemoryInfo(addition, parsed);
            return true;
        default:
            ERROR("Unsupported Additional Info: %.", static_cast<uint32_t>(parserType));
            return false;
    }
}

void ParserAdditionalInfoAdapter::AdapterContextIdInfo(const MsprofAdditionalInfo* addition,
                                                       ParserAdditionalInfo* parsed)
{
    parsed->contextIdInfo.opName = addition->contextIdInfo.opName;
    parsed->contextIdInfo.ctxIdNum = addition->contextIdInfo.ctxIdNum;
    uint32_t ctxIdNum = parsed->contextIdInfo.ctxIdNum;
    if (ctxIdNum > MSPROF_CTX_ID_MAX_NUM)
    {
        ctxIdNum = MSPROF_CTX_ID_MAX_NUM;
    }
    for (uint32_t i = 0; i < ctxIdNum; i++)
    {
        parsed->contextIdInfo.ctxIds[i] = addition->contextIdInfo.ctxIds[i];
    }
}

void ParserAdditionalInfoAdapter::AdapterFusionOpInfo(const MsprofAdditionalInfo* addition,
                                                      ParserAdditionalInfo* parsed)
{
    parsed->fusionOpInfo.opName = addition->fusionOpInfo.opName;
    parsed->fusionOpInfo.fusionOpNum = addition->fusionOpInfo.fusionOpNum;
    parsed->fusionOpInfo.inputMemsize = addition->fusionOpInfo.inputMemsize;
    parsed->fusionOpInfo.outputMemsize = addition->fusionOpInfo.outputMemsize;
    parsed->fusionOpInfo.weightMemSize = addition->fusionOpInfo.weightMemSize;
    parsed->fusionOpInfo.workspaceMemSize = addition->fusionOpInfo.workspaceMemSize;
    parsed->fusionOpInfo.totalMemSize = addition->fusionOpInfo.totalMemSize;
    uint32_t opNum = parsed->fusionOpInfo.fusionOpNum;
    if (opNum > MSPROF_GE_FUSION_OP_NUM)
    {
        opNum = MSPROF_GE_FUSION_OP_NUM;
    }
    for (uint32_t i = 0; i < opNum; i++)
    {
        parsed->fusionOpInfo.fusionOpId[i] = addition->fusionOpInfo.fusionOpId[i];
    }
}

bool ParserAdditionalInfoAdapter::AdapterGraphId(MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed)
{
    if (addition->data == nullptr)
    {
        ERROR("covert Graph id info data failed.");
        return false;
    }
    auto curr = ReinterpretConvert<MsprofGraphIdInfo*>(addition->data);
    parsed->graphIdInfo.modelName = curr->modelName;
    parsed->graphIdInfo.graphId = curr->graphId;
    parsed->graphIdInfo.modelId = curr->modelId;
    return true;
}

void ParserAdditionalInfoAdapter::AdapterHcclInfo(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed)
{
    parsed->hcclInfo.itemId = addition->hcclInfo.itemId;
    parsed->hcclInfo.cclTag = addition->hcclInfo.cclTag;
    parsed->hcclInfo.groupName = addition->hcclInfo.groupName;
    parsed->hcclInfo.localRank = addition->hcclInfo.localRank;
    parsed->hcclInfo.remoteRank = addition->hcclInfo.remoteRank;
    parsed->hcclInfo.rankSize = addition->hcclInfo.rankSize;
    parsed->hcclInfo.workFlowMode = addition->hcclInfo.workFlowMode;
    parsed->hcclInfo.planeID = addition->hcclInfo.planeID;
    parsed->hcclInfo.ctxID = addition->hcclInfo.ctxID;
    parsed->hcclInfo.notifyID = addition->hcclInfo.notifyID;
    parsed->hcclInfo.stage = addition->hcclInfo.stage;
    parsed->hcclInfo.role = addition->hcclInfo.role;
    parsed->hcclInfo.durationEstimated = addition->hcclInfo.durationEstimated;
    parsed->hcclInfo.srcAddr = addition->hcclInfo.srcAddr;
    parsed->hcclInfo.dstAddr = addition->hcclInfo.dstAddr;
    parsed->hcclInfo.dataSize = addition->hcclInfo.dataSize;
    parsed->hcclInfo.opType = addition->hcclInfo.opType;
    parsed->hcclInfo.dataType = addition->hcclInfo.dataType;
    parsed->hcclInfo.linkType = addition->hcclInfo.linkType;
    parsed->hcclInfo.transportType = addition->hcclInfo.transportType;
    parsed->hcclInfo.rdmaType = addition->hcclInfo.rdmaType;
    parsed->hcclInfo.reserve2 = addition->hcclInfo.reserve2;
}

void ParserAdditionalInfoAdapter::AdapterMultiThread(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed)
{
    parsed->multiThread.threadNum = addition->multiThread.threadNum;
    uint32_t threadNum = parsed->multiThread.threadNum;
    if (threadNum > MSPROF_MULTI_THREAD_MAX_NUM)
    {
        threadNum = MSPROF_MULTI_THREAD_MAX_NUM;
    }
    for (uint32_t i = 0; i < threadNum; i++)
    {
        parsed->multiThread.threadId[i] = addition->multiThread.threadId[i];
    }
}

void ParserAdditionalInfoAdapter::AdapterMemoryInfo(const MsprofAdditionalInfo* addition, ParserAdditionalInfo* parsed)
{
    parsed->memoryInfo.addr = addition->memoryInfo.addr;
    parsed->memoryInfo.size = addition->memoryInfo.size;
    parsed->memoryInfo.nodeId = addition->memoryInfo.nodeId;
    parsed->memoryInfo.totalAllocateMemory = addition->memoryInfo.totalAllocateMemory;
    parsed->memoryInfo.totalReserveMemory = addition->memoryInfo.totalReserveMemory;
    parsed->memoryInfo.deviceId = addition->memoryInfo.deviceId;
    parsed->memoryInfo.deviceType = addition->memoryInfo.deviceType;
}

bool ParserAicpuAdapter::AdapterNode(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter node data failed.");
        return false;
    }
    aicpuData->node.streamId = additionalData->aicpuNode.streamId;
    aicpuData->node.taskId = additionalData->aicpuNode.taskId;
    aicpuData->node.rev = additionalData->aicpuNode.rev;
    aicpuData->node.runStartTime = additionalData->aicpuNode.runStartTime;
    aicpuData->node.runStartTick = additionalData->aicpuNode.runStartTick;
    aicpuData->node.computeStartTime = additionalData->aicpuNode.computeStartTime;
    aicpuData->node.memcpyStartTime = additionalData->aicpuNode.memcpyStartTime;
    aicpuData->node.memcpyEndTime = additionalData->aicpuNode.memcpyEndTime;
    aicpuData->node.runEndTime = additionalData->aicpuNode.runEndTime;
    aicpuData->node.runEndTick = additionalData->aicpuNode.runEndTick;
    aicpuData->node.threadId = additionalData->aicpuNode.threadId;
    aicpuData->node.deviceId = additionalData->aicpuNode.deviceId;
    aicpuData->node.submitTick = additionalData->aicpuNode.submitTick;
    aicpuData->node.scheduleTick = additionalData->aicpuNode.scheduleTick;
    aicpuData->node.tickBeforeRun = additionalData->aicpuNode.tickBeforeRun;
    aicpuData->node.tickAfterRun = additionalData->aicpuNode.tickAfterRun;
    aicpuData->node.kernelType = additionalData->aicpuNode.kernelType;
    aicpuData->node.dispatchTime = additionalData->aicpuNode.dispatchTime;
    aicpuData->node.totalTime = additionalData->aicpuNode.totalTime;
    aicpuData->node.fftsThreadId = additionalData->aicpuNode.fftsThreadId;
    aicpuData->node.version = additionalData->aicpuNode.version;
    return true;
}

bool ParserAicpuAdapter::AdapterModel(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter model data failed.");
        return false;
    }
    aicpuData->model.indexId = additionalData->aicpuModel.indexId;
    aicpuData->model.modelId = additionalData->aicpuModel.modelId;
    aicpuData->model.tagId = additionalData->aicpuModel.tagId;
    aicpuData->model.rsv1 = additionalData->aicpuModel.rsv1;
    aicpuData->model.eventId = additionalData->aicpuModel.eventId;
    return true;
}

bool ParserAicpuAdapter::AdapterDp(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter dp data failed.");
        return false;
    }
    errno_t res = memcpy_s(aicpuData->dp.action, sizeof(aicpuData->dp.action), additionalData->aicpuDp.action,
                           sizeof(additionalData->aicpuDp.action));
    errno_t resOst = memcpy_s(aicpuData->dp.source, sizeof(aicpuData->dp.source), additionalData->aicpuDp.source,
                              sizeof(additionalData->aicpuDp.source));
    if (res != EOK || resOst != EOK)
    {
        ERROR("memcpy aicpuDp data failed!");
        return false;
    }
    aicpuData->dp.index = additionalData->aicpuDp.index;
    aicpuData->dp.size = additionalData->aicpuDp.size;
    return true;
}

bool ParserAicpuAdapter::AdapterMi(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter mi data failed.");
        return false;
    }
    aicpuData->mi.nodeTag = additionalData->aicpuMi.nodeTag;
    aicpuData->mi.reserve = additionalData->aicpuMi.reserve;
    aicpuData->mi.queueSize = additionalData->aicpuMi.queueSize;
    aicpuData->mi.runStartTime = additionalData->aicpuMi.runStartTime;
    aicpuData->mi.runEndTime = additionalData->aicpuMi.runEndTime;
    return true;
}

bool ParserAicpuAdapter::AdapterCommTurn(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter commTurn data failed.");
        return false;
    }
    aicpuData->commTurn.serverStartTime = additionalData->commTurn.serverStartTime;
    aicpuData->commTurn.waitMsgStartTime = additionalData->commTurn.waitMsgStartTime;
    aicpuData->commTurn.kfcAlgExeStartTime = additionalData->commTurn.kfcAlgExeStartTime;
    aicpuData->commTurn.sendTaskStartTime = additionalData->commTurn.sendTaskStartTime;
    aicpuData->commTurn.sendSqeFinishTime = additionalData->commTurn.sendSqeFinishTime;
    aicpuData->commTurn.rtsqExeEndTime = additionalData->commTurn.rtsqExeEndTime;
    aicpuData->commTurn.serverEndTime = additionalData->commTurn.serverEndTime;
    aicpuData->commTurn.dataLen = additionalData->commTurn.dataLen;
    aicpuData->commTurn.deviceId = additionalData->commTurn.deviceId;
    aicpuData->commTurn.streamId = additionalData->commTurn.streamId;
    aicpuData->commTurn.taskId = additionalData->commTurn.taskId;
    aicpuData->commTurn.version = additionalData->commTurn.version;
    aicpuData->commTurn.commTurn = additionalData->commTurn.commTurn;
    aicpuData->commTurn.currentTurn = additionalData->commTurn.currentTurn;
    return true;
}

bool ParserAicpuAdapter::AdapterComputeTurn(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter computeTurn data failed.");
        return false;
    }
    aicpuData->computeTurn.waitComputeStartTime = additionalData->computeTurn.waitComputeStartTime;
    aicpuData->computeTurn.computeStartTime = additionalData->computeTurn.computeStartTime;
    aicpuData->computeTurn.computeExeEndTime = additionalData->computeTurn.computeExeEndTime;
    aicpuData->computeTurn.dataLen = additionalData->computeTurn.dataLen;
    aicpuData->computeTurn.deviceId = additionalData->computeTurn.deviceId;
    aicpuData->computeTurn.streamId = additionalData->computeTurn.streamId;
    aicpuData->computeTurn.taskId = additionalData->computeTurn.taskId;
    aicpuData->computeTurn.version = additionalData->computeTurn.version;
    aicpuData->computeTurn.computeTurn = additionalData->computeTurn.computeTurn;
    aicpuData->computeTurn.currentTurn = additionalData->computeTurn.currentTurn;
    return true;
}

bool ParserAicpuAdapter::AdapterOpInfo(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter opInfo data failed.");
        return false;
    }
    aicpuData->opInfo.relay = additionalData->opInfo.relay;
    aicpuData->opInfo.retry = additionalData->opInfo.retry;
    aicpuData->opInfo.dataType = additionalData->opInfo.dataType;
    aicpuData->opInfo.algType = additionalData->opInfo.algType;
    aicpuData->opInfo.count = additionalData->opInfo.count;
    aicpuData->opInfo.groupName = additionalData->opInfo.groupName;
    aicpuData->opInfo.rankSize = additionalData->opInfo.rankSize;
    aicpuData->opInfo.streamId = additionalData->opInfo.streamId;
    aicpuData->opInfo.taskId = additionalData->opInfo.taskId;
    return true;
}

bool ParserAicpuAdapter::AdapterFlipTask(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter flip task data failed.");
        return false;
    }
    aicpuData->flipTask.streamId = additionalData->flipTask.streamId;
    aicpuData->flipTask.taskId = additionalData->flipTask.taskId;
    aicpuData->flipTask.flipNum = additionalData->flipTask.flipNum;
    return true;
}

bool ParserAicpuAdapter::AdapterMainStreamTask(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter mainStream task data failed.");
        return false;
    }
    aicpuData->mainStreamTask.aicpuStreamId = additionalData->mainStreamTask.aicpuStreamId;
    aicpuData->mainStreamTask.aicpuTaskId = additionalData->mainStreamTask.aicpuTaskId;
    aicpuData->mainStreamTask.streamId = additionalData->mainStreamTask.streamId;
    aicpuData->mainStreamTask.taskId = additionalData->mainStreamTask.taskId;
    aicpuData->mainStreamTask.type = additionalData->mainStreamTask.type;
    return true;
}

bool ParserAicpuAdapter::AdapterKfcInfos(const MsprofAdditionalInfo* additionalData, AicpuData* aicpuData)
{
    if (additionalData == nullptr || aicpuData == nullptr)
    {
        ERROR("adapter KfcInfo data failed.");
        return false;
    }
    for (uint32_t i = 0; i < KFC_INFOS_NUM; i++)
    {
        aicpuData->KfcInfos.infos[i].itemId = additionalData->kfcInfos.infos[i].itemId;
        aicpuData->KfcInfos.infos[i].cclTag = additionalData->kfcInfos.infos[i].cclTag;
        aicpuData->KfcInfos.infos[i].groupName = additionalData->kfcInfos.infos[i].groupName;
        aicpuData->KfcInfos.infos[i].localRank = additionalData->kfcInfos.infos[i].localRank;
        aicpuData->KfcInfos.infos[i].remoteRank = additionalData->kfcInfos.infos[i].remoteRank;
        aicpuData->KfcInfos.infos[i].rankSize = additionalData->kfcInfos.infos[i].rankSize;
        aicpuData->KfcInfos.infos[i].stage = additionalData->kfcInfos.infos[i].stage;
        aicpuData->KfcInfos.infos[i].notifyID = additionalData->kfcInfos.infos[i].notifyID;
        aicpuData->KfcInfos.infos[i].timeStamp = additionalData->kfcInfos.infos[i].timeStamp;
        aicpuData->KfcInfos.infos[i].durationEstimated = additionalData->kfcInfos.infos[i].durationEstimated;
        aicpuData->KfcInfos.infos[i].srcAddr = additionalData->kfcInfos.infos[i].srcAddr;
        aicpuData->KfcInfos.infos[i].dstAddr = additionalData->kfcInfos.infos[i].dstAddr;
        aicpuData->KfcInfos.infos[i].dataSize = additionalData->kfcInfos.infos[i].dataSize;
        aicpuData->KfcInfos.infos[i].taskId = additionalData->kfcInfos.infos[i].taskId;
        aicpuData->KfcInfos.infos[i].reserve = additionalData->kfcInfos.infos[i].reserve;
        aicpuData->KfcInfos.infos[i].streamId = additionalData->kfcInfos.infos[i].streamId;
        aicpuData->KfcInfos.infos[i].planeID = additionalData->kfcInfos.infos[i].planeID;
        aicpuData->KfcInfos.infos[i].opType = additionalData->kfcInfos.infos[i].opType;
        aicpuData->KfcInfos.infos[i].dataType = additionalData->kfcInfos.infos[i].dataType;
        aicpuData->KfcInfos.infos[i].linkType = additionalData->kfcInfos.infos[i].linkType;
        aicpuData->KfcInfos.infos[i].transportType = additionalData->kfcInfos.infos[i].transportType;
        aicpuData->KfcInfos.infos[i].rdmaType = additionalData->kfcInfos.infos[i].rdmaType;
        aicpuData->KfcInfos.infos[i].role = additionalData->kfcInfos.infos[i].role;
        aicpuData->KfcInfos.infos[i].workFlowMode = additionalData->kfcInfos.infos[i].workFlowMode;
    }
    return true;
}

}  // namespace Adapter
}  // namespace Domain
}  // namespace Analysis
