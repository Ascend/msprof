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
#include "analysis/csrc/domain/services/init/include/device_task_process.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace testing;
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

const std::string DEVICE_PATH = "./device_0";
class DeviceTaskProcessUTest : public testing::Test {
protected:
    DataInventory dataInventory_;
protected:
    void SetUp() override
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        File::RemoveDir(DEVICE_PATH, 0);
    }
};

TEST_F(DeviceTaskProcessUTest, ShouldInitDeviceTaskSuccess)
{
    DeviceTaskProcess deviceTaskProcess;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(Analysis::ANALYSIS_OK, deviceTaskProcess.Run(dataInventory_, deviceContext));
}

TEST_F(DeviceTaskProcessUTest, ShouldNotCreateSqliteDir)
{
    DeviceTaskProcess deviceTaskProcess;
    DeviceContext deviceContext;
    deviceContext.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    ASSERT_EQ(Analysis::ANALYSIS_OK, deviceTaskProcess.Run(dataInventory_, deviceContext));
    EXPECT_FALSE(File::Exist(File::PathJoin({DEVICE_PATH, "sqlite"})));
}
