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

#include <string>
#include <memory>
#include <vector>
#include <random>
#include "gtest/gtest.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/tree.h"

using namespace Analysis::Domain;

class TreeUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(TreeUTest, GetRootNullptrAndNotNullptr)
{
    EventInfo testInfo{EventType::EVENT_TYPE_API, 0, 0, 0};
    auto eventPtr = std::make_shared<Event>(std::shared_ptr<ParserApi>{},
                                            testInfo);
    auto treeNode = std::make_shared<TreeNode>(eventPtr);

    auto treeNull = std::make_shared<Tree>(nullptr);
    auto checkNullptr = treeNull->GetRoot();
    EXPECT_EQ(nullptr, checkNullptr);

    auto tree = std::make_shared<Tree>(treeNode);
    auto check = tree->GetRoot();
    EXPECT_NE(nullptr, check);
}