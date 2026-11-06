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
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/context/include/device_context.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

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

struct DynamicSchedulingState
{
    std::mutex mutex;
    std::condition_variable condition;
    bool slowRootStarted{false};
    bool allowSlowRootFinish{false};
    bool fastChildRan{false};
};

ProcessCreator Returning(uint32_t code)
{
    return [code]() -> std::unique_ptr<Process>
    { return std::unique_ptr<Process>(new CallbackProcess([code](DataInventory&) { return code; })); };
}
}  // namespace

class ProcessControlUTest : public Test
{
   protected:
    void SetUp() override { context->deviceContextInfo.deviceInfo.chipID = CHIP_V3_2_0; }

    void Bind(RuntimeProcessCollection& processes)
    {
        for (auto& process : processes)
        {
            process.second.context = context;
            process.second.inventory = inventory;
        }
    }

    std::shared_ptr<DataInventory> inventory = std::make_shared<DataInventory>();
    std::shared_ptr<Infra::DeviceContext> context = std::make_shared<Infra::DeviceContext>();
};

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

    RuntimeProcessCollection processes = {
        {1, {producer, {}, {}, {CHIP_ID_ALL}, "producer", true}},
        {2,
         {firstConsumer, {typeid(int)}, {1}, {CHIP_ID_ALL}, "first_consumer", true}},
        {3,
         {lastConsumer,
          {typeid(int), typeid(std::string)},
          {2},
          {CHIP_ID_ALL},
          "last_consumer",
          true}},
    };
    Bind(processes);
    ProcessControl processControl(processes);

    EXPECT_TRUE(processControl.ExecuteProcess());
    EXPECT_TRUE(firstConsumerSawData);
    EXPECT_TRUE(lastConsumerSawData);
    EXPECT_EQ(inventory->Size(), 0UL);
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

    RuntimeProcessCollection processes = {
        {1, {slowRoot, {}, {}, {CHIP_ID_ALL}, "slow_root", true}},
        {2, {fastRoot, {}, {}, {CHIP_ID_ALL}, "fast_root", true}},
        {3,
         {fastChild, {}, {2}, {CHIP_ID_ALL}, "fast_child", true}},
    };
    ProcessControlOptions options;
    options.maxWorkerThreads = 2;
    Bind(processes);
    ProcessControl processControl(processes, options);
    bool result = false;
    std::thread executionThread([&]() { result = processControl.ExecuteProcess(); });

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

TEST_F(ProcessControlUTest, ShouldReportOptionalFailureWithoutStoppingScheduling)
{
    auto creator = []() -> std::unique_ptr<Process>
    { return std::unique_ptr<Process>(new CallbackProcess([](DataInventory&) { return 0x2aU; })); };
    RuntimeProcessCollection processes = {
        {1, {creator, {}, {}, {CHIP_ID_ALL}, "optional_parser", false}},
    };
    Bind(processes);
    ProcessControl processControl(processes);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool result = processControl.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();

    EXPECT_TRUE(result);
    EXPECT_TRUE(prints.empty());
    EXPECT_THAT(messages,
                Contains(AllOf(Field(&TestLogMessage::level, "[ERROR]"),
                               Field(&TestLogMessage::message,
                                     HasSubstr("Process failed: node=optional_parser, chip_id=" +
                                               std::to_string(CHIP_V3_2_0) + ", return_code=0x2a, mandatory=false")))));
}

TEST_F(ProcessControlUTest, ShouldPrioritizeMandatoryFailureOverDfxStopInConsole)
{
    auto creator = []() -> std::unique_ptr<Process>
    { return std::unique_ptr<Process>(new CallbackProcess([](DataInventory&) { return 0x2aU; })); };
    RuntimeProcessCollection processes = {
        {1, {creator, {}, {}, {CHIP_ID_ALL}, "failing_parser", true}},
    };
    context->deviceContextInfo.dfxInfo.stopAt = "failing_parser";
    Bind(processes);
    ProcessControl processControl(processes);
    StartTestPrintCapture();
    const bool result = processControl.ExecuteProcess();
    const auto prints = StopTestPrintCapture();

    EXPECT_FALSE(result);
    EXPECT_THAT(prints, ElementsAre(AllOf(
                            Field(&TestLogMessage::level, "[ERROR]"),
                            Field(&TestLogMessage::message, "Processing failed. Please check the logs for details."))));
}

TEST_F(ProcessControlUTest, ShouldBlockFailedDependenciesAndRunIndependentBranch)
{
    bool dependentRan = false;
    bool independentRan = false;
    auto fail = []() -> std::unique_ptr<Process>
    { return std::unique_ptr<Process>(new CallbackProcess([](DataInventory&) { return ANALYSIS_ERROR; })); };
    auto dependent = [&]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess(
            [&](DataInventory&)
            {
                dependentRan = true;
                return ANALYSIS_OK;
            }));
    };
    auto independent = [&]() -> std::unique_ptr<Process>
    {
        return std::unique_ptr<Process>(new CallbackProcess(
            [&](DataInventory&)
            {
                independentRan = true;
                return ANALYSIS_OK;
            }));
    };
    RuntimeProcessCollection processes = {
        {1, {fail, {}, {}, {CHIP_ID_ALL}, "failed", false}},
        {2,
         {dependent, {}, {1}, {CHIP_ID_ALL}, "blocked", false}},
        {3, {independent, {}, {}, {CHIP_ID_ALL}, "independent", true}},
    };
    Bind(processes);
    ProcessControl control(processes);

    EXPECT_TRUE(control.ExecuteProcess());
    EXPECT_FALSE(dependentRan);
    EXPECT_TRUE(independentRan);
    const auto results = control.GetRuntimeResults();
    ASSERT_EQ(results.size(), 3U);
    for (const auto& result : results)
    {
        const auto& name = result.second.statistics.processName;
        if (name == "failed")
        {
            EXPECT_EQ(result.second.state, RuntimeProcessState::FAILED);
        }
        else if (name == "blocked")
        {
            EXPECT_EQ(result.second.state, RuntimeProcessState::BLOCKED);
        }
        else
        {
            EXPECT_EQ(name, "independent");
            EXPECT_EQ(result.second.state, RuntimeProcessState::SUCCEEDED);
        }
    }
}

TEST_F(ProcessControlUTest, ShouldHonorDataReleaseOption)
{
    for (bool release : {false, true})
    {
        inventory = std::make_shared<DataInventory>();
        RuntimeProcessCollection processes = {
            {1, {[]() -> std::unique_ptr<Process> {
                return std::unique_ptr<Process>(new CallbackProcess([](DataInventory& data) {
                    return data.Inject(std::make_shared<int>(7)) ? ANALYSIS_OK : ANALYSIS_ERROR;
                }));
            }, {}, {}, {}, "producer", true}},
        };
        Bind(processes);
        ProcessControlOptions options;
        options.releaseUnusedData = release;
        ProcessControl executor(processes, options);
        ASSERT_TRUE(executor.ExecuteProcess());
        EXPECT_EQ(inventory->GetPtr<int>() == nullptr, release);
    }
}

TEST_F(ProcessControlUTest, ShouldReportMandatoryFailureAndBlockItsChild)
{
    RuntimeProcessCollection processes = {
        {1, {Returning(0x2aU), {}, {}, {}, "failing_parser", true}},
        {2, {Returning(ANALYSIS_OK), {}, {1}, {}, "persistence", true}},
    };
    Bind(processes);
    ProcessControl executor(processes);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool result = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();
    EXPECT_FALSE(result);
    EXPECT_THAT(prints, ElementsAre(AllOf(
        Field(&TestLogMessage::level, "[ERROR]"),
        Field(&TestLogMessage::message, "Processing failed. Please check the logs for details."))));
    EXPECT_THAT(messages, Contains(AllOf(
        Field(&TestLogMessage::level, "[ERROR]"),
        Field(&TestLogMessage::message, HasSubstr("Process failed: node=failing_parser, chip_id=" +
            std::to_string(CHIP_V3_2_0) + ", return_code=0x2a, mandatory=true")))));
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::BLOCKED);
    const auto statistics = executor.GetExecuteStat();
    ASSERT_EQ(statistics.allLevelStat.size(), 1U);
    EXPECT_FALSE(statistics.allLevelStat[0].generalResult);
    ASSERT_EQ(statistics.allLevelStat[0].processStatistics.size(), 1U);
    EXPECT_EQ(statistics.allLevelStat[0].processStatistics[0].chipId, CHIP_V3_2_0);
}

TEST_F(ProcessControlUTest, ShouldStopAtAssignedProcessAndRecordDfxStatistics)
{
    context->deviceContextInfo.dfxInfo.stopAt = "parser";
    RuntimeProcessCollection processes = {
        {1, {Returning(ANALYSIS_OK), {}, {}, {}, "parser", true}},
        {2, {Returning(ANALYSIS_OK), {}, {1}, {}, "persistence", true}},
    };
    Bind(processes);
    ProcessControl executor(processes);
    StartTestLogCapture();
    StartTestPrintCapture();
    const bool result = executor.ExecuteProcess();
    const auto messages = StopTestLogCapture();
    const auto prints = StopTestPrintCapture();
    EXPECT_FALSE(result);
    EXPECT_TRUE(prints.empty());
    EXPECT_THAT(messages, Contains(AllOf(
        Field(&TestLogMessage::level, "[INFO]"),
        Field(&TestLogMessage::message, HasSubstr("DFX stop requested: node=parser, chip_id=" +
            std::to_string(CHIP_V3_2_0) + ", return_code=0x0, mandatory=true")))));
    EXPECT_THAT(messages, Not(Contains(Field(&TestLogMessage::message, HasSubstr("Process failed:")))));
    EXPECT_EQ(executor.GetRuntimeResults().at(2).state, RuntimeProcessState::BLOCKED);
    const auto statistics = executor.GetExecuteStat();
    ASSERT_EQ(statistics.allLevelStat.size(), 1U);
    std::string report;
    RecordProcessStat(statistics, "device_0", report);
    EXPECT_THAT(report, HasSubstr("[parser]:return: 0x0, mandatory:true, dfxStop:true"));
}

TEST_F(ProcessControlUTest, ShouldRejectMissingDuplicateAndCyclicDependencies)
{
    const std::vector<std::vector<RuntimeProcessKey>> invalid = {{42}, {1, 1}, {2}};
    for (const auto& dependencies : invalid)
    {
        RuntimeProcessCollection processes = {
            {1, {Returning(ANALYSIS_OK), {}, {}, {}, "producer", true}},
            {2, {Returning(ANALYSIS_OK), {}, dependencies, {}, "consumer", true}},
        };
        Bind(processes);
        ProcessControl executor(processes);
        EXPECT_FALSE(executor.ExecuteProcess());
        EXPECT_TRUE(executor.GetRuntimeResults().empty());
    }
}

TEST_F(ProcessControlUTest, ShouldKeepOwnedBindingsAndResultSnapshotsAfterCallerReleasesThem)
{
    class BindingProcess final : public Process
    {
        uint32_t ProcessEntry(DataInventory& data, const Context& ctx) override
        {
            return data.Inject(std::make_shared<uint32_t>(ctx.GetChipID())) ? ANALYSIS_OK : ANALYSIS_ERROR;
        }
    };
    RuntimeProcessCollection processes = {
        {1, {[]() -> std::unique_ptr<Process> { return std::unique_ptr<Process>(new BindingProcess); },
             {}, {}, {}, "binding", true}},
    };
    Bind(processes);
    std::weak_ptr<DataInventory> ownedInventory = inventory;
    std::weak_ptr<const Context> ownedContext = context;
    ProcessControlOptions options;
    options.releaseUnusedData = false;
    ProcessControl executor(processes, options);
    inventory.reset();
    context.reset();
    ASSERT_TRUE(executor.ExecuteProcess());
    ASSERT_FALSE(ownedInventory.expired());
    ASSERT_FALSE(ownedContext.expired());
    const auto value = ownedInventory.lock()->GetPtr<uint32_t>();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, CHIP_V3_2_0);
    EXPECT_EQ(executor.GetRuntimeResults().at(1).statistics.chipId, CHIP_V3_2_0);
}

TEST_F(ProcessControlUTest, ShouldHandleRepeatedEmptyGraphsWithFreshStatistics)
{
    RuntimeProcessCollection processes;
    ProcessControl executor(processes);
    for (size_t run = 0; run < 2; ++run)
    {
        EXPECT_TRUE(executor.ExecuteProcess());
        EXPECT_TRUE(executor.GetExecuteStat().allLevelStat.empty());
        EXPECT_TRUE(executor.GetRuntimeResults().empty());
    }
}
