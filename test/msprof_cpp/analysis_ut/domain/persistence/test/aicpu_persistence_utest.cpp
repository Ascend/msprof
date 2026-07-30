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

#include <set>
#include <vector>

#define private public
#include "analysis/csrc/domain/services/persistence/device/aicpu_persistence.h"
#undef private

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;

namespace Analysis {
namespace Domain {
namespace {

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


}  // namespace
}  // namespace Domain
}  // namespace Analysis
