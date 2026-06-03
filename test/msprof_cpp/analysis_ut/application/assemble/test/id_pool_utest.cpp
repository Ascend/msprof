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
#include <algorithm>
#include <vector>
#include <unordered_map>
#include "gtest/gtest.h"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

using namespace Analysis::Application::Credential;
using namespace Analysis::Utils;

class IdPoolUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
    }
};

TEST_F(IdPoolUTest, GetUint64IdShouldReturnNewIndexWhenInputIsStrAndKeyNotExist)
{
    uint64_t expect = 0;
    std::string opName = "conv2d";
    auto res = IdPool::GetInstance().GetUint64Id(opName);
    EXPECT_EQ(expect, res);
}

TEST_F(IdPoolUTest, GetUint64IdShouldReturnOriIndexWhenInputIsStrAndKeyExist)
{
    uint64_t expect = 0;
    std::string opName = "pool";
    IdPool::GetInstance().GetUint64Id(opName);
    auto res = IdPool::GetInstance().GetUint64Id(opName);
    EXPECT_EQ(expect, res);
}

TEST_F(IdPoolUTest, GetUint32IdShouldReturnNewIndexWhenInputIsStrAndKeyNotExist)
{
    uint64_t expect = 0;
    std::string str = "/home/test/PROF_000001_20231010105807679_GRRHDCGNICIIQCPA";
    auto res = IdPool::GetInstance().GetUint32Id(str);
    EXPECT_EQ(expect, res);
}

TEST_F(IdPoolUTest, GetUint32IdShouldReturnOriIndexWhenInputIsStrAndKeyExist)
{
    uint64_t expect = 0;
    std::string str = "/home/test/PROF_000001_20231010105807679_GRRHDCGNICIIQCPA";
    IdPool::GetInstance().GetUint64Id(str);
    auto res = IdPool::GetInstance().GetUint32Id(str);
    EXPECT_EQ(expect, res);
}

TEST_F(IdPoolUTest, GetIdShouldReturnNewIndexWhenInputIsTupleAndKeyNotExist)
{
    uint64_t expect = 0;
    uint32_t deviceId = 1;
    uint32_t streamId = 3;
    uint32_t taskId = 4;
    uint32_t contextId = 5;
    uint32_t batchId = 6;
    uint32_t devType = 0;
    CorrelationTuple key = std::make_tuple(deviceId, streamId, taskId, contextId, batchId, devType);
    auto res = IdPool::GetInstance().GetId(key);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetIdShouldReturnNewIndexWhenInputIsTupleAndKeyExist)
{
    uint64_t expect = 0;
    uint32_t deviceId = 2;
    uint32_t streamId = 4;
    uint32_t taskId = 4;
    uint32_t contextId = 5;
    uint32_t batchId = 6;
    uint32_t devType = 0;
    CorrelationTuple key = std::make_tuple(deviceId, streamId, taskId, contextId, batchId, devType);
    IdPool::GetInstance().GetId(key);
    auto res = IdPool::GetInstance().GetId(key);
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, GetIdShouldReturnDifferentIndexWhenOnlyDevTypeIsDifferent)
{
    uint64_t expectFirst = 0;
    uint64_t expectSecond = 1;
    auto npuTaskKey = std::make_tuple(1, 2, 3, 4, 5, 0);
    auto dpuTaskKey = std::make_tuple(1, 2, 3, 4, 5, 2);
    EXPECT_EQ(expectFirst, IdPool::GetInstance().GetId(npuTaskKey));
    EXPECT_EQ(expectSecond, IdPool::GetInstance().GetId(dpuTaskKey));
    EXPECT_EQ(expectFirst, IdPool::GetInstance().GetId(npuTaskKey));
    EXPECT_EQ(expectSecond, IdPool::GetInstance().GetId(dpuTaskKey));
}

TEST_F(IdPoolUTest, GetAllUint64IdsShouldReturnMaps)
{
    std::unordered_map<std::string, uint64_t> expect;
    uint64_t denseId = 2;
    uint64_t poolId = 1;
    uint64_t conv2dId = 0;
    expect["dense"] = denseId;
    expect["pool"] = poolId;
    expect["conv2d"] = conv2dId;
    std::string opName = "conv2d";
    IdPool::GetInstance().GetUint64Id(opName);
    opName = "pool";
    IdPool::GetInstance().GetUint64Id(opName);
    opName = "dense";
    IdPool::GetInstance().GetUint64Id(opName);
    auto res = IdPool::GetInstance().GetAllUint64Ids();
    EXPECT_EQ(expect, res);
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, Uint64IdShouldBeUniqueWhenInputIsStrInTheMultiThreadScenario)
{
    const int threadsNum = 200;
    const int tasksNum = 100;
    std::vector<uint64_t> taskResult;
    std::vector<uint64_t> taskRepeatedResult;
    ThreadPool pool(threadsNum);
    std::mutex taskMutex;
    std::mutex taskRepeatedMutex;
    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (uint64_t i = 0; i < tasksNum; i++) {
        auto taskGet = [i, &taskResult, &taskMutex]() {
            auto res = IdPool::GetInstance().GetUint64Id(std::to_string(i));
            std::lock_guard<std::mutex> lock(taskMutex);
            taskResult.emplace_back(res);
        };
        auto taskRepeatedGet = [i, &taskRepeatedResult, &taskRepeatedMutex]() {
            auto res = IdPool::GetInstance().GetUint64Id(std::to_string(i));
            std::lock_guard<std::mutex> lock(taskRepeatedMutex);
            taskRepeatedResult.emplace_back(res);
        };
        pool.AddTask(taskGet);
        pool.AddTask(taskRepeatedGet);
    }

    pool.WaitAllTasks();
    pool.Stop();
    std::sort(taskResult.begin(), taskResult.end());
    std::sort(taskRepeatedResult.begin(), taskRepeatedResult.end());
    for (uint64_t i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, taskResult[i]);
        EXPECT_EQ(i, taskRepeatedResult[i]);
    }
    IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, Uint32IdShouldBeUniqueWhenInputIsStrInTheMultiThreadScenario)
{
    const int threadsNum = 200;
    const int tasksNum = 100;
    std::vector<uint64_t> taskResult;
    std::vector<uint64_t> taskRepeatedResult;
    ThreadPool pool(threadsNum);
    std::mutex taskMutex;
    std::mutex taskRepeatedMutex;
    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (uint64_t i = 0; i < tasksNum; i++) {
    auto taskGet = [i, &taskResult, &taskMutex]() {
        auto res = IdPool::GetInstance().GetUint32Id(std::to_string(i));
        std::lock_guard<std::mutex> lock(taskMutex);
        taskResult.emplace_back(res);
    };
    auto taskRepeatedGet = [i, &taskRepeatedResult, &taskRepeatedMutex]() {
        auto res = IdPool::GetInstance().GetUint32Id(std::to_string(i));
        std::lock_guard<std::mutex> lock(taskRepeatedMutex);
        taskRepeatedResult.emplace_back(res);
    };
    pool.AddTask(taskGet);
    pool.AddTask(taskRepeatedGet);
    }

    pool.WaitAllTasks();
    pool.Stop();
    std::sort(taskResult.begin(), taskResult.end());
    std::sort(taskRepeatedResult.begin(), taskRepeatedResult.end());
    for (uint64_t i = 0; i < tasksNum; ++i) {
    EXPECT_EQ(i, taskResult[i]);
    EXPECT_EQ(i, taskRepeatedResult[i]);
}
IdPool::GetInstance().Clear();
}

TEST_F(IdPoolUTest, IdsShouldBeUniqueWhenInputIsTupleInTheMultiThreadScenario)
{
    const int threadsNum = 200;
    const int tasksNum = 100;
    std::vector<uint64_t> taskResult;
    std::vector<uint64_t> taskRepeatedResult;
    ThreadPool pool(threadsNum);
    std::mutex taskMutex;
    std::mutex taskRepeatedMutex;

    auto err = pool.Start();
    EXPECT_TRUE(err);

    for (uint64_t i = 0; i < tasksNum; i++) {
        auto taskGet = [i, &taskResult, &taskMutex]() {
            auto res = IdPool::GetInstance().GetId(std::make_tuple(i, i, i, i, i, 0));
            std::lock_guard<std::mutex> lock(taskMutex);
            taskResult.emplace_back(res);
        };
        auto taskRepeatedGet = [i, &taskRepeatedResult, &taskRepeatedMutex]() {
            auto res = IdPool::GetInstance().GetId(std::make_tuple(i, i, i, i, i, 0));
            std::lock_guard<std::mutex> lock(taskRepeatedMutex);
            taskRepeatedResult.emplace_back(res);
        };
        pool.AddTask(taskGet);
        pool.AddTask(taskRepeatedGet);
    }

    pool.WaitAllTasks();
    pool.Stop();
    std::sort(taskResult.begin(), taskResult.end());
    std::sort(taskRepeatedResult.begin(), taskRepeatedResult.end());
    for (uint64_t i = 0; i < tasksNum; ++i) {
        EXPECT_EQ(i, taskResult[i]);
        EXPECT_EQ(i, taskRepeatedResult[i]);
    }
    IdPool::GetInstance().Clear();
}
