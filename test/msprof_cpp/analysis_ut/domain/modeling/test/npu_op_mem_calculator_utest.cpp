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

#include "gtest/gtest.h"

#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/domain/services/modeling/npu_op_mem_calculator.h"

using namespace Analysis::Domain::Host;

namespace {
std::shared_ptr<ParserAdditionalInfo> MakeRaw(uint64_t operatorId, uint64_t addr, int64_t size, uint64_t timestamp,
                                             uint32_t deviceId = 0, uint32_t threadId = 1)
{
    auto data = std::make_shared<ParserAdditionalInfo>();
    data->dataLen = sizeof(ParserMemoryInfo);
    data->timeStamp = timestamp;
    data->threadId = threadId;
    data->memoryInfo.nodeId = operatorId;
    data->memoryInfo.addr = addr;
    data->memoryInfo.size = size;
    data->memoryInfo.deviceId = deviceId;
    data->memoryInfo.totalAllocateMemory = timestamp + 100;
    data->memoryInfo.totalReserveMemory = timestamp + 200;
    return data;
}
}  // namespace

TEST(NpuOpMemCalculatorUTest, CalculateShouldMatchAllocationAndReleaseInReleaseOrder)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> rawData{
        MakeRaw(1, 10, 100, 10, 0, 1),
        MakeRaw(2, 20, 200, 20, 0, 1),
        MakeRaw(1, 10, -999, 30, 0, 99),
        MakeRaw(2, 20, -1, 40, 0, 1),
        MakeRaw(3, 30, -1, 50),
        MakeRaw(4, 40, 0, 60),
    };
    std::unordered_map<uint64_t, std::string> hashData{{1, "operator_one"}};

    NpuOpMemCalculationResult result;
    ASSERT_TRUE(NpuOpMemCalculator().Calculate(rawData, hashData, result));

    ASSERT_EQ(result.memoryRecords.size(), rawData.size());
    EXPECT_EQ(result.memoryRecords[0].component, "GE");
    EXPECT_EQ(result.memoryRecords[0].totalReserveMemory, 210U);
    EXPECT_EQ(result.memoryRecords[0].totalAllocateMemory, 110U);
    EXPECT_EQ(result.memoryRecords[0].deviceType, "NPU:0");

    ASSERT_EQ(result.operatorMemory.size(), 2U);
    EXPECT_EQ(result.operatorMemory[0].operatorId, 1U);
    EXPECT_EQ(result.operatorMemory[0].size, 100);
    EXPECT_EQ(result.operatorMemory[0].allocationTime, 10U);
    EXPECT_EQ(result.operatorMemory[0].releaseTime, 30U);
    EXPECT_EQ(result.operatorMemory[0].duration, 20);
    EXPECT_EQ(result.operatorMemory[0].releaseTotalAllocated, 130U);
    EXPECT_EQ(result.operatorMemory[0].name, "operator_one");
    EXPECT_EQ(result.operatorMemory[1].operatorId, 2U);
    EXPECT_TRUE(result.operatorMemory[1].name.empty());
}

TEST(NpuOpMemCalculatorUTest, CalculateShouldSortByTimeAndUseLatestAllocationForDuplicateKey)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> rawData{
        MakeRaw(1, 10, -1, 50),
        MakeRaw(1, 10, 100, 10),
        MakeRaw(2, 20, 200, 20),
        MakeRaw(1, 10, 300, 30),
        MakeRaw(2, 20, -1, 40),
    };

    NpuOpMemCalculationResult result;
    ASSERT_TRUE(NpuOpMemCalculator().Calculate(rawData, {}, result));

    ASSERT_EQ(result.memoryRecords.size(), rawData.size());
    EXPECT_EQ(result.memoryRecords[0].timestamp, 50U);
    EXPECT_EQ(result.memoryRecords[1].timestamp, 10U);

    ASSERT_EQ(result.operatorMemory.size(), 2U);
    EXPECT_EQ(result.operatorMemory[0].operatorId, 2U);
    EXPECT_EQ(result.operatorMemory[0].allocationTime, 20U);
    EXPECT_EQ(result.operatorMemory[0].releaseTime, 40U);
    EXPECT_EQ(result.operatorMemory[0].duration, 20);
    EXPECT_EQ(result.operatorMemory[1].operatorId, 1U);
    EXPECT_EQ(result.operatorMemory[1].size, 300);
    EXPECT_EQ(result.operatorMemory[1].allocationTime, 30U);
    EXPECT_EQ(result.operatorMemory[1].releaseTime, 50U);
    EXPECT_EQ(result.operatorMemory[1].duration, 20);
}

TEST(NpuOpMemCalculatorUTest, CalculateShouldPreserveInputOrderForEqualTimestamps)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> rawData{
        MakeRaw(1, 10, 100, 10),
        MakeRaw(1, 10, 200, 10),
        MakeRaw(1, 10, -1, 20),
    };

    NpuOpMemCalculationResult result;
    ASSERT_TRUE(NpuOpMemCalculator().Calculate(rawData, {}, result));

    ASSERT_EQ(result.operatorMemory.size(), 1U);
    EXPECT_EQ(result.operatorMemory[0].size, 200);
    EXPECT_EQ(result.operatorMemory[0].allocationTime, 10U);
    EXPECT_EQ(result.operatorMemory[0].releaseTime, 20U);
}

TEST(NpuOpMemCalculatorUTest, CalculateShouldKeepUnreleasedDataInInsertionOrderAfterDuplicateUpdate)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> rawData{
        MakeRaw(1, 10, 100, 30, 0),
        MakeRaw(1, 10, 200, 10, 1),
        MakeRaw(2, 20, 300, 20, 0),
        MakeRaw(1, 10, 400, 40, 1),
    };

    NpuOpMemCalculationResult result;
    ASSERT_TRUE(NpuOpMemCalculator().Calculate(rawData, {}, result));

    ASSERT_EQ(result.operatorMemory.size(), 3U);
    EXPECT_EQ(result.operatorMemory[0].deviceType, "NPU:1");
    EXPECT_EQ(result.operatorMemory[0].operatorId, 1U);
    EXPECT_EQ(result.operatorMemory[0].size, 400);
    EXPECT_EQ(result.operatorMemory[0].allocationTime, 40U);
    EXPECT_EQ(result.operatorMemory[1].deviceType, "NPU:0");
    EXPECT_EQ(result.operatorMemory[1].operatorId, 2U);
    EXPECT_EQ(result.operatorMemory[1].allocationTime, 20U);
    EXPECT_EQ(result.operatorMemory[2].deviceType, "NPU:0");
    EXPECT_EQ(result.operatorMemory[2].operatorId, 1U);
    EXPECT_EQ(result.operatorMemory[2].allocationTime, 30U);
}

TEST(NpuOpMemCalculatorUTest, CalculateShouldClampOverflowDurationAndContinue)
{
    const uint64_t overflowReleaseTime = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U;
    std::vector<std::shared_ptr<ParserAdditionalInfo>> rawData{
        MakeRaw(1, 10, 100, 0),
        MakeRaw(2, 20, 200, 1),
        MakeRaw(2, 20, -1, 2),
        MakeRaw(1, 10, -1, overflowReleaseTime),
    };

    NpuOpMemCalculationResult result;
    ASSERT_TRUE(NpuOpMemCalculator().Calculate(rawData, {}, result));

    ASSERT_EQ(result.memoryRecords.size(), rawData.size());
    ASSERT_EQ(result.operatorMemory.size(), 2U);
    EXPECT_EQ(result.operatorMemory[0].operatorId, 2U);
    EXPECT_EQ(result.operatorMemory[0].duration, 1);
    EXPECT_EQ(result.operatorMemory[1].operatorId, 1U);
    EXPECT_EQ(result.operatorMemory[1].releaseTime, overflowReleaseTime);
    EXPECT_EQ(result.operatorMemory[1].duration, std::numeric_limits<int64_t>::max());
}
