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

#include <cstring>
#include <set>
#include <tuple>
#include <vector>

#define private public
#include "analysis/csrc/domain/services/persistence/device/aicpu_persistence.h"
#undef private

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace testing;

namespace Analysis {
namespace Domain {
namespace {
const std::string AICPU_DEVICE_PATH = "./aicpu_device_0";

// =========================================================================
// ProcessAicpuDataByDataType — 数据按类型分发测试
// =========================================================================

class AicpuPersistenceProcessDataUtest : public Test {
protected:
    void SetUp() override
    {
        persistence_ = std::make_shared<AicpuPersistence>();
    }

    AicpuData CreateAicpuData(AicpuType type, uint64_t timeStamp = 1000)
    {
        AicpuData data;
        data.timeStamp = timeStamp;
        data.type = type;
        return data;
    }

protected:
    std::shared_ptr<AicpuPersistence> persistence_;
};

TEST_F(AicpuPersistenceProcessDataUtest, ShouldReturnTrueWhenInputIsEmpty)
{
    std::vector<AicpuData> emptyInput;
    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(emptyInput));

    // 所有类型向量都应为空
    EXPECT_TRUE(persistence_->nodeData_.empty());
    EXPECT_TRUE(persistence_->dpData_.empty());
    EXPECT_TRUE(persistence_->modelData_.empty());
    EXPECT_TRUE(persistence_->miData_.empty());
    EXPECT_TRUE(persistence_->commTurnData_.empty());
    EXPECT_TRUE(persistence_->computeTurnData_.empty());
    EXPECT_TRUE(persistence_->opInfoData_.empty());
    EXPECT_TRUE(persistence_->flipTaskData_.empty());
    EXPECT_TRUE(persistence_->mainStreamTaskData_.empty());
    EXPECT_TRUE(persistence_->kfcInfosData_.empty());
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleNodeToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_NODE)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->nodeData_.size(), 1);
    EXPECT_EQ(persistence_->nodeData_[0].type, AicpuType::AICPU_NODE);
    EXPECT_EQ(persistence_->nodeData_[0].timeStamp, 1000);

    // 其他向量不受影响
    EXPECT_TRUE(persistence_->dpData_.empty());
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleDpToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_DP, 2000)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->dpData_.size(), 1);
    EXPECT_EQ(persistence_->dpData_[0].type, AicpuType::AICPU_DP);
    EXPECT_EQ(persistence_->dpData_[0].timeStamp, 2000);
    EXPECT_TRUE(persistence_->nodeData_.empty());
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleModelToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_MODEL)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->modelData_.size(), 1);
    EXPECT_EQ(persistence_->modelData_[0].type, AicpuType::AICPU_MODEL);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleMiToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_MI)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->miData_.size(), 1);
    EXPECT_EQ(persistence_->miData_[0].type, AicpuType::AICPU_MI);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleCommTurnToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::KFC_COMM_TURN)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->commTurnData_.size(), 1);
    EXPECT_EQ(persistence_->commTurnData_[0].type, AicpuType::KFC_COMM_TURN);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleComputeTurnToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::KFC_COMPUTE_TURN)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->computeTurnData_.size(), 1);
    EXPECT_EQ(persistence_->computeTurnData_[0].type, AicpuType::KFC_COMPUTE_TURN);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleOpInfoToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::HCCL_OP_INFO)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->opInfoData_.size(), 1);
    EXPECT_EQ(persistence_->opInfoData_[0].type, AicpuType::HCCL_OP_INFO);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleFlipTaskToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_FLIP_TASK)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->flipTaskData_.size(), 1);
    EXPECT_EQ(persistence_->flipTaskData_[0].type, AicpuType::AICPU_FLIP_TASK);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleMainStreamTaskToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->mainStreamTaskData_.size(), 1);
    EXPECT_EQ(persistence_->mainStreamTaskData_[0].type, AicpuType::AICPU_MASTER_STREAM_HCCL_TASK);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeSingleKfcHcclInfoToCorrectVector)
{
    std::vector<AicpuData> input = {CreateAicpuData(AicpuType::KFC_HCCL_INFO)};

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->kfcInfosData_.size(), 1);
    EXPECT_EQ(persistence_->kfcInfosData_[0].type, AicpuType::KFC_HCCL_INFO);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeMultipleItemsOfSameType)
{
    std::vector<AicpuData> input;
    for (int i = 0; i < 5; ++i) {
        input.push_back(CreateAicpuData(AicpuType::AICPU_NODE, 1000 + i));
    }

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->nodeData_.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(persistence_->nodeData_[i].timeStamp, 1000 + i);
        EXPECT_EQ(persistence_->nodeData_[i].type, AicpuType::AICPU_NODE);
    }
    // 其他向量不受影响
    EXPECT_TRUE(persistence_->dpData_.empty());
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeMixedTypesCorrectly)
{
    // 混合 10 种类型，每种 2 条
    std::vector<AicpuData> input;
    input.push_back(CreateAicpuData(AicpuType::AICPU_NODE, 1));
    input.push_back(CreateAicpuData(AicpuType::AICPU_DP, 2));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MODEL, 3));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MI, 4));
    input.push_back(CreateAicpuData(AicpuType::KFC_COMM_TURN, 5));
    input.push_back(CreateAicpuData(AicpuType::KFC_COMPUTE_TURN, 6));
    input.push_back(CreateAicpuData(AicpuType::HCCL_OP_INFO, 7));
    input.push_back(CreateAicpuData(AicpuType::AICPU_FLIP_TASK, 8));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 9));
    input.push_back(CreateAicpuData(AicpuType::KFC_HCCL_INFO, 10));
    // 第二组
    input.push_back(CreateAicpuData(AicpuType::AICPU_NODE, 11));
    input.push_back(CreateAicpuData(AicpuType::AICPU_DP, 12));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MODEL, 13));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MI, 14));
    input.push_back(CreateAicpuData(AicpuType::KFC_COMM_TURN, 15));
    input.push_back(CreateAicpuData(AicpuType::KFC_COMPUTE_TURN, 16));
    input.push_back(CreateAicpuData(AicpuType::HCCL_OP_INFO, 17));
    input.push_back(CreateAicpuData(AicpuType::AICPU_FLIP_TASK, 18));
    input.push_back(CreateAicpuData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 19));
    input.push_back(CreateAicpuData(AicpuType::KFC_HCCL_INFO, 20));

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));

    // 验证每种类型各有 2 条，且顺序保持
    EXPECT_EQ(persistence_->nodeData_.size(), 2);
    EXPECT_EQ(persistence_->nodeData_[0].timeStamp, 1);
    EXPECT_EQ(persistence_->nodeData_[1].timeStamp, 11);

    EXPECT_EQ(persistence_->dpData_.size(), 2);
    EXPECT_EQ(persistence_->dpData_[0].timeStamp, 2);
    EXPECT_EQ(persistence_->dpData_[1].timeStamp, 12);

    EXPECT_EQ(persistence_->modelData_.size(), 2);
    EXPECT_EQ(persistence_->miData_.size(), 2);
    EXPECT_EQ(persistence_->commTurnData_.size(), 2);
    EXPECT_EQ(persistence_->computeTurnData_.size(), 2);
    EXPECT_EQ(persistence_->opInfoData_.size(), 2);
    EXPECT_EQ(persistence_->flipTaskData_.size(), 2);
    EXPECT_EQ(persistence_->mainStreamTaskData_.size(), 2);
    EXPECT_EQ(persistence_->kfcInfosData_.size(), 2);
}

TEST_F(AicpuPersistenceProcessDataUtest, ShouldDistributeInInputOrder)
{
    // 同一类型多条数据应按输入顺序保存
    std::vector<AicpuData> input = {
        CreateAicpuData(AicpuType::AICPU_NODE, 100),
        CreateAicpuData(AicpuType::AICPU_DP, 200),
        CreateAicpuData(AicpuType::AICPU_NODE, 300),
        CreateAicpuData(AicpuType::AICPU_DP, 400),
    };

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->nodeData_.size(), 2);
    EXPECT_EQ(persistence_->nodeData_[0].timeStamp, 100);
    EXPECT_EQ(persistence_->nodeData_[1].timeStamp, 300);
    EXPECT_EQ(persistence_->dpData_.size(), 2);
    EXPECT_EQ(persistence_->dpData_[0].timeStamp, 200);
    EXPECT_EQ(persistence_->dpData_[1].timeStamp, 400);
}

// =========================================================================
// AicpuData 结构体测试
// =========================================================================

TEST(AicpuDataStructTest, ShouldDefaultInitializeTaskIdWithInvalidContextId)
{
    AicpuData data;
    // TaskId 默认构造中 contextId = INVALID_CONTEXT_ID
    EXPECT_EQ(data.taskId.contextId, INVALID_CONTEXT_ID);
}

TEST(AicpuDataStructTest, ShouldSupportSettingAndReadingNodeType)
{
    AicpuData data;
    data.type = AicpuType::AICPU_NODE;
    data.node.runStartTick = 1000;
    data.node.runEndTick = 2000;

    EXPECT_EQ(data.type, AicpuType::AICPU_NODE);
    EXPECT_EQ(data.node.runStartTick, 1000);
    EXPECT_EQ(data.node.runEndTick, 2000);
}

// =========================================================================
// AicpuPersistence 构造/析构测试
// =========================================================================

TEST(AicpuPersistenceLifecycleTest, ShouldInitializeAllMemberVectorsEmpty)
{
    AicpuPersistence persistence;

    EXPECT_TRUE(persistence.nodeData_.empty());
    EXPECT_TRUE(persistence.dpData_.empty());
    EXPECT_TRUE(persistence.modelData_.empty());
    EXPECT_TRUE(persistence.miData_.empty());
    EXPECT_TRUE(persistence.commTurnData_.empty());
    EXPECT_TRUE(persistence.computeTurnData_.empty());
    EXPECT_TRUE(persistence.opInfoData_.empty());
    EXPECT_TRUE(persistence.flipTaskData_.empty());
    EXPECT_TRUE(persistence.mainStreamTaskData_.empty());
    EXPECT_TRUE(persistence.kfcInfosData_.empty());
}

// =========================================================================
// ProcessEntry 测试
// =========================================================================

class AicpuPersistenceProcessEntryUtest : public Test {
protected:
    void SetUp() override
    {
        persistence_ = std::make_shared<AicpuPersistence>();
    }

protected:
    std::shared_ptr<AicpuPersistence> persistence_;
    Infra::DataInventory dataInventory_;
    DeviceContext deviceContext_;
};

TEST_F(AicpuPersistenceProcessEntryUtest, ShouldReturnErrorWhenAicpuDataIsNull)
{
    // 不注入任何数据，aicpuData 为 nullptr
    auto ret = persistence_->ProcessEntry(dataInventory_, deviceContext_);
    EXPECT_EQ(ret, ANALYSIS_ERROR);
}

TEST_F(AicpuPersistenceProcessEntryUtest, ShouldReturnErrorWhenDeviceStreamInfoIsNull)
{
    auto aicpuData = std::make_shared<std::vector<AicpuData>>();
    dataInventory_.Inject(aicpuData);
    // deviceStreamInfo 缺失

    auto ret = persistence_->ProcessEntry(dataInventory_, deviceContext_);
    EXPECT_EQ(ret, ANALYSIS_ERROR);
}

TEST_F(AicpuPersistenceProcessEntryUtest, ShouldReturnErrorWhenHostStreamInfoIsNull)
{
    auto aicpuData = std::make_shared<std::vector<AicpuData>>();
    auto deviceStreamInfo = std::make_shared<DeviceStreamInfo>();
    dataInventory_.Inject(aicpuData);
    dataInventory_.Inject(deviceStreamInfo);
    // hostStreamInfo 缺失

    auto ret = persistence_->ProcessEntry(dataInventory_, deviceContext_);
    EXPECT_EQ(ret, ANALYSIS_ERROR);
}

TEST_F(AicpuPersistenceProcessEntryUtest, ShouldReturnErrorWhenGeHashMapIsNull)
{
    auto aicpuData = std::make_shared<std::vector<AicpuData>>();
    auto deviceStreamInfo = std::make_shared<DeviceStreamInfo>();
    auto hostStreamInfo = std::make_shared<HostStreamInfo>();
    dataInventory_.Inject(aicpuData);
    dataInventory_.Inject(deviceStreamInfo);
    dataInventory_.Inject(hostStreamInfo);
    // geHashMap 缺失

    auto ret = persistence_->ProcessEntry(dataInventory_, deviceContext_);
    EXPECT_EQ(ret, ANALYSIS_ERROR);
}

// =========================================================================
// ProcessAicpuDataByDataType 边界测试
// =========================================================================

TEST_F(AicpuPersistenceProcessDataUtest, ShouldHandleHcclOpInfoEnumGap)
{
    // HCCL_OP_INFO = 10，与 AICPU_MI(=3) 之间有空隙，验证分发不受枚举值不连续影响
    std::vector<AicpuData> input = {
        CreateAicpuData(AicpuType::AICPU_MI, 1),
        CreateAicpuData(AicpuType::HCCL_OP_INFO, 2),
    };

    EXPECT_TRUE(persistence_->ProcessAicpuDataByDataType(input));
    EXPECT_EQ(persistence_->miData_.size(), 1);
    EXPECT_EQ(persistence_->miData_[0].timeStamp, 1);
    EXPECT_EQ(persistence_->opInfoData_.size(), 1);
    EXPECT_EQ(persistence_->opInfoData_[0].timeStamp, 2);
}


class AicpuPersistenceComputeBatchIdUtest : public Test {
protected:
    void SetUp() override
    {
        persistence_ = std::make_shared<AicpuPersistence>();
    }

    // 创建一个 timestamp=ts, streamId=s, taskId=0 的 AicpuData
    AicpuData MakeTaskData(AicpuType type, uint64_t timestamp, uint16_t streamId, uint32_t taskId = 0)
    {
        AicpuData data;
        data.timeStamp = timestamp;
        data.type = type;
        data.taskId.streamId = streamId;
        data.taskId.taskId = taskId;
        data.taskId.batchId = 0;
        return data;
    }

    // 创建 flip 数据：timestamp + streamId + flipNum
    AicpuData MakeFlipData(uint64_t timestamp, uint16_t streamId, uint32_t flipNum)
    {
        AicpuData data;
        data.timeStamp = timestamp;
        data.type = AicpuType::AICPU_FLIP_TASK;
        data.taskId.streamId = streamId;
        data.flipTask.flipNum = flipNum;
        return data;
    }

protected:
    std::shared_ptr<AicpuPersistence> persistence_;
};

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldReturnImmediatelyWhenFlipDataIsEmpty)
{
    persistence_->mainStreamTaskData_ = {MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 100, 1)};
    // flipTaskData_ 为空

    persistence_->ComputeAicpuBatchId();

    // batchId 应保持默认值 0
    EXPECT_EQ(persistence_->mainStreamTaskData_[0].taskId.batchId, 0);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldReturnImmediatelyWhenTaskDataIsEmpty)
{
    persistence_->flipTaskData_ = {MakeFlipData(100, 1, 0)};
    // mainStreamTaskData_ 和 kfcInfosData_ 为空

    persistence_->ComputeAicpuBatchId();

    EXPECT_EQ(persistence_->flipTaskData_[0].taskId.batchId, 0);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldNotComputeBatchIdWhenStreamHasNoFlip)
{
    // stream 1 有 tasks 但没有 flip
    persistence_->mainStreamTaskData_ = {MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 200, 1)};
    // flip 在 stream 2
    persistence_->flipTaskData_ = {MakeFlipData(100, 2, 0)};

    persistence_->ComputeAicpuBatchId();

    // stream 1 没有对应的 flip，batchId 不计算
    EXPECT_EQ(persistence_->mainStreamTaskData_[0].taskId.batchId, 0);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldComputeCorrectBatchIdForMainStreamTasks)
{
    // stream 1: 3 个 task，2 个 flip → batchId 应被分为 2 组
    // flip1@100: tasks before here → batch 0
    // flip2@300: tasks between 100-300 → batch 1
    // tasks after 300 → batch 2
    persistence_->mainStreamTaskData_ = {
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 50, 1),   // before flip1
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 200, 1),  // between flip1-flip2
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 400, 1),  // after flip2
    };
    persistence_->flipTaskData_ = {
        MakeFlipData(100, 1, 0),
        MakeFlipData(300, 1, 0),
    };

    persistence_->ComputeAicpuBatchId();

    EXPECT_EQ(persistence_->mainStreamTaskData_[0].taskId.batchId, 0);  // ts=50  < flip@100
    EXPECT_EQ(persistence_->mainStreamTaskData_[1].taskId.batchId, 1);  // ts=200 between flips
    EXPECT_EQ(persistence_->mainStreamTaskData_[2].taskId.batchId, 2);  // ts=400 > flip@300
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldComputeBatchIdIndependentlyPerStream)
{
    // stream 1: 1 flip@100, 2 tasks (50, 200)
    // stream 2: 1 flip@500, 2 tasks (300, 600)
    persistence_->mainStreamTaskData_ = {
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 50, 1),
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 200, 1),
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 300, 2),
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 600, 2),
    };
    persistence_->flipTaskData_ = {
        MakeFlipData(100, 1, 0),
        MakeFlipData(500, 2, 0),
    };

    persistence_->ComputeAicpuBatchId();

    // stream 1: ts=50<100(batch0), ts=200>100(batch1)
    EXPECT_EQ(persistence_->mainStreamTaskData_[0].taskId.batchId, 0);
    EXPECT_EQ(persistence_->mainStreamTaskData_[1].taskId.batchId, 1);
    // stream 2: ts=300<500(batch0), ts=600>500(batch1)
    EXPECT_EQ(persistence_->mainStreamTaskData_[2].taskId.batchId, 0);
    EXPECT_EQ(persistence_->mainStreamTaskData_[3].taskId.batchId, 1);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldComputeBatchIdForKfcInfos)
{
    // stream 1: 1 flip@200, 2 kfc infos entries in 1 AicpuData
    persistence_->flipTaskData_ = {MakeFlipData(200, 1, 0)};

    AicpuData kfc = MakeTaskData(AicpuType::KFC_HCCL_INFO, 100, 1);
    kfc.KfcInfos.infos[0].timeStamp = 100;
    kfc.KfcInfos.infos[0].streamId = 1;
    kfc.KfcInfos.infos[0].groupName = 1;
    kfc.KfcInfos.infos[0].taskId = 10;
    kfc.KfcInfos.infos[1].timeStamp = 300;
    kfc.KfcInfos.infos[1].streamId = 1;
    kfc.KfcInfos.infos[1].groupName = 1;
    kfc.KfcInfos.infos[1].taskId = 20;
    persistence_->kfcInfosData_ = {kfc};

    persistence_->ComputeAicpuBatchId();

    // 两条 info 共享 AicpuData.taskId.batchId，timestamp 大的(300)后写入
    // info[0]@100 → flip@200 → batch 0，info[1]@300 → batch 1
    // 最终 kfc.taskId.batchId = 1（后写入覆盖）
    EXPECT_EQ(persistence_->kfcInfosData_[0].taskId.batchId, 1);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldSkipKfcInfoWithZeroGroupName)
{
    persistence_->flipTaskData_ = {MakeFlipData(200, 1, 0)};

    AicpuData kfc = MakeTaskData(AicpuType::KFC_HCCL_INFO, 100, 1);
    kfc.KfcInfos.infos[0].timeStamp = 100;
    kfc.KfcInfos.infos[0].streamId = 1;
    kfc.KfcInfos.infos[0].groupName = 0;  // 无效，应跳过
    kfc.KfcInfos.infos[1].timeStamp = 300;
    kfc.KfcInfos.infos[1].streamId = 1;
    kfc.KfcInfos.infos[1].groupName = 1;
    persistence_->kfcInfosData_ = {kfc};

    persistence_->ComputeAicpuBatchId();

    // 只有 info[1] 参与计算，ts=300 > flip@200 → batch 1
    EXPECT_EQ(persistence_->kfcInfosData_[0].taskId.batchId, 1);
}

TEST_F(AicpuPersistenceComputeBatchIdUtest, ShouldComputeBothMainStreamAndKfcTogether)
{
    // 同 stream 1: 1 flip@200, 1 mainStream task@100, 1 kfc info@300
    persistence_->flipTaskData_ = {MakeFlipData(200, 1, 0)};

    persistence_->mainStreamTaskData_ = {
        MakeTaskData(AicpuType::AICPU_MASTER_STREAM_HCCL_TASK, 100, 1),
    };

    AicpuData kfc = MakeTaskData(AicpuType::KFC_HCCL_INFO, 300, 1);
    kfc.KfcInfos.infos[0].timeStamp = 300;
    kfc.KfcInfos.infos[0].streamId = 1;
    kfc.KfcInfos.infos[0].groupName = 1;
    persistence_->kfcInfosData_ = {kfc};

    persistence_->ComputeAicpuBatchId();

    // mainStream@100 < flip@200 → batch 0
    EXPECT_EQ(persistence_->mainStreamTaskData_[0].taskId.batchId, 0);
    // kfc@300 > flip@200 → batch 1
    EXPECT_EQ(persistence_->kfcInfosData_[0].taskId.batchId, 1);
}

// =========================================================================
// GenerateAndSaveNode / GenerateAndSaveDp — 落盘 db 文件名与表名测试
// 校验 AiCpuData/AiCpuDp 数据落到 ai_cpu.db（而非 aicpu.db），与 Python 侧对齐
// =========================================================================

class AicpuPersistenceSaveUtest : public Test {
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(AICPU_DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({AICPU_DEVICE_PATH, "sqlite"})));
        persistence_ = std::make_shared<AicpuPersistence>();
        // freq=1000, sysCnt=1000, hostMonotonic=1000，时间换算结果确定无溢出
        persistence_->params_ = SyscntConversionParams(1000.0, 1000, 1000);
    }
    void TearDown() override
    {
        EXPECT_TRUE(File::RemoveDir(AICPU_DEVICE_PATH, 0));
    }

    AicpuData CreateNodeData()
    {
        AicpuData data;
        data.type = AicpuType::AICPU_NODE;
        data.taskId.streamId = 1;
        data.taskId.taskId = 10;
        data.node.runStartTick = 1000;
        data.node.runEndTick = 2000;
        data.node.computeStartTime = 1000;
        data.node.memcpyStartTime = 1200;
        data.node.memcpyEndTime = 1500;
        data.node.dispatchTime = 300;
        data.node.submitTick = 900;
        data.node.tickAfterRun = 2100;
        return data;
    }

    AicpuData CreateDpData()
    {
        AicpuData data;
        data.type = AicpuType::AICPU_DP;
        data.timeStamp = 500;
        memcpy(data.dp.action, "MALLOC", sizeof("MALLOC"));
        memcpy(data.dp.source, "HOST", sizeof("HOST"));
        data.dp.size = 1024;
        return data;
    }

    std::string GetSqliteDbPath() const
    {
        return File::PathJoin({AICPU_DEVICE_PATH, "sqlite", "ai_cpu.db"});
    }

    std::string GetOldSqliteDbPath() const
    {
        return File::PathJoin({AICPU_DEVICE_PATH, "sqlite", "aicpu.db"});
    }

protected:
    std::shared_ptr<AicpuPersistence> persistence_;
};

TEST_F(AicpuPersistenceSaveUtest, ShouldSaveNodeToAiCpuDb)
{
    persistence_->nodeData_.emplace_back(CreateNodeData());

    ASSERT_EQ(ANALYSIS_OK, persistence_->GenerateAndSaveNode(AICPU_DEVICE_PATH));

    // 落盘文件名应为 ai_cpu.db，而不是旧的 aicpu.db
    std::string dbPath = GetSqliteDbPath();
    EXPECT_TRUE(File::Exist(dbPath));
    EXPECT_FALSE(File::Exist(GetOldSqliteDbPath()));

    DBRunner dbRunner(dbPath);
    EXPECT_TRUE(dbRunner.CheckTableExists("AiCpuData"));
    using NodeRow = std::tuple<uint32_t, uint16_t, double, double, std::string, uint64_t, uint64_t, double, uint64_t,
                               double>;
    std::vector<NodeRow> rows;
    EXPECT_TRUE(dbRunner.QueryData("SELECT * FROM AiCpuData", rows));
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<0>(rows[0]), 1);   // stream_id
    EXPECT_EQ(std::get<1>(rows[0]), 10);  // task_id
    EXPECT_EQ(std::get<4>(rows[0]), "");  // node_name
}

TEST_F(AicpuPersistenceSaveUtest, ShouldSaveDpToAiCpuDb)
{
    persistence_->dpData_.emplace_back(CreateDpData());

    ASSERT_EQ(ANALYSIS_OK, persistence_->GenerateAndSaveDp(AICPU_DEVICE_PATH));

    // 落盘文件名应为 ai_cpu.db，而不是旧的 aicpu.db
    std::string dbPath = GetSqliteDbPath();
    EXPECT_TRUE(File::Exist(dbPath));
    EXPECT_FALSE(File::Exist(GetOldSqliteDbPath()));

    // 表名应与 Python 侧 TABLE_AI_CPU_DP("AiCpuDP") 完全对齐
    DBRunner dbRunner(dbPath);
    std::vector<std::tuple<std::string>> tableNames;
    EXPECT_TRUE(dbRunner.QueryData("SELECT name FROM sqlite_master WHERE type='table'", tableNames));
    bool foundAiCpuDP = false;
    for (const auto& name : tableNames) {
        if (std::get<0>(name) == "AiCpuDP") {
            foundAiCpuDP = true;
        }
    }
    EXPECT_TRUE(foundAiCpuDP);

    using DpRow = std::tuple<double, std::string, std::string, uint64_t>;
    std::vector<DpRow> rows;
    EXPECT_TRUE(dbRunner.QueryData("SELECT * FROM AiCpuDP", rows));
    ASSERT_EQ(rows.size(), 1);
    EXPECT_EQ(std::get<1>(rows[0]), "MALLOC");  // action
    EXPECT_EQ(std::get<2>(rows[0]), "HOST");    // source
    EXPECT_EQ(std::get<3>(rows[0]), 1024);      // buffer_size
}

TEST_F(AicpuPersistenceSaveUtest, ShouldOnlyWriteAiCpuDbFileWhenSaveBothNodeAndDp)
{
    persistence_->nodeData_.emplace_back(CreateNodeData());
    persistence_->dpData_.emplace_back(CreateDpData());

    ASSERT_EQ(ANALYSIS_OK, persistence_->GenerateAndSaveNode(AICPU_DEVICE_PATH));
    ASSERT_EQ(ANALYSIS_OK, persistence_->GenerateAndSaveDp(AICPU_DEVICE_PATH));

    // Node 和 Dp 共用同一个 ai_cpu.db，且不残留 aicpu.db
    EXPECT_TRUE(File::Exist(GetSqliteDbPath()));
    EXPECT_FALSE(File::Exist(GetOldSqliteDbPath()));

    DBRunner dbRunner(GetSqliteDbPath());
    EXPECT_TRUE(dbRunner.CheckTableExists("AiCpuData"));
    EXPECT_TRUE(dbRunner.CheckTableExists("AiCpuDP"));
}


}  // namespace
}  // namespace Domain
}  // namespace Analysis
