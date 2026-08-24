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
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/process/process_topo.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "fake_process/pah_d.h"
#include "fake_process/pstart_a.h"
#include "fake_process/pstart_h.h"
#include "fake_process/base_gc.h"
#include "fake_process/process_spy.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"

using namespace testing;
using namespace Analysis;
using namespace Infra;

namespace
{
class CallbackProcess final : public Process
{
   public:
    explicit CallbackProcess(std::function<uint32_t(DataInventory&)> callback) : callback_(std::move(callback)) {}

   private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Infra::Context&) override
    {
        return callback_(dataInventory);
    }

    std::function<uint32_t(DataInventory&)> callback_;
};

struct ProducerProcessTag
{
};
struct FirstConsumerProcessTag
{
};
struct LastConsumerProcessTag
{
};
struct SlowRootProcessTag
{
};
struct FastRootProcessTag
{
};
struct FastChildProcessTag
{
};

struct DynamicSchedulingState
{
    std::mutex mutex;
    std::condition_variable condition;
    bool slowRootStarted{false};
    bool allowSlowRootFinish{false};
    bool fastChildRan{false};
};
}  // namespace

class ProcessControlUTest : public Test {
protected:
    void SetUp() override
    {
    }
    void TearDown() override
    {
        dataInventory.RemoveRestData({});
    }

    DataInventory dataInventory;
};

/* 此测试套的TOPO图如下
                                         ┌───────────────┐
                                         │     PdfE      │
                                         │               │
                                         │  CHIP_V1_1_0  │
                                ┌────────┤  CHIP_V2_1_0  │
                                │        └───────┬───────┘
                                ▼                │
                        ┌───────────────┐        │
                        │     PgF       │        │
                        │               │        │
            ┌───────────┤  CHIP_V1_1_0  │        │
            │           │  CHIP_V2_1_0  │        │
            │           └───────┬───────┘        │
            │                   │                │
            ▼                   ▼                │
   ┌───────────────┐    ┌───────────────┐        │
   │     PabC      │    │     PabG      │        │
   │               │    │               │        │
   │  CHIP_V1_1_0  │    │  CHIP_V1_1_0  │        │
   │               │    │  CHIP_V2_1_0  │        │
   └──┬────────┬───┘    └┬──────┬───────┘        │
      │        │         │      │                │
      │        ▼         ▼      │                ▼
      │    ┌───────────────┐    │         ┌───────────────┐
      │    │     PdB       │    │         │     PahD      │
      │    │               ├────┼────────►│               │
      │    │  CHIP_V1_1_0  │    │         │  CHIP_V1_1_0  │
      │    │  CHIP_V2_1_0  │    │ ┌───────┤  CHIP_V2_1_0  │
      │    └───────────────┘    │ │       └──────┬────────┘
      │                         │ │              │
      │                         │ │              │
      │                         ▼ ▼              ▼
      │             ┌───────────────┐      ┌───────────────┐
      │             │    PStartA    │      │    PstartH    │
      └────────────►│               │      │               │
                    │  CHIP_V1_1_0  │      │  CHIP_V1_1_0  │
                    │  CHIP_V2_1_0  │      │               │
                    └───────────────┘      └───────────────┘
*/

TEST_F(ProcessControlUTest, ShouldGetAllRegisterInfoRight)
{
    std::vector<uint32_t> expChipsPA{CHIP_V1_1_0, CHIP_V2_1_0};
    std::vector<std::type_index> expProcPreProc{typeid(Ps::PstartA), typeid(Ps::PstartH)};
    std::vector<std::type_index> exparamTypes{typeid(Ps::StartA), typeid(Ps::StartH)};
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ASSERT_EQ(regInfo.size(), 8ul);   // 应该有8个相关Process
    EXPECT_EQ(regInfo[typeid(Ps::PstartA)].chipIds, expChipsPA);
    EXPECT_EQ(regInfo[typeid(Ps::PahD)].processDependence, expProcPreProc);
    EXPECT_EQ(regInfo[typeid(Ps::PahD)].paramTypes, exparamTypes);
}

TEST_F(ProcessControlUTest, ShouldFilterRelatedProcessWhenBuildProcessTopo)
{
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessTopo processTopoBuilder(regInfo);
    auto chipRelatedProcess = processTopoBuilder.BuildProcessControlTopoByChip(CHIP_V1_1_0);
    ASSERT_EQ(chipRelatedProcess.size(), 8ul);  // 应该有8个相关Process

    ProcessTopo processTopoBuilderChip2(regInfo);
    chipRelatedProcess = processTopoBuilderChip2.BuildProcessControlTopoByChip(CHIP_V2_1_0);
    ASSERT_EQ(chipRelatedProcess.size(), 6ul);  // 应该有6个相关Process
    std::vector<std::type_index> expProcPreProc{typeid(Ps::PstartA)};
    EXPECT_EQ(chipRelatedProcess[typeid(Ps::PahD)].processDependence, expProcPreProc);
}

static void VerifyLastTwoFakeLevel(const ExecuteProcessStat& stat)
{
    const auto& allStat = stat.allLevelStat;
    std::vector<std::string> deps;
    EXPECT_TRUE(allStat[4].generalResult);  // 第4个Level应该成功
    ASSERT_EQ(allStat[4].processStatistics.size(), 1ul);  // 第4个Level应该有1个Process
    EXPECT_THAT(allStat[4].processStatistics[0].processName, testing::StrEq("Ps::PgF"));
    ASSERT_EQ(allStat[4].processStatistics[0].dependProcessNames.size(), 2ul);  // 第4个Level应该有两个依赖的Process
    deps.push_back(allStat[4].processStatistics[0].dependProcessNames[0]);  // 判断第4个Level的依赖
    deps.push_back(allStat[4].processStatistics[0].dependProcessNames[1]);  // 判断第4个Level的依赖
    EXPECT_THAT(deps, testing::Contains("Ps::PabC"));
    EXPECT_THAT(deps, testing::Contains("Ps::PabG"));

    EXPECT_TRUE(allStat[5].generalResult);  // 第5个Level应该成功
    ASSERT_EQ(allStat[5].processStatistics.size(), 1ul);  // 第5个Level应该有1个Process
    EXPECT_THAT(allStat[5].processStatistics[0].processName, testing::StrEq("Ps::PdfE"));
    ASSERT_EQ(allStat[5].processStatistics[0].dependProcessNames.size(), 2ul);  // 第5个Level应该有两个依赖的Process
    deps.clear();
    deps.push_back(allStat[5].processStatistics[0].dependProcessNames[0]);  // 校验第5个Level依赖的Process
    deps.push_back(allStat[5].processStatistics[0].dependProcessNames[1]);  // 校验第5个Level依赖的Process
    EXPECT_THAT(deps, testing::Contains("Ps::PgF"));
    EXPECT_THAT(deps, testing::Contains("Ps::PahD"));
}

static void VerifyWholeChipV1FakeProcess(const ExecuteProcessStat stat)
{
    const auto& allStat = stat.allLevelStat;
    EXPECT_EQ(stat.chipId, CHIP_V1_1_0);
    ASSERT_EQ(allStat.size(), 6ul);  // 一共有6个统计结果
    EXPECT_TRUE(allStat[0].generalResult);
    ASSERT_EQ(allStat[0].processStatistics.size(), 2ul);  // 第0个Leve有两个Process
    std::vector<std::string> names;
    names.push_back(allStat[0].processStatistics[0].processName);
    names.push_back(allStat[0].processStatistics[1].processName);
    EXPECT_THAT(names, testing::Contains("Ps::PstartA"));
    EXPECT_THAT(names, testing::Contains("Ps::PstartH"));
    // 头一轮执行的Process没有依赖
    EXPECT_TRUE(allStat[0].processStatistics[0].dependProcessNames.empty());
    EXPECT_TRUE(allStat[0].processStatistics[1].dependProcessNames.empty());

    EXPECT_TRUE(allStat[1].generalResult);
    ASSERT_EQ(allStat[1].processStatistics.size(), 1ul);
    EXPECT_THAT(allStat[1].processStatistics[0].processName, testing::StrEq("Ps::PahD"));
    ASSERT_EQ(allStat[1].processStatistics[0].dependProcessNames.size(), 2ul); // 第1个Level有2个依赖
    std::vector<std::string> deps;
    deps.push_back(allStat[1].processStatistics[0].dependProcessNames[0]);
    deps.push_back(allStat[1].processStatistics[0].dependProcessNames[1]);
    EXPECT_THAT(deps, testing::Contains("Ps::PstartA"));
    EXPECT_THAT(deps, testing::Contains("Ps::PstartH"));

    EXPECT_TRUE(allStat[2].generalResult);  // 第2个Level应该成功
    ASSERT_EQ(allStat[2].processStatistics.size(), 1ul);  // 第2个Level应该有1个Process
    EXPECT_THAT(allStat[2].processStatistics[0].processName, testing::StrEq("Ps::PdB")); // 第2个Level的Process Name
    ASSERT_EQ(allStat[2].processStatistics[0].dependProcessNames.size(), 1ul); // 第2个Level的1个依赖
    EXPECT_EQ(allStat[2].processStatistics[0].dependProcessNames[0], "Ps::PahD"); // 判断第2个Level的1 个依赖

    EXPECT_TRUE(allStat[3].generalResult);  // 第3个Level应该成功
    ASSERT_EQ(allStat[3].processStatistics.size(), 2ul); // 第3个Level应该有2个Process
    ASSERT_EQ(allStat[3].processStatistics[0].dependProcessNames.size(), 2ul); // 第3个Level应该有2个依赖
    ASSERT_EQ(allStat[3].processStatistics[1].dependProcessNames.size(), 2ul); // 第3个Level应该有2个依赖
    names.clear();
    names.push_back(allStat[3].processStatistics[0].processName); // 判断第3个Level的Process
    names.push_back(allStat[3].processStatistics[1].processName); // 判断第3个Level的Process
    EXPECT_THAT(names, testing::Contains("Ps::PabC"));
    EXPECT_THAT(names, testing::Contains("Ps::PabG"));
    deps.clear();
    deps.push_back(allStat[3].processStatistics[0].dependProcessNames[0]); // 判断第3个Level的依赖
    deps.push_back(allStat[3].processStatistics[0].dependProcessNames[1]); // 判断第3个Level的依赖
    deps.push_back(allStat[3].processStatistics[1].dependProcessNames[0]); // 判断第3个Level的依赖
    deps.push_back(allStat[3].processStatistics[1].dependProcessNames[1]); // 判断第3个Level的依赖
    EXPECT_THAT(deps, testing::Contains("Ps::PstartA"));
    EXPECT_THAT(deps, testing::Contains("Ps::PdB"));
    VerifyLastTwoFakeLevel(stat);
}

TEST_F(ProcessControlUTest, ShouldRunAllChipV1ProcessWhenExecute)
{
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessControl processControl(regInfo);

    ASSERT_TRUE(processControl.ExecuteProcess(dataInventory, deviceInstance));
    auto stat = processControl.GetExecuteStat();
    VerifyWholeChipV1FakeProcess(stat);
    EXPECT_EQ(Ps::GetBaseGcCount(), 2ul);

    Ps::ResetBaseGcCount();
}

TEST_F(ProcessControlUTest, ShouldKeepDataWhenReleaseUnusedDataDisabled)
{
    Ps::ResetBaseGcCount();
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessControlOptions options;
    options.releaseUnusedData = false;
    ProcessControl processControl(regInfo, options);

    ASSERT_TRUE(processControl.ExecuteProcess(dataInventory, deviceInstance));
    EXPECT_NE(dataInventory.GetPtr<Ps::StartA>(), nullptr);
    EXPECT_GT(dataInventory.Size(), 0UL);
    Ps::ResetBaseGcCount();
}

TEST_F(ProcessControlUTest, ShouldReleaseDataByDefault)
{
    Ps::ResetBaseGcCount();
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessControl processControl(regInfo);

    ASSERT_TRUE(processControl.ExecuteProcess(dataInventory, deviceInstance));
    EXPECT_EQ(dataInventory.Size(), 0UL);
    Ps::ResetBaseGcCount();
}

TEST_F(ProcessControlUTest, ShouldKeepDataUntilLastConsumerFinishes)
{
    bool firstConsumerSawData = false;
    bool lastConsumerSawData = false;
    auto producer = []() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([](DataInventory& dataInventory)
        {
            return dataInventory.Inject(std::make_shared<int>(7)) ? ANALYSIS_OK : ANALYSIS_ERROR;
        }));
    };
    auto firstConsumer = [&firstConsumerSawData]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([&firstConsumerSawData](DataInventory& dataInventory)
        {
            auto value = dataInventory.GetPtr<int>();
            firstConsumerSawData = value != nullptr && *value == 7;
            if (!firstConsumerSawData || !dataInventory.Inject(std::make_shared<std::string>("ready")))
            {
                return ANALYSIS_ERROR;
            }
            return ANALYSIS_OK;
        }));
    };
    auto lastConsumer = [&lastConsumerSawData]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([&lastConsumerSawData](DataInventory& dataInventory)
        {
            auto value = dataInventory.GetPtr<int>();
            auto marker = dataInventory.GetPtr<std::string>();
            lastConsumerSawData = value != nullptr && *value == 7 && marker != nullptr && *marker == "ready";
            return lastConsumerSawData ? ANALYSIS_OK : ANALYSIS_ERROR;
        }));
    };

    ProcessCollection processes = {
        {typeid(ProducerProcessTag), {producer, {}, {}, {CHIP_ID_ALL}, "producer", true}},
        {typeid(FirstConsumerProcessTag),
         {firstConsumer, {typeid(int)}, {typeid(ProducerProcessTag)}, {CHIP_ID_ALL}, "first_consumer", true}},
        {typeid(LastConsumerProcessTag),
         {lastConsumer,
          {typeid(int), typeid(std::string)},
          {typeid(FirstConsumerProcessTag)},
          {CHIP_ID_ALL},
          "last_consumer",
          true}},
    };
    Infra::DeviceContext context;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    ProcessControl processControl(processes);

    EXPECT_TRUE(processControl.ExecuteProcess(dataInventory, context));
    EXPECT_TRUE(firstConsumerSawData);
    EXPECT_TRUE(lastConsumerSawData);
    EXPECT_EQ(dataInventory.Size(), 0UL);
}

TEST_F(ProcessControlUTest, ShouldScheduleReadyDependentWithoutWaitingForUnrelatedProcess)
{
    auto state = std::make_shared<DynamicSchedulingState>();
    auto slowRoot = [state]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([state](DataInventory&)
        {
            std::unique_lock<std::mutex> lock(state->mutex);
            state->slowRootStarted = true;
            state->condition.notify_all();
            state->condition.wait(lock, [state]() { return state->allowSlowRootFinish; });
            return ANALYSIS_OK;
        }));
    };
    auto fastRoot = []() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([](DataInventory&) { return ANALYSIS_OK; }));
    };
    auto fastChild = [state]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess([state](DataInventory&)
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->fastChildRan = true;
            state->condition.notify_all();
            return ANALYSIS_OK;
        }));
    };

    ProcessCollection processes = {
        {typeid(SlowRootProcessTag), {slowRoot, {}, {}, {CHIP_ID_ALL}, "slow_root", true}},
        {typeid(FastRootProcessTag), {fastRoot, {}, {}, {CHIP_ID_ALL}, "fast_root", true}},
        {typeid(FastChildProcessTag),
         {fastChild, {}, {typeid(FastRootProcessTag)}, {CHIP_ID_ALL}, "fast_child", true}},
    };
    Infra::DeviceContext context;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    ProcessControl processControl(processes, options);
    bool result = false;
    std::thread executionThread([&]() { result = processControl.ExecuteProcess(dataInventory, context); });

    bool slowRootStarted = false;
    bool fastChildRan = false;
    {
        std::unique_lock<std::mutex> lock(state->mutex);
        slowRootStarted =
            state->condition.wait_for(lock, std::chrono::seconds(1), [state]() { return state->slowRootStarted; });
        fastChildRan = state->condition.wait_for(lock, std::chrono::seconds(1), [state]() { return state->fastChildRan; });
        state->allowSlowRootFinish = true;
    }
    state->condition.notify_all();
    executionThread.join();
    EXPECT_TRUE(slowRootStarted);
    EXPECT_TRUE(fastChildRan);
    EXPECT_TRUE(result);
}

TEST_F(ProcessControlUTest, ShouldFailedWhenProcessTopoCircled)
{
    struct A {};
    struct B {};
    struct C {};
    struct D {};
    ProcessCollection circled = {
        {typeid(A), {{}, {}, {}, {CHIP_ID_ALL}, "A", true}},
        {typeid(B), {{}, {}, {typeid(A), typeid(D)}, {CHIP_ID_ALL}, "B", true}},
        {typeid(C), {{}, {}, {typeid(B)}, {CHIP_ID_ALL}, "C", true}},
        {typeid(D), {{}, {}, {typeid(C)}, {CHIP_ID_ALL}, "D", true}},
    };
    ProcessCollection empty;
    ProcessControl processControl(empty);
    EXPECT_FALSE(processControl.VerifyProcess(circled));
}

TEST_F(ProcessControlUTest, ShouldFilterTheLastProcess)
{
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_1;
    auto creator = []() -> std::unique_ptr<Process> {return nullptr;};
    struct A {};
    struct B {};
    struct C {};
    struct D {};
    ProcessCollection chain = {
        {typeid(A), {creator, {}, {}, {CHIP_ID_ALL}, "A", true}},
        {typeid(B), {creator, {}, {typeid(A)}, {CHIP_ID_ALL}, "B", true}},
        {typeid(C), {creator, {}, {}, {CHIP_ID_ALL}, "C", true}},
        {typeid(D), {creator, {}, {typeid(A), typeid(B), typeid(C)}, {CHIP_V1_1_3}, "D", true}},
    };
    ProcessCollection empty;
    ProcessControl processControl(chain);
    auto ret = processControl.ExecuteProcess(dataInventory, deviceInstance);
    ASSERT_TRUE(ret);
    auto stat = processControl.GetExecuteStat();
    ASSERT_EQ(stat.allLevelStat.size(), 2ul);
    std::vector<std::string> names;
    names.push_back(stat.allLevelStat[0].processStatistics[0].processName);
    names.push_back(stat.allLevelStat[0].processStatistics[1].processName);
    EXPECT_THAT(names, testing::Contains("A"));
    EXPECT_THAT(names, testing::Contains("C"));

    ASSERT_EQ(stat.allLevelStat[1].processStatistics.size(), 1ul);
    EXPECT_EQ(stat.allLevelStat[1].processStatistics[0].processName, "B");
    EXPECT_FALSE(stat.allLevelStat[1].processStatistics[0].mandatory);
}

TEST_F(ProcessControlUTest, ShouldStopWhenOneMandatoryFailed)
{
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    ProcessSpy::SetResult("PabG", 1);

    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessControl processControl(regInfo);

    auto ret = processControl.ExecuteProcess(dataInventory, deviceInstance);
    ProcessSpy::ClearResult();

    ASSERT_FALSE(ret);
    auto stat = processControl.GetExecuteStat();
    EXPECT_EQ(stat.chipId, CHIP_V1_1_0);
    ASSERT_EQ(stat.allLevelStat.size(), 4ul);  // 应该有4个Level
    EXPECT_FALSE(stat.allLevelStat[3].generalResult);  // 第3个Level应该为失败
    ASSERT_EQ(stat.allLevelStat[3].processStatistics.size(), 2ul);  // 第3个Level包含2个Process
    EXPECT_NE(stat.allLevelStat[3].processStatistics[0].returnCode,   // 第3个Level的两个Process结果不应该相同
              stat.allLevelStat[3].processStatistics[1].returnCode);  // 第3个Level的两个Process结果不应该相同
}

TEST_F(ProcessControlUTest, ShouldStopAtAssignedProcessWhenUserAssigned)
{
    Infra::DeviceContext deviceInstance;
    deviceInstance.deviceContextInfo.deviceInfo.chipID = CHIP_V1_1_0;
    deviceInstance.deviceContextInfo.dfxInfo.stopAt = "Ps::PahD";
    auto regInfo = ProcessRegister::CopyProcessInfo();
    ProcessControl processControl(regInfo);

    EXPECT_FALSE(processControl.ExecuteProcess(dataInventory, deviceInstance));
    auto stat = processControl.GetExecuteStat();
    const auto& levelStat = stat.allLevelStat;
    ASSERT_EQ(levelStat.size(), 2ul);
    ASSERT_EQ(levelStat[1].processStatistics.size(), 1ul);
    EXPECT_EQ(levelStat[1].processStatistics[0].processName, "Ps::PahD");
}
