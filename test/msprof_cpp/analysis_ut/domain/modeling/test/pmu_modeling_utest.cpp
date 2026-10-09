/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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
#include <gmock/gmock.h>
#include <vector>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Utils;
using namespace Analysis::Infra;

namespace Analysis {
namespace Domain {
class PmuModelingUTest : public Test {
protected:
    void SetUp() override
    {
        auto data = std::make_shared<std::vector<HalPmuData>>();
        dataInventory_.Inject(data);
        auto flip = std::make_shared<std::vector<HalTrackData>>(std::vector<HalTrackData>{});
        dataInventory_.Inject(flip);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<HalPmuData> GenerateHalPmuData()
{
    std::vector<HalPmuData> pmuData;
    constexpr size_t SEQ_NUM = 15;
    static const uint16_t STREAM_ID_SEQ[SEQ_NUM]{1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3};
    static const uint16_t TASK_ID_SEQ[SEQ_NUM]{1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM]{1000, 1000, 3000, 3000, 10000, 20000, 2000, 2000, 4000, 4000, 20000,
                                                 1000, 2000, 3000, 4000};
    static const HalPmuType PMU_TYPE_SEQ[SEQ_NUM]{PMU, BLOCK_PMU, PMU, BLOCK_PMU, PMU, PMU, PMU, BLOCK_PMU, PMU,
                                                  BLOCK_PMU, PMU, PMU, PMU, PMU, PMU};
    HalPmuData pmuInfo{};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        pmuInfo.type = PMU_TYPE_SEQ[i];
        pmuInfo.hd.taskId.streamId = STREAM_ID_SEQ[i];
        pmuInfo.hd.taskId.taskId = TASK_ID_SEQ[i];
        pmuInfo.hd.taskId.contextId = TASK_ID_SEQ[i];
        pmuInfo.hd.timestamp = TIMESTAMP_SEQ[i];
        pmuData.push_back(pmuInfo);
    }
    return pmuData;
}

static std::vector<HalTrackData> GenerateFlipData()
{
    std::vector<HalTrackData> flipData;
    constexpr size_t SEQ_NUM = 3;
    static const uint16_t STREAM_ID_SEQ[SEQ_NUM]{1, 1, 2};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM]{9900, 19900, 19990};
    HalTrackData flipInfo{};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        flipInfo.type = TS_TASK_FLIP;
        flipInfo.hd.taskId.streamId = STREAM_ID_SEQ[i];
        flipInfo.hd.timestamp = TIMESTAMP_SEQ[i];
        flipData.push_back(flipInfo);
    }
    return flipData;
}

TEST_F(PmuModelingUTest, ShouldSupplyDefaultBatchIdWithoutFlipData)
{
    PmuModeling pmuModeling;
    DeviceContext context;
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    ASSERT_EQ(Analysis::ANALYSIS_OK, pmuModeling.Run(dataInventory_, context));

    auto result = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_TRUE(result);
    ASSERT_EQ(15ul, result->size());
    for (const auto& res : *result) {
        ASSERT_EQ(0, res.hd.taskId.batchId);
    }
}

TEST_F(PmuModelingUTest, ShouldSupplyCorrectBatchIdWithFlipData)
{
    auto pmuData = GenerateHalPmuData();
    auto originPmuData = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    originPmuData->swap(pmuData);

    auto flip = GenerateFlipData();
    auto originFlip = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    originFlip->swap(flip);

    PmuModeling pmuModeling;
    DeviceContext context;

    ASSERT_EQ(Analysis::ANALYSIS_OK, pmuModeling.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_TRUE(result);
    ASSERT_EQ(15ul, result->size());

    constexpr size_t SEQ_NUM = 15;
    // streamId = 1 有两次翻转，streamId = 2 有一次翻转，streamId = 3 没有翻转
    const uint16_t EXCEPT_BATCH_ID[SEQ_NUM]{0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        ASSERT_EQ(EXCEPT_BATCH_ID[i], (*result)[i].hd.taskId.batchId);
    }
}

TEST_F(PmuModelingUTest, ShouldReturnOKWithoutPmuData)
{
    PmuModeling pmuModeling;
    DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, pmuModeling.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    ASSERT_TRUE(result);
    ASSERT_EQ(0ul, result->size());
}
}
}