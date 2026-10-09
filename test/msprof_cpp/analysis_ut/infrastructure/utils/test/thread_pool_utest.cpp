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
#include <set>
#include <iostream>
#include <algorithm>
#include "gtest/gtest.h"
#include "analysis/csrc/infrastructure/utils/safe_unordered_map.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

using namespace Analysis::Utils;

class ThreadPoolUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

/* 线程池测试并行执行任务加速效果 */
TEST_F(ThreadPoolUTest, TestThreadPoolExecTimeShouldDecrease)
{
    const int tasksNum = 30;
    const int threadsNum = 15;

    ThreadPool pool(threadsNum);
    /* 每个task模拟执行1s任务 */
    auto task = []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    };
    auto err = pool.Start();
    EXPECT_TRUE(err);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < tasksNum; ++i) {
        pool.AddTask(task);
    }
    pool.WaitAllTasks();
    pool.Stop();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "duration = " << elapsed.count() << std::endl;
    /* 15个线程去执行30个任务 总时间约为2s */
    std::chrono::duration<double> expectT{2.2};
    auto check = elapsed <= expectT;
    EXPECT_TRUE(check);
}

/* 线程池测试获取未分配任务的数量 */
TEST_F(ThreadPoolUTest, TestGetUnassignedTasksNumShouldReturnZeroWhenNoAddTasks)
{
    const int tasksNum = 30;
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto err = pool.Start();
    EXPECT_TRUE(err);

    auto checkZero = pool.GetUnassignedTasksNum();
    EXPECT_EQ(checkZero, 0);
}

/* 线程池测试获取未分配任务的数量 */
TEST_F(ThreadPoolUTest, TestGetUnassignedTasksNumShouldReturnThirtyWhenAddThirtyTasks)
{
    const int tasksNum = 30;
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto task = []() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    };

    for (int i = 0; i < tasksNum; ++i) {
        pool.AddTask(task);
    }

    auto checkNum = pool.GetUnassignedTasksNum();
    const int expectNum = 30;
    EXPECT_EQ(expectNum, checkNum);
}

TEST_F(ThreadPoolUTest, TestGetThreadsNumShouldReturnZeroWhenNotStart)
{
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto checkZero = pool.GetThreadsNum();
    EXPECT_EQ(checkZero, 0);
}

TEST_F(ThreadPoolUTest, TestStartShouldReturnFalseWhenMultiCallStart)
{
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto checkTrue = pool.Start();
    EXPECT_TRUE(checkTrue);

    auto checkFalse = pool.Start();
    EXPECT_FALSE(checkFalse);
}

TEST_F(ThreadPoolUTest, TestStartShouldReturnFalseWhenMultiCallStop)
{
    const int threadsNum = 15;
    ThreadPool pool(threadsNum);

    auto checkTrue = pool.Start();
    EXPECT_TRUE(checkTrue);

    auto checkStopTrue = pool.Stop();
    EXPECT_TRUE(checkStopTrue);

    auto checkStopFalse = pool.Stop();
    EXPECT_FALSE(checkStopFalse);
}

TEST_F(ThreadPoolUTest, TestGetThreadsNumShouldReturnThirtyWhenAfterStart)
{
    const int tasksNum = 30;
    const int threadsNum = 30;
    ThreadPool pool(threadsNum);

    auto err = pool.Start();
    EXPECT_TRUE(err);

    auto checkThirty = pool.GetThreadsNum();
    const int expectNum = 30;
    EXPECT_EQ(checkThirty, expectNum);
}

/* 线程池测试并行操作SafeUnorderedMap */
TEST_F(ThreadPoolUTest, TestThreadPoolShouldStart100WriteThreadsAnd100ReadThreadsCorrectly)
{
    SafeUnorderedMap<std::string, int> testMap;
    const int threadsNum = 200;
    const int tasksNum = 100;
    ThreadPool pool(threadsNum);
    std::vector<int> res;
    std::mutex resLock;

    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (int i = 0; i < tasksNum; ++i) {
        /* 100个Write任务写不同的键值对 */
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        /* 100个Read任务读Write任务写入的键值对 */
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            /* 记录读到的结果 */
            std::lock_guard<std::mutex> lock(resLock);
            res.emplace_back(tmp);
        };

        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }

    pool.WaitAllTasks();
    pool.Stop();
    /* 首先Map中的数量需要等于插入的任务数 */
    EXPECT_EQ(tasksNum, testMap.Size());
    /* 检查是否有重复键值对 */
    std::sort(res.begin(), res.end());
    std::set<int> resSet(res.begin(), res.end());
    EXPECT_EQ(tasksNum, resSet.size());
    /* 检查结果是否正确 */
    for (int i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, res[i]);
    }
}

/* 线程池测试并行操作SafeUnorderedMap */
TEST_F(ThreadPoolUTest, TestThreadPoolShouldMultiTimesCallAddTaskCorrectly)
{
    SafeUnorderedMap<std::string, int> testMap;
    const int threadsNum = 200;
    const int tasksNum = 50;
    ThreadPool pool(threadsNum);
    std::vector<int> res;
    std::mutex resLock;
    pool.Start();
    /* 验证多次AddTask然后WaitAllTasks之后Stop的正确性 */
    for (int i = 0; i < tasksNum; ++i) {
        /* 100个Write任务写不同的键值对 */
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        /* 100个Read任务读Write任务写入的键值对 */
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            /* 记录读到的结果 */
            std::lock_guard<std::mutex> lock(resLock);
            res.emplace_back(tmp);
        };

        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }

    pool.WaitAllTasks();
    const int tasksNewNum = 100;
    for (int i = tasksNum; i < tasksNewNum; ++i) {
        /* 100个Write任务写不同的键值对 */
        auto taskWrite = [i, &testMap]() {
            testMap.Insert(std::to_string(i), i);
        };
        /* 100个Read任务读Write任务写入的键值对 */
        auto taskRead = [i, &testMap, &res, &resLock]() {
            thread_local int tmp = -1;
            while (!testMap.Find(std::to_string(i), tmp)) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            /* 记录读到的结果 */
            std::lock_guard<std::mutex> lock(resLock);
            res.emplace_back(tmp);
        };
        pool.AddTask(taskRead);
        pool.AddTask(taskWrite);
    }
    pool.WaitAllTasks();
    pool.Stop();
    /* 首先Map中的数量需要等于插入的任务数 */
    EXPECT_EQ(tasksNewNum, testMap.Size());
    /* 检查是否有重复键值对 */
    std::sort(res.begin(), res.end());
    std::set<int> resSet(res.begin(), res.end());
    EXPECT_EQ(tasksNewNum, resSet.size());
    /* 检查结果是否正确 */
    for (int i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, res[i]);
    }
}