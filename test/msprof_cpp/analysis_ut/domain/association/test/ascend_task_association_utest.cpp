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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <map>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"

using namespace testing;
using namespace Analysis::Infra;

namespace Analysis {
namespace Domain {
const int TEST_ID = 1;
class AscendTaskAssociationUTest : public Test {
protected:
    void SetUp()
    {
        auto data = std::make_shared<std::map<TaskId, std::vector<HostTask>>>();
        dataInventory_.Inject(data);
        auto deviceTask = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
        dataInventory_.Inject(deviceTask);
        auto streamInfo = std::make_shared<HostStreamInfo>();
        dataInventory_.Inject(streamInfo);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
    }

protected:
    DataInventory dataInventory_;
};

static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTask()
{
    std::map<TaskId, std::vector<DeviceTask>> deviceTask;
    auto &res1 = deviceTask[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().taskEnd = 11000;  // 测试结束时间11000
    res1.back().taskStart = 10000;  // 测试开始时间10000
    auto &res2 = deviceTask[{1, 2, 1, 1}];
    res2.emplace_back();
    res2.back().taskStart = 20000;  // 测试结束时间20000
    res2.back().taskEnd = 22000;   // 测试开始时间22000
    return deviceTask;
}

static std::map<TaskId, std::vector<HostTask>> GeneratorHostTask()
{
    std::map<TaskId, std::vector<HostTask>> hostTask;
    auto &res1 = hostTask[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().modelId = TEST_ID;
    res1.back().connection_id = 1234;  // 测试connection_id 1234
    res1.back().taskTypeStr = "KERNEL_AICPU";
    res1.back().streamId = TEST_ID;
    res1.back().contextId = TEST_ID;
    res1.back().taskId = TEST_ID;
    res1.back().batchId = TEST_ID;
    res1.back().requestId = TEST_ID;
    auto &res2 = hostTask[{1, 3, 1, 1}];
    res2.emplace_back();
    res2.back().modelId = TEST_ID;
    res2.back().connection_id = 2345;  // 测试connection_id 2345
    res2.back().taskTypeStr = "AI_CORE";
    res2.back().streamId = TEST_ID;
    res2.back().contextId = TEST_ID;
    res2.back().taskId = TEST_ID;
    res2.back().batchId = 3;   // 测试batchId 3
    res2.back().requestId = TEST_ID;
    return hostTask;
}

TEST_F(AscendTaskAssociationUTest, ShouldGenerateAscendTaskWhenProcessRun)
{
    AscendTaskAssociation association;
    Context context;
    auto hostDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto deviceDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto hostData = GeneratorHostTask();
    auto deviceData = GenerateDeviceTask();
    deviceDataS->swap(deviceData);
    hostDataS->swap(hostData);
    ASSERT_EQ(ANALYSIS_OK, association.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    ASSERT_EQ(3ul, result->size());
    for (auto& it : *result) {
        if (it.batchId == 1) {  // 验证batchId 1
            ASSERT_EQ(1234, it.connectionId);  // 预期connectionId 1234
        } else if (it.batchId == 2) {  // 验证batchId 2
            ASSERT_EQ(-1, it.connectionId);  // 预期connectionId -1
        } else if (it.batchId == 3) {   // 验证batchId 3
            ASSERT_EQ(2345, it.connectionId);  // 预期connectionId 2345
        }
    }
}

TEST_F(AscendTaskAssociationUTest, ShouldReturnErrorWhenDeviceTaskIsEmpty)
{
    AscendTaskAssociation association;
    DeviceContext context;
    uint64_t diff = 100;
    auto hostDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto hostData = GeneratorHostTask();
    hostDataS->swap(hostData);
    context.deviceContextInfo.hostStartLog.cntVctDiff = diff;
    ASSERT_EQ(ANALYSIS_ERROR, association.Run(dataInventory_, context));
}

TEST_F(AscendTaskAssociationUTest, ShouldReturnOKWhenDeviceTaskAndHostTaskIsEmpty)
{
    AscendTaskAssociation association;
    DeviceContext context;
    ASSERT_EQ(ANALYSIS_OK, association.Run(dataInventory_, context));
}

TEST_F(AscendTaskAssociationUTest, TestProcessEntryReturnUnsetStreamIdWhenDeviceTaskIsEmpty)
{
    AscendTaskAssociation association;
    DeviceContext context;
    auto hostDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto deviceDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto hostData = GeneratorHostTask();
    auto deviceData = GenerateDeviceTask();
    deviceDataS->swap(deviceData);
    hostDataS->swap(hostData);
    context.deviceContextInfo.deviceInfo.chipID = 15;
    association.ProcessEntry(dataInventory_, context);
    auto result = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    ASSERT_EQ(deviceDataS->begin()->first.streamId, TEST_ID);
}

TEST_F(AscendTaskAssociationUTest, TestProcessEntrySetStreamIdSuccessWhenStreamIdInfoEmpty)
{
    AscendTaskAssociation association;
    DeviceContext context;
    auto hostDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto deviceDataS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    TaskId taskId = {(uint16_t)1, (uint16_t)1, (uint16_t)1, 1};
    deviceDataS->emplace(taskId, std::vector<DeviceTask>());
    auto streamIdMap = dataInventory_.GetPtr<HostStreamInfo>();
    streamIdMap->streamIdMap.emplace(TEST_ID, 100);
    context.deviceContextInfo.deviceInfo.chipID = 15;
    association.ProcessEntry(dataInventory_, context);
    auto result = dataInventory_.GetPtr<std::vector<TopDownTask>>();
    ASSERT_EQ(deviceDataS->begin()->first.streamId, 100);
}
}
}