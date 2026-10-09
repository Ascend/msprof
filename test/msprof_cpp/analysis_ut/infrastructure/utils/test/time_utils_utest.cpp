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

#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

using namespace Analysis::Utils;

class TimeUtilsUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TimeUtilsUTest, TestGetTimeFromSyscntShouldReturnOriSysCntWhenFreqIsDefault)
{
    SyscntConversionParams params{DEFAULT_FREQ, 3666503140109, 36471130547330};
    uint64_t taskSysCnt = 3666510676667;
    double expectRes = 3666510676667.000;
    EXPECT_DOUBLE_EQ(GetTimeFromHostCnt(taskSysCnt, params).Double(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetTimeFromSyscntShouldReturnTimestampWhenSyscntGraterBaseThenTimePlus)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    uint64_t taskSysCnt = 3666510676667;
    double expectRes = 36471205912910.000;
    EXPECT_DOUBLE_EQ(GetTimeFromSyscnt(taskSysCnt, params).Double(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetTimeFromSyscntShouldReturnTimestampWhenSyscntSmallerBaseThenTimeMinus)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    uint64_t taskSysCnt = 3666503100000;
    double expectRes = 36471130146240.000;
    EXPECT_DOUBLE_EQ(GetTimeFromSyscnt(taskSysCnt, params).Double(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetLocalTimeShouldReturnLocalTimestamp)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    ProfTimeRecord record{1701069324370978000, 1701069338159976000, 36471129942580};
    uint64_t taskSysCnt = 3666510676667;
    std::string expectRes = "1701069324446948330";
    auto timestamp = GetTimeFromSyscnt(taskSysCnt, params);
    EXPECT_EQ(GetLocalTime(timestamp, record).Str(), expectRes);
}

TEST_F(TimeUtilsUTest, TestGetDurTimeFromSyscntShouldReturnTimestamp)
{
    SyscntConversionParams params{100.0, 3666503140109, 36471130547330};
    uint64_t sysCnt = 36665;
    uint64_t expectRes = 366650;
    EXPECT_EQ(expectRes, GetDurTimeFromSyscnt(sysCnt, params).Uint64());
}