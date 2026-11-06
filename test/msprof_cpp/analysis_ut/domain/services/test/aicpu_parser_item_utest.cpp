/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include <gtest/gtest.h>

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

using namespace testing;

namespace Analysis {
namespace Domain {

// 声明 aicpu_parser_item.cpp 中的 parse 函数以便直接测试
int AicpuNodeParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuDpParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuModelParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuMiParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int KfcCommTurnParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int KfcComputeTurnParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int HcclOpInfoParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuFlipTaskParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuMasterStreamHcclTaskParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData,
                                       uint16_t expandStatus);
int KfcHcclInfoParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
// V6（chip 15/16/19）专用实现
int AicpuNodeParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int KfcCommTurnParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int KfcComputeTurnParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData,
                               uint16_t expandStatus);
int HcclOpInfoParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuFlipTaskParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData, uint16_t expandStatus);
int AicpuMasterStreamHcclTaskParseItem_V6(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *aicpuData,
                                          uint16_t expandStatus);

namespace {

const uint16_t TEST_STREAM_ID = 0x1000;
const uint16_t TEST_TASK_ID = 0x2000;
const uint16_t EXPAND_STATUS_OFF = 0;
const uint16_t EXPAND_STATUS_ON = 1;
const uint64_t TEST_TIMESTAMP = 0x123456789ABCDEF0ULL;

// =========================================================================
// AICPU_NODE parse item 测试
// =========================================================================

class AicpuNodeParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.aicpuNode.streamId = TEST_STREAM_ID;
        additionalInfo_.aicpuNode.taskId = TEST_TASK_ID;
        additionalInfo_.aicpuNode.runStartTick = 100;
        additionalInfo_.aicpuNode.runEndTick = 200;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuNodeParseItemUtest, ShouldCopyTimestampAndNodeDataCorrectly)
{
    int ret = AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                 reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.node.runStartTick, 100);
    EXPECT_EQ(aicpuData_.node.runEndTick, 200);
}

TEST_F(AicpuNodeParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                       reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_NODE);
}

TEST_F(AicpuNodeParseItemUtest, ShouldSetBatchIdToZero)
{
    AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                       reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.taskId.batchId, 0);
}

TEST_F(AicpuNodeParseItemUtest, ShouldSetContextIdToInvalid)
{
    AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                       reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.taskId.contextId, INVALID_CONTEXT_ID);
}

// =========================================================================
// AICPU_DP parse item 测试
// =========================================================================

class AicpuDpParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.aicpuDp.index = 42;
        additionalInfo_.aicpuDp.size = 1024;
        memcpy(additionalInfo_.aicpuDp.action, "read", 5);
        memcpy(additionalInfo_.aicpuDp.source, "device0", 8);
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuDpParseItemUtest, ShouldCopyTimestampAndDpDataCorrectly)
{
    int ret = AicpuDpParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                               reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.dp.index, 42);
    EXPECT_EQ(aicpuData_.dp.size, 1024);
}

TEST_F(AicpuDpParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuDpParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                     reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_DP);
}

// =========================================================================
// AICPU_MODEL parse item 测试
// =========================================================================

class AicpuModelParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.aicpuModel.indexId = 0xABCD;
        additionalInfo_.aicpuModel.modelId = 12345;
        additionalInfo_.aicpuModel.tagId = 7;
        additionalInfo_.aicpuModel.eventId = 999;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuModelParseItemUtest, ShouldCopyTimestampAndModelDataCorrectly)
{
    int ret = AicpuModelParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                  reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.model.indexId, 0xABCD);
    EXPECT_EQ(aicpuData_.model.modelId, 12345);
    EXPECT_EQ(aicpuData_.model.tagId, 7);
    EXPECT_EQ(aicpuData_.model.eventId, 999);
}

TEST_F(AicpuModelParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuModelParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                        reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_MODEL);
}

// =========================================================================
// AICPU_MI parse item 测试
// =========================================================================

class AicpuMiParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.aicpuMi.nodeTag = 1;
        additionalInfo_.aicpuMi.queueSize = 64;
        additionalInfo_.aicpuMi.runStartTime = 10000;
        additionalInfo_.aicpuMi.runEndTime = 20000;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuMiParseItemUtest, ShouldCopyTimestampAndMiDataCorrectly)
{
    int ret = AicpuMiParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                               reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.mi.nodeTag, 1);
    EXPECT_EQ(aicpuData_.mi.queueSize, 64);
    EXPECT_EQ(aicpuData_.mi.runStartTime, 10000);
    EXPECT_EQ(aicpuData_.mi.runEndTime, 20000);
}

TEST_F(AicpuMiParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuMiParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                     reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_MI);
}

// =========================================================================
// KFC_COMM_TURN parse item 测试
// =========================================================================

class KfcCommTurnParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.commTurn.deviceId = 0;
        additionalInfo_.commTurn.streamId = TEST_STREAM_ID;
        additionalInfo_.commTurn.taskId = TEST_TASK_ID;
        additionalInfo_.commTurn.commTurn = 3;
        additionalInfo_.commTurn.currentTurn = 1;
        additionalInfo_.commTurn.serverStartTime = 1000;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(KfcCommTurnParseItemUtest, ShouldCopyTimestampAndCommTurnDataCorrectly)
{
    int ret = KfcCommTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                   reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.commTurn.commTurn, 3);
    EXPECT_EQ(aicpuData_.commTurn.currentTurn, 1);
    EXPECT_EQ(aicpuData_.commTurn.serverStartTime, 1000);
}

TEST_F(KfcCommTurnParseItemUtest, ShouldSetCorrectAicpuType)
{
    KfcCommTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_COMM_TURN);
}

TEST_F(KfcCommTurnParseItemUtest, ShouldSetBatchIdToZero)
{
    KfcCommTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.taskId.batchId, 0);
}

TEST_F(KfcCommTurnParseItemUtest, ShouldSetContextIdToInvalid)
{
    KfcCommTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.taskId.contextId, INVALID_CONTEXT_ID);
}

// =========================================================================
// KFC_COMPUTE_TURN parse item 测试
// =========================================================================

class KfcComputeTurnParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.computeTurn.deviceId = 0;
        additionalInfo_.computeTurn.streamId = TEST_STREAM_ID;
        additionalInfo_.computeTurn.taskId = TEST_TASK_ID;
        additionalInfo_.computeTurn.computeTurn = 5;
        additionalInfo_.computeTurn.currentTurn = 2;
        additionalInfo_.computeTurn.waitComputeStartTime = 500;
        additionalInfo_.computeTurn.computeStartTime = 600;
        additionalInfo_.computeTurn.computeExeEndTime = 700;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(KfcComputeTurnParseItemUtest, ShouldCopyTimestampAndComputeTurnDataCorrectly)
{
    int ret = KfcComputeTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                      reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.computeTurn.computeTurn, 5);
    EXPECT_EQ(aicpuData_.computeTurn.currentTurn, 2);
    EXPECT_EQ(aicpuData_.computeTurn.waitComputeStartTime, 500);
}

TEST_F(KfcComputeTurnParseItemUtest, ShouldSetCorrectAicpuType)
{
    KfcComputeTurnParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                            reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_COMPUTE_TURN);
}

// =========================================================================
// HCCL_OP_INFO parse item 测试
// =========================================================================

class HcclOpInfoParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.opInfo.streamId = TEST_STREAM_ID;
        additionalInfo_.opInfo.taskId = TEST_TASK_ID;
        additionalInfo_.opInfo.relay = 1;
        additionalInfo_.opInfo.retry = 0;
        additionalInfo_.opInfo.dataType = 3;
        additionalInfo_.opInfo.algType = 0xDEAD;
        additionalInfo_.opInfo.count = 128;
        additionalInfo_.opInfo.rankSize = 8;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(HcclOpInfoParseItemUtest, ShouldCopyTimestampAndOpInfoDataCorrectly)
{
    int ret = HcclOpInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                  reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.opInfo.relay, 1);
    EXPECT_EQ(aicpuData_.opInfo.retry, 0);
    EXPECT_EQ(aicpuData_.opInfo.dataType, 3);
    EXPECT_EQ(aicpuData_.opInfo.algType, 0xDEAD);
    EXPECT_EQ(aicpuData_.opInfo.count, 128);
    EXPECT_EQ(aicpuData_.opInfo.rankSize, 8);
}

TEST_F(HcclOpInfoParseItemUtest, ShouldSetCorrectAicpuType)
{
    HcclOpInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                        reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::HCCL_OP_INFO);
}

// =========================================================================
// AICPU_FLIP_TASK parse item 测试
// =========================================================================

class AicpuFlipTaskParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.flipTask.streamId = TEST_STREAM_ID;
        additionalInfo_.flipTask.taskId = TEST_TASK_ID;
        additionalInfo_.flipTask.flipNum = 3;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuFlipTaskParseItemUtest, ShouldCopyTimestampAndFlipTaskDataCorrectly)
{
    int ret = AicpuFlipTaskParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                     reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.flipTask.flipNum, 3);
}

TEST_F(AicpuFlipTaskParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuFlipTaskParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                           reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_FLIP_TASK);
}

// =========================================================================
// AICPU_MASTER_STREAM_HCCL_TASK parse item 测试
// =========================================================================

class AicpuMasterStreamHcclTaskParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        additionalInfo_.mainStreamTask.aicpuStreamId = 0x100;
        additionalInfo_.mainStreamTask.aicpuTaskId = 0x200;
        additionalInfo_.mainStreamTask.streamId = 0x300;
        additionalInfo_.mainStreamTask.taskId = 0x400;
        additionalInfo_.mainStreamTask.type = 1;  // 尾
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(AicpuMasterStreamHcclTaskParseItemUtest, ShouldCopyTimestampAndMainStreamTaskDataCorrectly)
{
    int ret = AicpuMasterStreamHcclTaskParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_),
                                                  sizeof(additionalInfo_),
                                                  reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.mainStreamTask.type, 1);

    // aicpu task 信息应写入 aicpuTaskId，而非 taskId
    EXPECT_EQ(aicpuData_.aicpuTaskId.streamId, 0x100);
    EXPECT_EQ(aicpuData_.aicpuTaskId.taskId, 0x200);
    EXPECT_EQ(aicpuData_.aicpuTaskId.batchId, 0);
    EXPECT_EQ(aicpuData_.aicpuTaskId.contextId, INVALID_CONTEXT_ID);

    // expand task 信息应写入 taskId，而非 aicpuTaskId
    EXPECT_EQ(aicpuData_.taskId.streamId, 0x300);
    EXPECT_EQ(aicpuData_.taskId.taskId, 0x400);
    EXPECT_EQ(aicpuData_.taskId.batchId, 0);
    EXPECT_EQ(aicpuData_.taskId.contextId, INVALID_CONTEXT_ID);
}

TEST_F(AicpuMasterStreamHcclTaskParseItemUtest, ShouldSetCorrectAicpuType)
{
    AicpuMasterStreamHcclTaskParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                       reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_MASTER_STREAM_HCCL_TASK);
}

// =========================================================================
// KFC_HCCL_INFO parse item 测试
// =========================================================================

class KfcHcclInfoParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
        additionalInfo_.timeStamp = TEST_TIMESTAMP;
        // KfcInfos contains two HcclTaskInfo entries
        additionalInfo_.kfcInfos.infos[0].streamId = 0x11;
        additionalInfo_.kfcInfos.infos[0].taskId = 0x22;
        additionalInfo_.kfcInfos.infos[1].streamId = 0x33;
        additionalInfo_.kfcInfos.infos[1].taskId = 0x44;
    }

protected:
    MsprofAdditionalInfo additionalInfo_;
    AicpuData aicpuData_;
};

TEST_F(KfcHcclInfoParseItemUtest, ShouldCopyTimestampAndKfcInfosDataCorrectly)
{
    int ret = KfcHcclInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                   reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[0].streamId, 0x11);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[0].taskId, 0x22);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[1].streamId, 0x33);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[1].taskId, 0x44);
}

TEST_F(KfcHcclInfoParseItemUtest, ShouldSetCorrectAicpuType)
{
    KfcHcclInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_HCCL_INFO);
}

TEST_F(KfcHcclInfoParseItemUtest, ShouldResetBatchIdWhenParse)
{
    // 上报数据不携带 batchId（联合体成员不会被默认初始化），解析时必须显式置 0，
    // 否则会残留脏值并落盘到 kfc_info.db 的 batch_id 列
    aicpuData_.KfcInfos.infos[0].batchId = 0xFFFF;
    aicpuData_.KfcInfos.infos[1].batchId = 0xFFFF;

    KfcHcclInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    for (uint32_t i = 0; i < KFC_INFOS_NUM; i++)
    {
        EXPECT_EQ(aicpuData_.KfcInfos.infos[i].batchId, 0);
    }
}

// =========================================================================
// expandStatus=1 场景测试 (Node 为代表)
// =========================================================================

TEST_F(AicpuNodeParseItemUtest, ShouldHandleExpandStatusOn)
{
    // expandStatus=1 时仍应正常解析并返回 OK
    int ret = AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                 reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_ON);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.timeStamp, TEST_TIMESTAMP);
    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_NODE);
}

}  // namespace

// =========================================================================
// 注册验证：所有 REGISTER_PARSER_ITEM 宏可通过 ParserItemFactory 获取
// =========================================================================

TEST(AicpuParserItemRegistrationTest, ShouldRetrieveAllTenParserItemsViaFactory)
{
    // 触发 aicpu_parser_item.cpp 中静态注册（链接后自动生效）
    auto nodeFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_NODE));
    auto dpFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_DP));
    auto modelFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_MODEL));
    auto miFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_MI));
    auto commFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::KFC_COMM_TURN));
    auto compFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::KFC_COMPUTE_TURN));
    auto opFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::HCCL_OP_INFO));
    auto flipFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_FLIP_TASK));
    auto mainFunc =
        ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK));
    auto kfcFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::KFC_HCCL_INFO));

    EXPECT_NE(nodeFunc, nullptr);
    EXPECT_NE(dpFunc, nullptr);
    EXPECT_NE(modelFunc, nullptr);
    EXPECT_NE(miFunc, nullptr);
    EXPECT_NE(commFunc, nullptr);
    EXPECT_NE(compFunc, nullptr);
    EXPECT_NE(opFunc, nullptr);
    EXPECT_NE(flipFunc, nullptr);
    EXPECT_NE(mainFunc, nullptr);
    EXPECT_NE(kfcFunc, nullptr);
}

// =========================================================================
// V6（chip 15/16/19）parse item 测试
// 语义对齐 Python StarsCommon：streamId 保持原值，taskId 按 need_merge 决定是否 32bit 合并
// =========================================================================

class AicpuV6ParseItemUtest : public Test {
protected:
    void SetUp() override
    {
        memset(&additionalInfo_, 0, sizeof(additionalInfo_));
        memset(&aicpuData_, 0, sizeof(aicpuData_));
    }

    void TearDown() override {}

    MsprofAdditionalInfo additionalInfo_{};
    AicpuData aicpuData_{};
};

// streamId 的 bit12/bit13 置位时，V6 也必须保持原值（旧 16bit 规则会被触发并改写原值）
const uint16_t V6_BIT12_STREAM_ID = 0x1000;
const uint16_t V6_BIT13_STREAM_ID = 0x2000;
const uint16_t V6_PLAIN_STREAM_ID = 0x0042;

TEST_F(AicpuV6ParseItemUtest, ShouldKeepRawStreamIdAndTaskIdForNode)
{
    additionalInfo_.aicpuNode.streamId = V6_BIT12_STREAM_ID;
    additionalInfo_.aicpuNode.taskId = 0x0033;
    aicpuData_.type = AicpuType::AICPU_DP;  // 非目标类型，确认被覆盖

    int ret = AicpuNodeParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                    reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    // need_merge=False：streamId / taskId 均透传上报原值
    EXPECT_EQ(aicpuData_.taskId.streamId, V6_BIT12_STREAM_ID);
    EXPECT_EQ(aicpuData_.taskId.taskId, 0x0033u);
    EXPECT_EQ(aicpuData_.taskId.batchId, 0u);
    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_NODE);
}

TEST_F(AicpuV6ParseItemUtest, ShouldMergeTaskIdForCommTurn)
{
    additionalInfo_.commTurn.streamId = V6_BIT13_STREAM_ID;
    additionalInfo_.commTurn.taskId = 0x0012;

    int ret = KfcCommTurnParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                      reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.taskId.streamId, V6_BIT13_STREAM_ID);
    // need_merge=True：高 16 位 taskId，低 16 位 streamId
    EXPECT_EQ(aicpuData_.taskId.taskId, ((0x0012u << 16) | V6_BIT13_STREAM_ID));
    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_COMM_TURN);
}

TEST_F(AicpuV6ParseItemUtest, ShouldMergeTaskIdForComputeTurn)
{
    additionalInfo_.computeTurn.streamId = V6_PLAIN_STREAM_ID;
    additionalInfo_.computeTurn.taskId = 0x0007;

    int ret = KfcComputeTurnParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                         reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.taskId.streamId, V6_PLAIN_STREAM_ID);
    EXPECT_EQ(aicpuData_.taskId.taskId, ((0x0007u << 16) | V6_PLAIN_STREAM_ID));
    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_COMPUTE_TURN);
}

TEST_F(AicpuV6ParseItemUtest, ShouldMergeTaskIdForFlipTask)
{
    additionalInfo_.flipTask.streamId = V6_PLAIN_STREAM_ID;
    additionalInfo_.flipTask.taskId = 0x0009;

    int ret = AicpuFlipTaskParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                        reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.taskId.streamId, V6_PLAIN_STREAM_ID);
    EXPECT_EQ(aicpuData_.taskId.taskId, ((0x0009u << 16) | V6_PLAIN_STREAM_ID));
    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_FLIP_TASK);
}

TEST_F(AicpuV6ParseItemUtest, ShouldKeepRawTaskIdForHcclOpInfo)
{
    additionalInfo_.opInfo.streamId = V6_BIT12_STREAM_ID;
    additionalInfo_.opInfo.taskId = 0x9ABCDEF0u;  // 上报本身为 u32

    int ret = HcclOpInfoParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                     reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.taskId.streamId, V6_BIT12_STREAM_ID);
    // need_merge=False：u32 taskId 不参与合并，也不截断
    EXPECT_EQ(aicpuData_.taskId.taskId, 0x9ABCDEF0u);
    EXPECT_EQ(aicpuData_.type, AicpuType::HCCL_OP_INFO);
}

TEST_F(AicpuV6ParseItemUtest, ShouldMergeBothTaskIdsForMasterStreamTask)
{
    additionalInfo_.mainStreamTask.aicpuStreamId = 0x7000;  // 28544, 接近样例中的 28520
    additionalInfo_.mainStreamTask.aicpuTaskId = 0x0032;
    additionalInfo_.mainStreamTask.streamId = 0x7A00;  // 31232, 接近样例中的 31139
    additionalInfo_.mainStreamTask.taskId = 0x8083;

    int ret = AicpuMasterStreamHcclTaskParseItem_V6(reinterpret_cast<uint8_t *>(&additionalInfo_),
                                                    sizeof(additionalInfo_),
                                                    reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    // 外层 TaskId 供批次计算等路径使用
    EXPECT_EQ(aicpuData_.aicpuTaskId.streamId, 0x7000u);
    EXPECT_EQ(aicpuData_.aicpuTaskId.taskId, ((0x0032u << 16) | 0x7000u));
    EXPECT_EQ(aicpuData_.taskId.streamId, 0x7A00u);
    EXPECT_EQ(aicpuData_.taskId.taskId, ((0x8083u << 16) | 0x7A00u));
    // union 字段是落盘与注入数据的取值来源，必须同样写入合并结果（回归点）
    EXPECT_EQ(aicpuData_.mainStreamTask.aicpuStreamId, 0x7000u);
    EXPECT_EQ(aicpuData_.mainStreamTask.aicpuTaskId, ((0x0032u << 16) | 0x7000u));
    EXPECT_EQ(aicpuData_.mainStreamTask.streamId, 0x7A00u);
    EXPECT_EQ(aicpuData_.mainStreamTask.taskId, ((0x8083u << 16) | 0x7A00u));
    // 外层与 union 必须一致，避免两套字段分叉
    EXPECT_EQ(aicpuData_.taskId.taskId, aicpuData_.mainStreamTask.taskId);
    EXPECT_EQ(aicpuData_.aicpuTaskId.taskId, aicpuData_.mainStreamTask.aicpuTaskId);
    EXPECT_EQ(aicpuData_.type, AicpuType::AICPU_MASTER_STREAM_HCCL_TASK);
}

TEST_F(AicpuV6ParseItemUtest, ShouldKeepRawKfcHcclInfoReusingNonV6Item)
{
    additionalInfo_.kfcInfos.infos[0].streamId = V6_BIT13_STREAM_ID;
    additionalInfo_.kfcInfos.infos[0].taskId = 0x0044u;

    int ret = KfcHcclInfoParseItem(reinterpret_cast<uint8_t *>(&additionalInfo_), sizeof(additionalInfo_),
                                   reinterpret_cast<uint8_t *>(&aicpuData_), EXPAND_STATUS_OFF);

    EXPECT_EQ(ret, ANALYSIS_OK);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[0].streamId, V6_BIT13_STREAM_ID);
    EXPECT_EQ(aicpuData_.KfcInfos.infos[0].taskId, 0x0044u);
    EXPECT_EQ(aicpuData_.type, AicpuType::KFC_HCCL_INFO);
}

TEST_F(AicpuV6ParseItemUtest, ShouldRegisterAllV6Items)
{
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_NODE)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_DP)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_MODEL)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_MI)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::KFC_COMM_TURN)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::KFC_COMPUTE_TURN)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::HCCL_OP_INFO)), nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_FLIP_TASK)), nullptr);
    EXPECT_NE(
        ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK)),
        nullptr);
    EXPECT_NE(ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::KFC_HCCL_INFO)), nullptr);
}

// 回归点：need_merge=False 只影响 taskId，不影响 streamId。
// 非 V6 实现会对 streamId 做 % 2048 掩码、对 taskId 做位拼接；
// V6 必须保留 streamId 原值，同时 taskId 不做合并。
TEST_F(AicpuV6ParseItemUtest, ShouldDifferFromNonV6OnStreamIdForNeedMergeFalseTypes)
{
    // streamId 的 bit12/bit13 同时置位，是旧规则必定改写原值的输入
    const uint16_t streamId = 0x3123;  // 12579
    const uint16_t taskId = 0x0045;

    // --- AICPU_NODE ---
    MsprofAdditionalInfo info{};
    AicpuData nonV6{};
    AicpuData v6{};
    info.aicpuNode.streamId = streamId;
    info.aicpuNode.taskId = taskId;

    AicpuNodeParseItem(reinterpret_cast<uint8_t *>(&info), sizeof(info), reinterpret_cast<uint8_t *>(&nonV6),
                       EXPAND_STATUS_OFF);
    AicpuNodeParseItem_V6(reinterpret_cast<uint8_t *>(&info), sizeof(info), reinterpret_cast<uint8_t *>(&v6),
                          EXPAND_STATUS_OFF);

    // 非 V6：streamId 被掩码成 streamId % 2048，taskId 被位拼接
    EXPECT_EQ(nonV6.taskId.streamId, static_cast<uint32_t>(streamId % 2048));
    EXPECT_NE(nonV6.taskId.taskId, taskId);
    // V6：streamId 与 taskId 均为原值
    EXPECT_EQ(v6.taskId.streamId, streamId);
    EXPECT_EQ(v6.taskId.taskId, taskId);
    EXPECT_NE(nonV6.taskId.streamId, v6.taskId.streamId);

    // --- HCCL_OP_INFO ---（taskId 上报本身是 u32）
    MsprofAdditionalInfo opInfo{};
    AicpuData opNonV6{};
    AicpuData opV6{};
    opInfo.opInfo.streamId = streamId;
    opInfo.opInfo.taskId = 0x9ABCDEF0u;

    HcclOpInfoParseItem(reinterpret_cast<uint8_t *>(&opInfo), sizeof(opInfo),
                        reinterpret_cast<uint8_t *>(&opNonV6), EXPAND_STATUS_OFF);
    HcclOpInfoParseItem_V6(reinterpret_cast<uint8_t *>(&opInfo), sizeof(opInfo), reinterpret_cast<uint8_t *>(&opV6),
                           EXPAND_STATUS_OFF);

    EXPECT_EQ(opNonV6.taskId.streamId, static_cast<uint32_t>(streamId % 2048));
    EXPECT_NE(opNonV6.taskId.taskId, 0x9ABCDEF0u);  // 非 V6 会截断 u32 taskId
    EXPECT_EQ(opV6.taskId.streamId, streamId);
    EXPECT_EQ(opV6.taskId.taskId, 0x9ABCDEF0u);     // V6 原样透传
}

// KFC_HCCL_INFO 在非 V6 下就是纯拷贝，V6 复用同一实现即可
TEST_F(AicpuV6ParseItemUtest, ShouldProduceSameResultForKfcHcclInfoInBothPaths)
{
    MsprofAdditionalInfo info{};
    AicpuData a{};
    AicpuData b{};
    info.kfcInfos.infos[0].streamId = 0x3123;
    info.kfcInfos.infos[0].taskId = 0x9ABCDEF0u;

    auto fn = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(AicpuType::KFC_HCCL_INFO));
    auto fnV6 = ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(AicpuType::KFC_HCCL_INFO));
    ASSERT_NE(fn, nullptr);
    ASSERT_NE(fnV6, nullptr);

    fn(reinterpret_cast<uint8_t *>(&info), sizeof(info), reinterpret_cast<uint8_t *>(&a), EXPAND_STATUS_OFF);
    fnV6(reinterpret_cast<uint8_t *>(&info), sizeof(info), reinterpret_cast<uint8_t *>(&b), EXPAND_STATUS_OFF);

    EXPECT_EQ(a.KfcInfos.infos[0].streamId, b.KfcInfos.infos[0].streamId);
    EXPECT_EQ(a.KfcInfos.infos[0].taskId, b.KfcInfos.infos[0].taskId);
    EXPECT_EQ(a.KfcInfos.infos[0].streamId, 0x3123);
    EXPECT_EQ(a.KfcInfos.infos[0].taskId, 0x9ABCDEF0u);
}

// union 字段由 u16 放宽到 u32 后，非 V6 的上报原值存入不得产生任何变化，
// 且必须与 adapter 拷入的 raw 值一致（非 V6 落盘读取的就是 raw）
TEST_F(AicpuV6ParseItemUtest, ShouldKeepRawUnionFieldsForMasterStreamTaskOnNonV6)
{
    const uint16_t rawAicpuStreamId = 0x7000;
    const uint16_t rawAicpuTaskId = 0x0032;
    const uint16_t rawStreamId = 0x7A00;
    const uint16_t rawTaskId = 0x8083;

    MsprofAdditionalInfo info{};
    info.mainStreamTask.aicpuStreamId = rawAicpuStreamId;
    info.mainStreamTask.aicpuTaskId = rawAicpuTaskId;
    info.mainStreamTask.streamId = rawStreamId;
    info.mainStreamTask.taskId = rawTaskId;
    info.mainStreamTask.type = 1;

    AicpuData data{};
    int ret = AicpuMasterStreamHcclTaskParseItem(reinterpret_cast<uint8_t *>(&info), sizeof(info),
                                                reinterpret_cast<uint8_t *>(&data), EXPAND_STATUS_OFF);
    EXPECT_EQ(ret, ANALYSIS_OK);

    // 非 V6：union 保持 adapter 拷入的 raw 上报值，加宽不改变数值
    EXPECT_EQ(data.mainStreamTask.aicpuStreamId, rawAicpuStreamId);
    EXPECT_EQ(data.mainStreamTask.aicpuTaskId, rawAicpuTaskId);
    EXPECT_EQ(data.mainStreamTask.streamId, rawStreamId);
    EXPECT_EQ(data.mainStreamTask.taskId, rawTaskId);
    EXPECT_EQ(data.mainStreamTask.type, 1);

    // 非 V6 仍按 StarsCommon 计算外层 TaskId（供批次计算使用），与 union raw 各自独立
    EXPECT_EQ(data.aicpuTaskId.taskId,
              static_cast<uint32_t>(StarsCommon::GetTaskId(rawAicpuStreamId, rawAicpuTaskId, EXPAND_STATUS_OFF)));
    EXPECT_EQ(data.taskId.taskId,
              static_cast<uint32_t>(StarsCommon::GetTaskId(rawStreamId, rawTaskId, EXPAND_STATUS_OFF)));
}

// =========================================================================
// V6 注册契约：确保 AICPU_PARSER_V6 下注册的是 *_V6 实现而非非 V6 函数
// 仅断言“工厂返回非空”无法发现注册笔误（如把 KfcCommTurnParseItem_V6 写成
// KfcCommTurnParseItem），此时工厂仍非空、所有按名字调用的用例仍通过，
// 但 V6 实际退化为 16bit 语义。这里直接比对函数指针实现是否不同。
// =========================================================================

TEST(AicpuParserItemRegistrationTest, ShouldRegisterDedicatedV6ImplementationForV6OnlyTypes)
{
    // 这 6 类在 V6 下语义与非 V6 不同，注册的必须是各自独立的 *_V6 函数
    using ItemFunc = std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)>;
    const std::pair<AicpuType, ItemFunc> dedicated[] = {
        {AicpuType::AICPU_NODE, AicpuNodeParseItem_V6},
        {AicpuType::KFC_COMM_TURN, KfcCommTurnParseItem_V6},
        {AicpuType::KFC_COMPUTE_TURN, KfcComputeTurnParseItem_V6},
        {AicpuType::HCCL_OP_INFO, HcclOpInfoParseItem_V6},
        {AicpuType::AICPU_FLIP_TASK, AicpuFlipTaskParseItem_V6},
        {AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, AicpuMasterStreamHcclTaskParseItem_V6},
    };
    for (const auto &item : dedicated)
    {
        auto v6Func = ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(item.first));
        auto baseFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(item.first));
        ASSERT_NE(v6Func, nullptr) << "type: " << static_cast<int>(item.first);
        ASSERT_NE(baseFunc, nullptr) << "type: " << static_cast<int>(item.first);

        // 注册的必须是预期的 *_V6 函数
        auto expected = item.second.target<int (*)(uint8_t *, uint32_t, uint8_t *, uint16_t)>();
        auto actual = v6Func.target<int (*)(uint8_t *, uint32_t, uint8_t *, uint16_t)>();
        ASSERT_NE(expected, nullptr) << "type: " << static_cast<int>(item.first);
        ASSERT_NE(actual, nullptr) << "type: " << static_cast<int>(item.first);
        EXPECT_EQ(*actual, *expected) << "V6 registration mismatch, type: " << static_cast<int>(item.first)
                                      << " (registered non-V6 implementation?)";

        // 且必须与非 V6 实现不同，否则 V6 语义退化
        auto base = baseFunc.target<int (*)(uint8_t *, uint32_t, uint8_t *, uint16_t)>();
        ASSERT_NE(base, nullptr);
        EXPECT_NE(*actual, *base) << "V6 type shares non-V6 implementation, type: "
                                  << static_cast<int>(item.first);
    }
}

TEST(AicpuParserItemRegistrationTest, ShouldReuseNonV6ImplementationForEquivalentTypes)
{
    // 这 4 类在 V6 下与非 V6 等价，允许（且预期）复用同一实现
    const AicpuType equivalent[] = {AicpuType::AICPU_DP, AicpuType::AICPU_MODEL, AicpuType::AICPU_MI,
                                    AicpuType::KFC_HCCL_INFO};
    for (AicpuType type : equivalent)
    {
        auto v6Func = ParserItemFactory::GetParseItem(AICPU_PARSER_V6, static_cast<int>(type));
        auto baseFunc = ParserItemFactory::GetParseItem(AICPU_PARSER, static_cast<int>(type));
        ASSERT_NE(v6Func, nullptr) << "type: " << static_cast<int>(type);
        ASSERT_NE(baseFunc, nullptr) << "type: " << static_cast<int>(type);

        auto v6 = v6Func.target<int (*)(uint8_t *, uint32_t, uint8_t *, uint16_t)>();
        auto base = baseFunc.target<int (*)(uint8_t *, uint32_t, uint8_t *, uint16_t)>();
        ASSERT_NE(v6, nullptr);
        ASSERT_NE(base, nullptr);
        EXPECT_EQ(*v6, *base) << "type " << static_cast<int>(type) << " should reuse non-V6 implementation";
    }
}

}  // namespace Domain
}  // namespace Analysis
