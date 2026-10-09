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
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
using namespace Analysis::Infra;
using namespace testing;

TEST(SyncUtilsUTest, ShouldGetSameTimeStrWhenMultiCallFunc)
{
    // 首次调用
    auto resStr1 = GetTimeStampStr();
    EXPECT_EQ(14ul, resStr1.size());

    // 再次调用
    auto resStr2 = GetTimeStampStr();
    EXPECT_EQ(resStr1, resStr2);
}