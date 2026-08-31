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

#include <vector>
#include <gtest/gtest.h>
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

using namespace testing;
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
const uint16_t DEFAULT_STREAM_ID = 1;

class HalTrackUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        std::vector<HalTrackData> vec{
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::TS_TASK_FLIP),
            createHalTrackData(HalTrackType::INVALID_TYPE),
        };
        std::shared_ptr<std::vector<HalTrackData>> data;
        MAKE_SHARED0_NO_OPERATION(data, std::vector<HalTrackData>, std::move(vec));
        dataInventory_.Inject(data);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }

    HalTrackData createHalTrackData(HalTrackType type)
    {
        HalTrackData ans = {};
        ans.hd.taskId.streamId = DEFAULT_STREAM_ID;
        ans.type = type;
        ans.flip = {};
        return ans;
    }
};

TEST_F(HalTrackUTest, ShouldReturnFlipData)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = GetFlipData(*data);
    ASSERT_EQ(1ul, result.size());
    ASSERT_EQ(HalTrackType::TS_TASK_FLIP, result[DEFAULT_STREAM_ID][0]->type);
}

TEST_F(HalTrackUTest, ShouldReturnFlipBeanWhenInputTaskFlipBean)
{
    auto data = dataInventory_.GetPtr<std::vector<HalTrackData>>();
    auto result = GetTrackDataByType(*data, HalTrackType::TS_TASK_FLIP);
    ASSERT_EQ(3ul, result.size());
    ASSERT_EQ(HalTrackType::TS_TASK_FLIP, result[0].type);
}

TEST_F(HalTrackUTest, ShouldGenerateStepTimeFromStepTraceRecordsOnly)
{
    HalTrackData start{};
    start.type = STEP_TRACE;
    start.stepTrace.indexId = 1;
    start.stepTrace.modelId = 2;
    start.stepTrace.timestamp = 10;
    start.stepTrace.tagId = 60000;
    HalTrackData end = start;
    end.stepTrace.timestamp = 20;
    end.stepTrace.tagId = 60001;

    HalTrackData fakeStart{};
    fakeStart.type = TS_TASK_FLIP;
    fakeStart.stepTrace.indexId = 3;
    fakeStart.stepTrace.timestamp = 11;
    fakeStart.stepTrace.tagId = 60000;
    HalTrackData fakeEnd = fakeStart;
    fakeEnd.type = TS_TASK_TYPE;
    fakeEnd.stepTrace.timestamp = 21;
    fakeEnd.stepTrace.tagId = 60001;

    std::vector<HalTrackData> data{fakeEnd, end, fakeStart, start};
    auto result = GenerateStepTime(data);
    ASSERT_EQ(1ul, result.size());
    EXPECT_EQ(1u, std::get<0>(result.front()));
    EXPECT_EQ(2ul, std::get<1>(result.front()));
    EXPECT_EQ(10ul, std::get<2>(result.front()));
    EXPECT_EQ(20ul, std::get<3>(result.front()));
}
