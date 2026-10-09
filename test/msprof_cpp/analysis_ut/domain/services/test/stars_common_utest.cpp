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
#include "analysis/csrc/domain/services/parser/parser_item/stars_common.h"

using namespace Analysis::Domain;

TEST(StarCommonUTest, ShouldStreamChangeLow12BitsWhenStreamBit13IsOne)
{
    auto res = StarsCommon::GetStreamId(8195, 8, 0);
    EXPECT_EQ(8, res);
}

TEST(StarCommonUTest, ShouldReturnStreamWhenStreamBit13IsNotOne)
{
    auto res = StarsCommon::GetStreamId(3, 8, 0);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldReturnStreamLow12BitsWhenStreamBit12IsOne)
{
    auto res = StarsCommon::GetStreamId(12291, 8, 0);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldTaskChangeLow12BitsWhenStreamBit13IsOne)
{
    auto res = StarsCommon::GetTaskId(8195, 8, 0);
    EXPECT_EQ(3, res);
}

TEST(StarCommonUTest, ShouldReturnTaskWhenStreamBit13AndBit12IsNotOne)
{
    auto res = StarsCommon::GetTaskId(3, 8, 0);
    EXPECT_EQ(8, res);
}

TEST(StarCommonUTest, ShouldReturnTaskChangeHighThreeBitsWhenStreamBit12IsOne)
{
    auto res = StarsCommon::GetTaskId(20483, 8, 0);
    EXPECT_EQ(16392, res);
}
