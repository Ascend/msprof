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

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
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

}  // namespace Domain
}  // namespace Analysis
