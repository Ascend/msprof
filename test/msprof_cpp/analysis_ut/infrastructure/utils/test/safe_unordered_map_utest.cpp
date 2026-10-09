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

#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <string>
#include <future>
#include <algorithm>
#include "gtest/gtest.h"

#include "analysis/csrc/infrastructure/utils/safe_unordered_map.h"

using namespace Analysis::Utils;

class SafeUnorderedMapUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

/* 验证 SafeUnorderedMap Iterate遍历正确性 */
TEST_F(SafeUnorderedMapUTest, TestIterateShouldTraverseAllItemsCorrectlyWhenMultiThreadCall)
{
    SafeUnorderedMap<std::string, int> testMap;
    const int insertNum = 10;
    for (int i = 0; i < insertNum; ++i) {
        testMap.Insert(std::to_string(i), i);
    }

    // 10个线程遍历，验证多线程下遍历map的正确性
    const int threadsNum = 10;
    std::thread threads[threadsNum];
    for (int i = 0; i < threadsNum; ++i) {
        threads[i] = std::thread([&testMap]() {
            for (auto it = testMap.Begin(); it != testMap.End(); it++) {
                EXPECT_EQ((*it).first, std::to_string((*it).second));
            }
        });
    }

    for (auto &t: threads) {
        t.join();
    }
}

/* 单线程验证 SafeUnorderedMap 增删查改 */
TEST_F(SafeUnorderedMapUTest, SingleThreadTestCUID)
{
    SafeUnorderedMap<std::string, int> testMap;
    const int insertNum = 10;
    for (int i = 0; i < insertNum; ++i) {
        testMap.Insert(std::to_string(i), i);
    }
    EXPECT_FALSE(testMap.Empty());
    EXPECT_EQ(testMap.Size(), insertNum);

    const std::string key1 = "1";
    const std::string key2 = "2";
    const int value1 = 1;
    const int value2 = 2;
    SafeUnorderedMap<std::string, int> testMap2 = testMap;
    EXPECT_EQ(testMap2[key1], value1);

    testMap2[key1] = value2;
    EXPECT_EQ(testMap2[key1], value2);

    testMap2.Erase(key1);
    int value = -1;
    EXPECT_FALSE(testMap2.Find(key1, value));
    EXPECT_TRUE(testMap2.Find(key2, value));
    EXPECT_EQ(value, value2);

    testMap2.Clear();
    EXPECT_TRUE(testMap2.Empty());
}

/* 多线程验证 SafeUnorderedMap 增删查改 */
TEST_F(SafeUnorderedMapUTest, MultiThreadInsertMultiValueToMultiKey)
{
    const int threadsNum = 100;
    SafeUnorderedMap<std::string, int> testMap;
    std::vector<std::future<int>> findNums;
    std::thread threads[threadsNum];

    /* 往testMap中插入("0", 0) - ("99", 99)的键值对 */
    auto lamfuncInsert = [](SafeUnorderedMap<std::string, int> &data,
                            const std::string &key,
                            const int &value,
                            const std::chrono::system_clock::time_point &absTime) {
        std::this_thread::sleep_until(absTime);
        data.Insert(key, value);
    };

    /* 向testMap中读取("0", 0) - ("99", 99)的键值对 */
    auto lamfuncCheck = [](SafeUnorderedMap<std::string, int> &data,
                           const std::string &key, std::chrono::system_clock::time_point absTime) {
        std::this_thread::sleep_until(absTime);
        thread_local int i = -1;
        while (!data.Find(key, i)) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        return i;
    };

    using std::chrono::system_clock;
    std::time_t timeT = system_clock::to_time_t(system_clock::now());
    const int timeOffset = 2;
    timeT += timeOffset;
    for (int i = 0; i < threadsNum; ++i) {
        threads[i] =
            std::thread(lamfuncInsert, std::ref(testMap), std::to_string(i), i, system_clock::from_time_t(timeT));
        findNums.push_back(std::async(std::launch::async,
                                      lamfuncCheck,
                                      std::ref(testMap),
                                      std::to_string(i),
                                      system_clock::from_time_t(timeT)));
    }

    const int sleepT = 2;
    std::this_thread::sleep_for(std::chrono::seconds(sleepT));
    for (auto &t: threads) {
        t.join();
    }

    std::vector<int> result;
    for (auto &t: findNums) {
        result.push_back(t.get());
    }
    /* 读取到的值应该是0~99 */
    std::sort(result.begin(), result.end());
    for (int i = 0; i < threadsNum; ++i) {
        EXPECT_EQ(i, result[i]);
    }
}