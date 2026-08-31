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

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include <string.h>

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/domain/services/adapter/parser_struct_adapter.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "securec.h"

using namespace testing;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Adapter;

namespace
{
void InitCompactInfoHeader(MsprofCompactInfo &compact)
{
    compact.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    compact.level = 1;
    compact.type = 2;
    compact.threadId = 3;
    compact.dataLen = 4;
    compact.timeStamp = 5;
}

void ExpectCompactInfoHeader(const ParserCompactInfo &parsed, const MsprofCompactInfo &compact)
{
    EXPECT_EQ(parsed.magicNumber, compact.magicNumber);
    EXPECT_EQ(parsed.level, compact.level);
    EXPECT_EQ(parsed.type, compact.type);
    EXPECT_EQ(parsed.threadId, compact.threadId);
    EXPECT_EQ(parsed.dataLen, compact.dataLen);
    EXPECT_EQ(parsed.timeStamp, compact.timeStamp);
}

void InitAdditionalInfoHeader(MsprofAdditionalInfo &addition)
{
    addition.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    addition.level = 1;
    addition.type = 2;
    addition.threadId = 3;
    addition.dataLen = 4;
    addition.timeStamp = 5;
}
}  // namespace

TEST(ParserCompactInfoAdapterTest, AdapterRuntimeTrackShouldMapV1Fields)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.runtimeTrack.deviceId = 6;
    compact.data.runtimeTrack.streamId = 7;
    compact.data.runtimeTrack.taskId = 8;
    compact.data.runtimeTrack.taskType = 9;
    compact.data.runtimeTrack.kernelName = 10;
    compact.data.runtimeTrack.extInfo.kernelInfo.numBlocks = 11;
    compact.data.runtimeTrack.extInfo.kernelInfo.argsSize = 12;
    compact.data.runtimeTrack.extInfo.kernelInfo.ratio = 3;
    compact.data.runtimeTrack.extInfo.kernelInfo.schedMode = 2;

    bool ret = ParserCompactInfoAdapter::AdapterRuntimeTrack(&compact, RuntimeTrackFormat::V1, &parsed);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.runtimeTrack.deviceId, 6);
    EXPECT_EQ(parsed.data.runtimeTrack.streamId, 7);
    EXPECT_EQ(parsed.data.runtimeTrack.taskId, 8);
    EXPECT_EQ(parsed.data.runtimeTrack.taskType, 9);
    EXPECT_EQ(parsed.data.runtimeTrack.kernelInfo.numBlocks, 11);
}

TEST(ParserCompactInfoAdapterTest, AdapterRuntimeTrackShouldMapV2Fields)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.runtimeTrackV2.deviceId = 6;
    compact.data.runtimeTrackV2.streamId = 7;
    compact.data.runtimeTrackV2.taskId = 8;
    compact.data.runtimeTrackV2.taskType = 9;
    compact.data.runtimeTrackV2.kernelName = 10;
    compact.data.runtimeTrackV2.extInfo.kernelInfo.numBlocks = 11;
    compact.data.runtimeTrackV2.extInfo.kernelInfo.argsSize = 12;
    compact.data.runtimeTrackV2.extInfo.kernelInfo.ratio = 3;
    compact.data.runtimeTrackV2.extInfo.kernelInfo.schedMode = 2;

    bool ret = ParserCompactInfoAdapter::AdapterRuntimeTrack(&compact, RuntimeTrackFormat::V2, &parsed);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.runtimeTrack.deviceId, 6);
    EXPECT_EQ(parsed.data.runtimeTrack.streamId, 7);
    EXPECT_EQ(parsed.data.runtimeTrack.taskId, 8);
    EXPECT_EQ(parsed.data.runtimeTrack.taskType, 9);
    EXPECT_EQ(parsed.data.runtimeTrack.kernelInfo.numBlocks, 11);
}

TEST(ParserCompactInfoAdapterTest, AdapterRuntimeTrackShouldReturnFalseForUnsupportedFormat)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);

    bool ret = ParserCompactInfoAdapter::AdapterRuntimeTrack(&compact, static_cast<RuntimeTrackFormat>(99), &parsed);

    EXPECT_FALSE(ret);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldMapDpuTrack)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.dpuTrack.deviceId = 6;
    compact.data.dpuTrack.streamId = 7;
    compact.data.dpuTrack.taskId = 8;
    compact.data.dpuTrack.taskType = 9;
    compact.data.dpuTrack.res = 10;
    compact.data.dpuTrack.startTime = 11;

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::DPU_TRACK_TYPE);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.dpuTrack.deviceId, 6);
    EXPECT_EQ(parsed.data.dpuTrack.streamId, 7);
    EXPECT_EQ(parsed.data.dpuTrack.taskId, 8);
    EXPECT_EQ(parsed.data.dpuTrack.taskType, 9);
    EXPECT_EQ(parsed.data.dpuTrack.res, 10);
    EXPECT_EQ(parsed.data.dpuTrack.startTime, 11);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldMapNodeBasicInfo)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.nodeBasicInfo.opName = 6;
    compact.data.nodeBasicInfo.taskType = 7;
    compact.data.nodeBasicInfo.opType = 8;
    compact.data.nodeBasicInfo.blockNum = 9;
    compact.data.nodeBasicInfo.opFlag = 10;
    compact.data.nodeBasicInfo.opState = 11;

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::NODE_BASIC_INFO_TYPE);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.nodeBasicInfo.opName, 6);
    EXPECT_EQ(parsed.data.nodeBasicInfo.taskType, 7);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldMapAttrInfo)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.nodeAttrInfo.opName = 6;
    compact.data.nodeAttrInfo.attrType = 7;
    compact.data.nodeAttrInfo.hashId = 8;

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::ATTR_INFO_TYPE);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.nodeAttrInfo.opName, 6);
    EXPECT_EQ(parsed.data.nodeAttrInfo.attrType, 7);
    EXPECT_EQ(parsed.data.nodeAttrInfo.hashId, 8);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldMapHcclopInfo)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.hcclopInfo.relay = 1;
    compact.data.hcclopInfo.retry = 0;
    compact.data.hcclopInfo.dataType = 3;
    compact.data.hcclopInfo.algType = 6;
    compact.data.hcclopInfo.count = 7;
    compact.data.hcclopInfo.groupName = 8;

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::HCCL_OP_INFO_TYPE);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.hcclopInfo.relay, 1);
    EXPECT_EQ(parsed.data.hcclopInfo.retry, 0);
    EXPECT_EQ(parsed.data.hcclopInfo.dataType, 3);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldMapMemcpyInfo)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.memcpyInfo.dataSize = 6;
    compact.data.memcpyInfo.maxSize = 7;
    compact.data.memcpyInfo.memcpyDirection = 8;

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::MEMCPY_INFO_TYPE);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.memcpyInfo.dataSize, 6);
    EXPECT_EQ(parsed.data.memcpyInfo.maxSize, 7);
    EXPECT_EQ(parsed.data.memcpyInfo.memcpyDirection, 8);
}

TEST(ParserCompactInfoAdapterTest, AdapterCompactInfoShouldReturnFalseForUnsupportedFormat)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);

    bool ret = ParserCompactInfoAdapter::AdapterCompactInfo(&compact, &parsed, CompactInfoFormat::COMPACT_INFO_TYPE);

    EXPECT_FALSE(ret);
}

TEST(ParserCompactInfoAdapterTest, AdapterCaptureStreamInfoShouldMapV1Fields)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.captureStreamInfo.deviceId = 6;
    compact.data.captureStreamInfo.captureStatus = 7;
    compact.data.captureStreamInfo.modelStreamId = 8;
    compact.data.captureStreamInfo.originalStreamId = 9;
    compact.data.captureStreamInfo.modelId = 10;

    bool ret = ParserCompactInfoAdapter::AdapterCaptureStreamInfo(&compact, CaptureStreamFormat::V1, &parsed);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.captureStreamInfo.deviceId, 6);
    EXPECT_EQ(parsed.data.captureStreamInfo.captureStatus, 7);
}

TEST(ParserCompactInfoAdapterTest, AdapterCaptureStreamInfoShouldMapV2Fields)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);
    compact.data.captureStreamInfoV2.deviceId = 6;
    compact.data.captureStreamInfoV2.captureStatus = 7;
    compact.data.captureStreamInfoV2.streamId = 8;
    compact.data.captureStreamInfoV2.originalStreamId = 9;
    compact.data.captureStreamInfoV2.modelId = 10;

    bool ret = ParserCompactInfoAdapter::AdapterCaptureStreamInfo(&compact, CaptureStreamFormat::V2, &parsed);

    EXPECT_TRUE(ret);
    ExpectCompactInfoHeader(parsed, compact);
    EXPECT_EQ(parsed.data.captureStreamInfo.deviceId, 6);
    EXPECT_EQ(parsed.data.captureStreamInfo.captureStatus, 7);
}

TEST(ParserCompactInfoAdapterTest, AdapterCaptureStreamInfoShouldReturnFalseForUnsupportedFormat)
{
    MsprofCompactInfo compact{};
    ParserCompactInfo parsed{};
    InitCompactInfoHeader(compact);

    bool ret = ParserCompactInfoAdapter::AdapterCaptureStreamInfo(&compact, static_cast<CaptureStreamFormat>(99), &parsed);

    EXPECT_FALSE(ret);
}

TEST(ParserApiAdapterTest, AdapterApiShouldMapAllFields)
{
    MsprofApi api{};
    ParserApi parsed{};
    api.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    api.level = 1;
    api.type = 2;
    api.threadId = 3;
    api.beginTime = 4;
    api.endTime = 5;
    api.itemId = 6;

    bool ret = ParserApiAdapter::AdapterApi(&api, &parsed);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, api.magicNumber);
    EXPECT_EQ(parsed.level, api.level);
    EXPECT_EQ(parsed.type, api.type);
    EXPECT_EQ(parsed.threadId, api.threadId);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapContextIdInfo)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    addition.contextIdInfo.opName = 6;
    addition.contextIdInfo.ctxIdNum = 3;
    addition.contextIdInfo.ctxIds[0] = 10;
    addition.contextIdInfo.ctxIds[1] = 11;
    addition.contextIdInfo.ctxIds[2] = 12;

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed,
                                                                  AdditionalInfoFormat::CONTEXT_ID_INFO_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.contextIdInfo.opName, 6);
    EXPECT_EQ(parsed.contextIdInfo.ctxIdNum, 3);
    EXPECT_EQ(parsed.contextIdInfo.ctxIds[0], 10);
    EXPECT_EQ(parsed.contextIdInfo.ctxIds[1], 11);
    EXPECT_EQ(parsed.contextIdInfo.ctxIds[2], 12);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapFusionOpInfo)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    addition.fusionOpInfo.opName = 6;
    addition.fusionOpInfo.fusionOpNum = 2;
    addition.fusionOpInfo.inputMemsize = 7;
    addition.fusionOpInfo.outputMemsize = 8;
    addition.fusionOpInfo.weightMemSize = 9;
    addition.fusionOpInfo.workspaceMemSize = 10;
    addition.fusionOpInfo.totalMemSize = 11;
    addition.fusionOpInfo.fusionOpId[0] = 12;
    addition.fusionOpInfo.fusionOpId[1] = 13;

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed,
                                                                  AdditionalInfoFormat::FUSION_OP_INFO_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.fusionOpInfo.opName, 6);
    EXPECT_EQ(parsed.fusionOpInfo.fusionOpNum, 2);
    EXPECT_EQ(parsed.fusionOpInfo.inputMemsize, 7);
    EXPECT_EQ(parsed.fusionOpInfo.fusionOpId[0], 12);
    EXPECT_EQ(parsed.fusionOpInfo.fusionOpId[1], 13);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapGraphId)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    MsprofGraphIdInfo graphId{};
    graphId.modelName = 6;
    graphId.graphId = 7;
    graphId.modelId = 8;
    memcpy(addition.data, &graphId, sizeof(graphId));

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed,
                                                                  AdditionalInfoFormat::GRAPH_ID_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.graphIdInfo.modelName, 6);
    EXPECT_EQ(parsed.graphIdInfo.graphId, 7);
    EXPECT_EQ(parsed.graphIdInfo.modelId, 8);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapHcclInfo)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    addition.hcclInfo.itemId = 6;
    addition.hcclInfo.cclTag = 7;
    addition.hcclInfo.groupName = 8;
    addition.hcclInfo.localRank = 9;
    addition.hcclInfo.remoteRank = 10;
    addition.hcclInfo.rankSize = 11;

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed, AdditionalInfoFormat::HCCL_INFO_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.hcclInfo.itemId, 6);
    EXPECT_EQ(parsed.hcclInfo.cclTag, 7);
    EXPECT_EQ(parsed.hcclInfo.groupName, 8);
    EXPECT_EQ(parsed.hcclInfo.localRank, 9);
    EXPECT_EQ(parsed.hcclInfo.remoteRank, 10);
    EXPECT_EQ(parsed.hcclInfo.rankSize, 11);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapMultiThread)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    addition.multiThread.threadNum = 3;
    addition.multiThread.threadId[0] = 10;
    addition.multiThread.threadId[1] = 11;
    addition.multiThread.threadId[2] = 12;

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed,
                                                                  AdditionalInfoFormat::MULTI_THREAD_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.multiThread.threadNum, 3);
    EXPECT_EQ(parsed.multiThread.threadId[0], 10);
    EXPECT_EQ(parsed.multiThread.threadId[1], 11);
    EXPECT_EQ(parsed.multiThread.threadId[2], 12);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldMapMemoryInfo)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);
    addition.memoryInfo.addr = 6;
    addition.memoryInfo.size = -7;
    addition.memoryInfo.nodeId = 8;
    addition.memoryInfo.totalAllocateMemory = 9;
    addition.memoryInfo.totalReserveMemory = 10;
    addition.memoryInfo.deviceId = 11;
    addition.memoryInfo.deviceType = 12;

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(
        &addition, &parsed, AdditionalInfoFormat::TASK_MEMORY_INFO_TYPE);

    EXPECT_TRUE(ret);
    EXPECT_EQ(parsed.magicNumber, addition.magicNumber);
    EXPECT_EQ(parsed.level, addition.level);
    EXPECT_EQ(parsed.type, addition.type);
    EXPECT_EQ(parsed.threadId, addition.threadId);
    EXPECT_EQ(parsed.dataLen, addition.dataLen);
    EXPECT_EQ(parsed.timeStamp, addition.timeStamp);
    EXPECT_EQ(parsed.memoryInfo.addr, 6);
    EXPECT_EQ(parsed.memoryInfo.size, -7);
    EXPECT_EQ(parsed.memoryInfo.nodeId, 8);
    EXPECT_EQ(parsed.memoryInfo.totalAllocateMemory, 9);
    EXPECT_EQ(parsed.memoryInfo.totalReserveMemory, 10);
    EXPECT_EQ(parsed.memoryInfo.deviceId, 11);
    EXPECT_EQ(parsed.memoryInfo.deviceType, 12);
}

TEST(ParserAdditionalInfoAdapterTest, AdapterAdditionalInfoShouldReturnFalseForUnsupportedFormat)
{
    MsprofAdditionalInfo addition{};
    ParserAdditionalInfo parsed{};
    InitAdditionalInfoHeader(addition);

    bool ret = ParserAdditionalInfoAdapter::AdapterAdditionalInfo(&addition, &parsed,
                                                                  AdditionalInfoFormat::TENSOR_INFO_TYPE);

    EXPECT_FALSE(ret);
}

TEST(ParserAicpuAdapterTest, AdapterNodeShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.aicpuNode.streamId = 1;
    addition.aicpuNode.taskId = 2;
    addition.aicpuNode.rev = 3;
    addition.aicpuNode.runStartTime = 4;
    addition.aicpuNode.runStartTick = 5;
    addition.aicpuNode.computeStartTime = 6;

    bool ret = ParserAicpuAdapter::AdapterNode(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.node.streamId, 1);
    EXPECT_EQ(data.node.taskId, 2);
    EXPECT_EQ(data.node.rev, 3);
    EXPECT_EQ(data.node.runStartTime, 4);
    EXPECT_EQ(data.node.runStartTick, 5);
    EXPECT_EQ(data.node.computeStartTime, 6);
}

TEST(ParserAicpuAdapterTest, AdapterModelShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.aicpuModel.indexId = 1;
    addition.aicpuModel.modelId = 2;
    addition.aicpuModel.tagId = 3;
    addition.aicpuModel.rsv1 = 4;
    addition.aicpuModel.eventId = 5;

    bool ret = ParserAicpuAdapter::AdapterModel(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.model.indexId, 1);
    EXPECT_EQ(data.model.modelId, 2);
    EXPECT_EQ(data.model.tagId, 3);
    EXPECT_EQ(data.model.rsv1, 4);
    EXPECT_EQ(data.model.eventId, 5);
}

TEST(ParserAicpuAdapterTest, AdapterDpShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    memset(addition.aicpuDp.action, 0, sizeof(addition.aicpuDp.action));
    memset(addition.aicpuDp.source, 0, sizeof(addition.aicpuDp.source));
    memcpy(addition.aicpuDp.action, "read", 5);
    memcpy(addition.aicpuDp.source, "device0", 8);
    addition.aicpuDp.index = 42;
    addition.aicpuDp.size = 1024;

    bool ret = ParserAicpuAdapter::AdapterDp(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_STREQ("read", data.dp.action);
    EXPECT_STREQ("device0", data.dp.source);
    EXPECT_EQ(data.dp.index, 42);
    EXPECT_EQ(data.dp.size, 1024);
}

TEST(ParserAicpuAdapterTest, AdapterDpShouldReturnFalseWhenMemcpySFails)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};

    MOCKER(&memcpy_s).stubs().will(returnValue(static_cast<errno_t>(1)));

    bool ret = ParserAicpuAdapter::AdapterDp(&addition, &data);

    EXPECT_FALSE(ret);

    MOCKER(&memcpy_s).reset();
}

TEST(ParserAicpuAdapterTest, AdapterMiShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.aicpuMi.nodeTag = 1;
    addition.aicpuMi.reserve = 2;
    addition.aicpuMi.queueSize = 3;
    addition.aicpuMi.runStartTime = 4;
    addition.aicpuMi.runEndTime = 5;

    bool ret = ParserAicpuAdapter::AdapterMi(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.mi.nodeTag, 1);
    EXPECT_EQ(data.mi.reserve, 2);
    EXPECT_EQ(data.mi.queueSize, 3);
    EXPECT_EQ(data.mi.runStartTime, 4);
    EXPECT_EQ(data.mi.runEndTime, 5);
}

TEST(ParserAicpuAdapterTest, AdapterCommTurnShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.commTurn.serverStartTime = 1;
    addition.commTurn.waitMsgStartTime = 2;
    addition.commTurn.kfcAlgExeStartTime = 3;
    addition.commTurn.sendTaskStartTime = 4;

    bool ret = ParserAicpuAdapter::AdapterCommTurn(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.commTurn.serverStartTime, 1);
    EXPECT_EQ(data.commTurn.waitMsgStartTime, 2);
    EXPECT_EQ(data.commTurn.kfcAlgExeStartTime, 3);
    EXPECT_EQ(data.commTurn.sendTaskStartTime, 4);
}

TEST(ParserAicpuAdapterTest, AdapterComputeTurnShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.computeTurn.waitComputeStartTime = 1;
    addition.computeTurn.computeStartTime = 2;
    addition.computeTurn.computeExeEndTime = 3;
    addition.computeTurn.dataLen = 4;
    addition.computeTurn.deviceId = 5;

    bool ret = ParserAicpuAdapter::AdapterComputeTurn(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.computeTurn.waitComputeStartTime, 1);
    EXPECT_EQ(data.computeTurn.computeStartTime, 2);
    EXPECT_EQ(data.computeTurn.computeExeEndTime, 3);
    EXPECT_EQ(data.computeTurn.dataLen, 4);
    EXPECT_EQ(data.computeTurn.deviceId, 5);
}

TEST(ParserAicpuAdapterTest, AdapterOpInfoShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.opInfo.relay = 1;
    addition.opInfo.retry = 0;
    addition.opInfo.dataType = 3;
    addition.opInfo.algType = 4;
    addition.opInfo.count = 5;
    addition.opInfo.groupName = 6;
    addition.opInfo.rankSize = 7;
    addition.opInfo.streamId = 8;
    addition.opInfo.taskId = 9;

    bool ret = ParserAicpuAdapter::AdapterOpInfo(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.opInfo.relay, 1);
    EXPECT_EQ(data.opInfo.retry, 0);
    EXPECT_EQ(data.opInfo.dataType, 3);
    EXPECT_EQ(data.opInfo.algType, 4);
    EXPECT_EQ(data.opInfo.count, 5);
}

TEST(ParserAicpuAdapterTest, AdapterFlipTaskShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.flipTask.streamId = 1;
    addition.flipTask.taskId = 2;
    addition.flipTask.flipNum = 3;

    bool ret = ParserAicpuAdapter::AdapterFlipTask(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.flipTask.streamId, 1);
    EXPECT_EQ(data.flipTask.taskId, 2);
    EXPECT_EQ(data.flipTask.flipNum, 3);
}

TEST(ParserAicpuAdapterTest, AdapterMainStreamTaskShouldMapAllFields)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    addition.mainStreamTask.aicpuStreamId = 1;
    addition.mainStreamTask.aicpuTaskId = 2;
    addition.mainStreamTask.streamId = 3;
    addition.mainStreamTask.taskId = 4;
    addition.mainStreamTask.type = 5;

    bool ret = ParserAicpuAdapter::AdapterMainStreamTask(&addition, &data);

    EXPECT_TRUE(ret);
    EXPECT_EQ(data.mainStreamTask.aicpuStreamId, 1);
    EXPECT_EQ(data.mainStreamTask.aicpuTaskId, 2);
    EXPECT_EQ(data.mainStreamTask.streamId, 3);
    EXPECT_EQ(data.mainStreamTask.taskId, 4);
    EXPECT_EQ(data.mainStreamTask.type, 5);
}

TEST(ParserAicpuAdapterTest, AdapterKfcInfosShouldMapAllEntries)
{
    MsprofAdditionalInfo addition{};
    AicpuData data{};
    for (uint32_t i = 0; i < KFC_INFOS_NUM; ++i) {
        addition.kfcInfos.infos[i].itemId = 1 + i;
        addition.kfcInfos.infos[i].cclTag = 2 + i;
        addition.kfcInfos.infos[i].groupName = 3 + i;
        addition.kfcInfos.infos[i].localRank = 4 + i;
        addition.kfcInfos.infos[i].remoteRank = 5 + i;
        addition.kfcInfos.infos[i].rankSize = 6 + i;
        addition.kfcInfos.infos[i].stage = 7 + i;
        addition.kfcInfos.infos[i].notifyID = 8 + i;
        addition.kfcInfos.infos[i].timeStamp = 9 + i;
        addition.kfcInfos.infos[i].durationEstimated = 1.5 + i;
    }

    bool ret = ParserAicpuAdapter::AdapterKfcInfos(&addition, &data);

    EXPECT_TRUE(ret);
    for (uint32_t i = 0; i < KFC_INFOS_NUM; ++i) {
        EXPECT_EQ(data.KfcInfos.infos[i].itemId, 1 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].cclTag, 2 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].groupName, 3 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].localRank, 4 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].remoteRank, 5 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].rankSize, 6 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].stage, 7 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].notifyID, 8 + i);
        EXPECT_EQ(data.KfcInfos.infos[i].timeStamp, 9 + i);
    }
}
