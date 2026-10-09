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
#include <memory>
#include <vector>
#include <string>
#include <gtest/gtest.h>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

using namespace std;
using namespace testing;
using namespace Analysis;
using namespace Infra;

class DataInventoryUTest : public Test {
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
    }
    DataInventory dataInventory_;
};

TEST_F(DataInventoryUTest, ShouldFailedWhenInputNullptr)
{
    std::shared_ptr<int> null;
    auto ret = dataInventory_.Inject(null);
    ASSERT_FALSE(ret);

    null.reset(new int);
    ret = dataInventory_.Inject(null);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(dataInventory_.GetPtr<int>());

// 不支持插入同类型
    ret = dataInventory_.Inject(null);
    ASSERT_FALSE(ret);

// 清空
    dataInventory_.RemoveRestData({});

// 清空后查不到
    ASSERT_FALSE(dataInventory_.GetPtr<int>());
}

constexpr uint32_t TEST_DATA_ARR_SZ = 10;
struct TestDataStruct {
    const char* cc;
    int arr[TEST_DATA_ARR_SZ];
};

vector<DataInventory> GetDataInventory()
{
    static vector<DataInventory> dataInventory;
    DataInventory data;
    data.Inject(std::make_shared<int>(100));  // 测试数据100，可以换成其它值
    auto str = std::make_shared<string>("sample");
    data.Inject(str);
    dataInventory.push_back(move(data));

    data.Inject(std::make_shared<int>(200));  // 测试数据200，可以换成其它值
    auto vec = std::make_shared<vector<int>>();
    vec->push_back(200);  // 测试数据200，可以换成其它值
    data.Inject(vec);
    dataInventory.push_back(move(data));

    DataInventory dataNew;
    auto tds = make_shared<TestDataStruct>();
    for (uint32_t i = 0; i < TEST_DATA_ARR_SZ; ++i) {
        tds->arr[i] = i + 100;  // 测试数据100，可以换成其它值
    }
    tds->cc = "hello";
    dataNew.Inject(tds);
    dataNew.Inject(std::make_shared<int>(300));  // 测试数据300，可以换成其它值
    dataInventory.push_back(move(dataNew));

    return move(dataInventory);
}

TEST_F(DataInventoryUTest, ShouldGetDataInventoryFromFunctionReturnByMove)
{
    auto data = GetDataInventory();
    ASSERT_EQ(data.size(), 3ul);  // 测试数据构造为3个

    auto intPtr = data[0].GetPtr<int>();
    auto str = data[0].GetPtr<string>();
    ASSERT_TRUE(intPtr);
    ASSERT_TRUE(str);
    EXPECT_EQ(*intPtr, 100);  // 测试数据100
    EXPECT_EQ(*str, "sample");

    intPtr = data[1].GetPtr<int>();
    auto vec = data[1].GetPtr<vector<int>>();
    ASSERT_TRUE(intPtr);
    ASSERT_TRUE(vec);
    EXPECT_EQ(*intPtr, 200);  // 测试数据200
    ASSERT_EQ(vec->size(), 1ul);
    EXPECT_EQ(vec->at(0), 200);  // 测试数据200

    auto tds = data[2].GetPtr<TestDataStruct>();  // 第2个节点
    intPtr = data[2].GetPtr<int>();  // 第2个节点
    ASSERT_TRUE(tds);
    EXPECT_STREQ(tds->cc, "hello");
    for (uint32_t i = 0; i < TEST_DATA_ARR_SZ; ++i) {
        EXPECT_EQ(tds->arr[i], i + 100);  // 测试数据100
    }
    ASSERT_TRUE(intPtr);
    EXPECT_EQ(*intPtr, 300);  // 测试数据300
}