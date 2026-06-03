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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/adapter/flip.h"

using namespace Analysis::Domain::Adapter;

class FlipUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }

    virtual void TearDown()
    {
    }

    template<class T>
    static std::vector<std::shared_ptr<T>> CreateData(const uint32_t &dataNum)
    {
        std::vector<std::shared_ptr<T>> data (dataNum);
        for (uint32_t i = 0; i < dataNum; ++i) {
            data[i] = std::make_shared<T>();
        }
        return data;
    }

    static void SetTaskTrack(std::shared_ptr<MsprofCompactInfo> &taskTrack,
                             const uint64_t &timeStamp, const MsprofRuntimeTrack &runtimeTrack)
    {
        taskTrack->timeStamp = timeStamp;
        taskTrack->data.runtimeTrack = runtimeTrack;
    }

    static void CheckBatchId(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack, uint16_t dataNum,
                             const std::vector<uint16_t> &batchIds)
    {
        ASSERT_EQ(dataNum, taskTrack.size());
        ASSERT_EQ(dataNum, batchIds.size());
        for (uint32_t i = 0; i < dataNum; ++i) {
            EXPECT_EQ(batchIds[i], Flip::GetBatchId(*taskTrack[i]));
        }
    }
};

TEST_F(FlipUTest, TestComputeBatchIdShouldReturnEmptyWhenTaskTrackIsEmpty)
{
    std::vector<std::shared_ptr<MsprofCompactInfo>> taskTrack {};
    std::vector<std::shared_ptr<FlipTask>> flipTask {};
    Flip::ComputeBatchId(taskTrack, flipTask);
    EXPECT_EQ(0, taskTrack.size());
}

TEST_F(FlipUTest, TestComputeBatchIdShoulSetBatchId0WhenFlipDataIsEmpty)
{
    const uint16_t dataNum = 2;
    auto taskTrack = CreateData<MsprofCompactInfo>(dataNum);
    // runtimeTrack: deviceId, streamId, (batchId, taskId), taskType
    taskTrack[0]->data.runtimeTrack = {0, 1, 0x00010001, 1};
    taskTrack[1]->data.runtimeTrack = {0, 1, 0x00010002, 1};
    std::vector<std::shared_ptr<FlipTask>> flipTask {};
    Flip::ComputeBatchId(taskTrack, flipTask);
    ASSERT_EQ(dataNum, taskTrack.size());
    EXPECT_EQ(0, Flip::GetBatchId(*taskTrack[0]));
    EXPECT_EQ(0, Flip::GetBatchId(*taskTrack[1]));
}

TEST_F(FlipUTest, TestComputeBatchIdShouldSetBatchIdWithEachStreamWhenFlipSepTaskData)
{
    const uint32_t compactInfoDataNum = 10;
    const uint32_t flipDataNum = 4;
    auto taskTrack = CreateData<MsprofCompactInfo>(compactInfoDataNum);
    auto flipData = CreateData<FlipTask>(flipDataNum);
    // runtimeTrack: deviceId, streamId, (batchId, taskId), taskType
    // deviceId 0, streamId 1
    SetTaskTrack(taskTrack[0], 111111, {0, 1, 0x00000001, 1});  // 第0个数据，timeStamp=111111
    SetTaskTrack(taskTrack[1], 111112, {0, 1, 0x00000002, 1});  // 第1个数据，timeStamp=111112
    SetTaskTrack(taskTrack[2], 111116, {0, 1, 0x0000FFFE, 1});  // 第2个数据，timeStamp=111116
    *flipData[0] = {0, 1, 0, 1, 111118};  // 第0个flip
    SetTaskTrack(taskTrack[3], 111120, {0, 1, 0x00000001, 1});  // 第3个数据，timeStamp=111120
    SetTaskTrack(taskTrack[4], 111130, {0, 1, 0x00000002, 1});  // 第4个数据，timeStamp=111130
    *flipData[1] = {0, 1, 3, 65535, 111140};  // stream destroy, 第1个flip
    // deviceId 0, streamId 2
    SetTaskTrack(taskTrack[5], 111130, {0, 2, 0x00000001, 1});  // 第5个数据，timeStamp=111130
    SetTaskTrack(taskTrack[6], 111131, {0, 2, 0x0000FFFE, 1});  // 第6个数据，timeStamp=111131
    *flipData[2] = {0, 2, 0, 1, 111132};  // 第2个flip
    SetTaskTrack(taskTrack[7], 111140, {0, 2, 0x00000001, 1});  // 第7个数据，timeStamp=111140
    *flipData[3] = {0, 2, 3, 65535, 111141};  // stream destroy, 第3个flip
    SetTaskTrack(taskTrack[8], 111150, {0, 2, 0x00000001, 1});  // 第8个数据，timeStamp=111150
    SetTaskTrack(taskTrack[9], 111160, {0, 2, 0x00000002, 1});  // 第9个数据，timeStamp=111160

    Flip::ComputeBatchId(taskTrack, flipData);
    CheckBatchId(taskTrack, compactInfoDataNum,
                 {0, 0, 0, 1, 1,
                  0, 0, 1, 2, 2});
}

TEST_F(FlipUTest, TestComputeBatchIdShoulSetBatchIdWhenFlipTaskIdIsNot0)
{
    const uint32_t compactInfoDataNum = 12;
    const uint32_t flipDataNum = 3;
    auto taskTrack = CreateData<MsprofCompactInfo>(compactInfoDataNum);
    auto flipData = CreateData<FlipTask>(flipDataNum);
    // deviceId 0, streamId 1
    SetTaskTrack(taskTrack[0], 111111, {0, 1, 0x00000001, 1});  // 第0个数据，timeStamp=111111
    SetTaskTrack(taskTrack[1], 111112, {0, 1, 0x00000002, 1});  // 第1个数据，timeStamp=111112
    SetTaskTrack(taskTrack[2], 111116, {0, 1, 0x0000FFFE, 1});  // 第2个数据，timeStamp=111116
    // real flip 0
    SetTaskTrack(taskTrack[3], 111120, {0, 1, 0x00000000, 1});  // 第3个数据，timeStamp=111120
    SetTaskTrack(taskTrack[4], 111130, {0, 1, 0x00000001, 1});  // 第4个数据，timeStamp=111130
    *flipData[0] = {0, 1, 2, 1, 111135};  // 第0个flip, taskId=2
    SetTaskTrack(taskTrack[5], 111140, {0, 1, 0x00000003, 1});  // 第5个数据，timeStamp=111140
    SetTaskTrack(taskTrack[6], 111141, {0, 1, 0x00000004, 1});  // 第6个数据，timeStamp=111141
    SetTaskTrack(taskTrack[7], 111144, {0, 1, 0x0000FFFE, 1});  // 第7个数据，timeStamp=111144
    // real flip 1
    SetTaskTrack(taskTrack[8], 111150, {0, 1, 0x00000002, 1});  // 第8个数据，timeStamp=111150
    SetTaskTrack(taskTrack[9], 111160, {0, 1, 0x00000003, 1});  // 第9个数据，timeStamp=111160
    *flipData[1] = {0, 1, 6, 1, 111165};  // 第1个flip, taskId=6
    SetTaskTrack(taskTrack[10], 111170, {0, 1, 0x00000009, 1});  // 第10个数据，timeStamp=111170
    *flipData[2] = {0, 1, 10, 65535, 111175};  // 第2个flip, taskId=10
    SetTaskTrack(taskTrack[11], 111190, {0, 1, 0x00000001, 1});  // 第11个数据，timeStamp=111190

    Flip::ComputeBatchId(taskTrack, flipData);
    CheckBatchId(taskTrack, compactInfoDataNum, {0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3});
}
