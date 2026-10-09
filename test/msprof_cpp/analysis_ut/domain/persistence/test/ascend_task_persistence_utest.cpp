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
#include "analysis/csrc/domain/services/persistence/device/ascend_task_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;


namespace Analysis {
namespace Domain {
namespace {
const std::string DEVICE_PATH = "./device_0";
const int TEST_ID = 1;
}
class AscendTaskPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<TopDownTask>>();
        dataInventory_.Inject(data);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<TopDownTask> GenerateTopDownTask()
{
    std::vector<TopDownTask> res;
    res.emplace_back(true, TEST_ID, TEST_ID, TEST_ID, UINT32_MAX, TEST_ID, "AI_CORE", "AIC", UINT32_MAX, TEST_ID,
                     TEST_ID, TEST_ID);
    res.emplace_back(false, TEST_ID, TEST_ID, TEST_ID, UINT32_MAX, TEST_ID, "AI_CPU", "AI_CPU", UINT32_MAX, TEST_ID,
                     TEST_ID, TEST_ID);
    return res;
}

TEST_F(AscendTaskPersistenceUTest, ShouldSaveAscendTaskDataSuccess)
{
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    auto ascendTaskS = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    auto ascendTask = GenerateTopDownTask();
    ascendTaskS->swap(ascendTask);
    ASSERT_EQ(ANALYSIS_OK, taskPersistence.Run(dataInventory_, context));
}

TEST_F(AscendTaskPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_ERROR, taskPersistence.Run(dataInventory_, context));
}

TEST_F(AscendTaskPersistenceUTest, TestShouldReturnOKWhenDataIsEmpty)
{
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(ANALYSIS_OK, taskPersistence.Run(dataInventory_, context));
}

TEST_F(AscendTaskPersistenceUTest, ShouldReturnSuccessButNotSaveDataWhenDynamicWithOnlyTask)
{
    AscendTaskPersistence taskPersistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.sampleInfo.dynamic = true;
    auto ascendTaskS = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    auto ascendTask = GenerateTopDownTask();
    ascendTask.pop_back();
    ascendTaskS->swap(ascendTask);
    ASSERT_EQ(ANALYSIS_OK, taskPersistence.Run(dataInventory_, context));
    auto path = File::PathJoin({DEVICE_PATH, "sqlite"});
    auto files = File::GetOriginData(path, {"ascend_task.db"}, {});
    EXPECT_EQ(0, files.size());
}
}
}