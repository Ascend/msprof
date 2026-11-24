/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/calculator/l2_cache/l2_cache_task_calculator.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string ROOT_DIR = "./l2_cache_task_calculator_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});

HalL2CacheRaw MakeRaw(uint16_t taskType, uint16_t streamId, uint16_t taskId, const std::vector<uint64_t> &events)
{
    HalL2CacheRaw row{};
    row.taskType = taskType;
    row.streamId = streamId;
    row.taskId = taskId;
    for (size_t i = 0; i < events.size() && i < L2_CACHE_EVENT_SLOT_COUNT; ++i)
    {
        row.events[i] = events[i];
    }
    return row;
}

class L2CacheTaskCalculatorUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    DeviceContext CreateContext(uint32_t chipId, const std::string &events = "") const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        context.deviceContextInfo.deviceInfo.chipID = chipId;
        context.deviceContextInfo.sampleInfo.l2CacheTaskProfilingEventsStr = events;
        return context;
    }

    void InjectRaw(Infra::DataInventory &inventory, const HalL2CacheRaw &row)
    {
        auto rows = std::make_shared<std::vector<HalL2CacheRaw>>();
        rows->emplace_back(row);
        ASSERT_TRUE(inventory.Inject(rows));
    }

    void InjectRaws(Infra::DataInventory &inventory, const std::vector<HalL2CacheRaw> &rows)
    {
        auto data = std::make_shared<std::vector<HalL2CacheRaw>>(rows);
        ASSERT_TRUE(inventory.Inject(data));
    }
};
}  // namespace

TEST_F(L2CacheTaskCalculatorUTest, ZeroRequestYieldsZeroRates)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {0, 10, 20}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.0, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.0, summary->at(0).victimRate);
}

TEST_F(L2CacheTaskCalculatorUTest, RoundOneThirdMatchesPython)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.333333, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.0, summary->at(0).victimRate);
}

TEST_F(L2CacheTaskCalculatorUTest, Chip1Uses059Group)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(8, 1, 2, {100, 40, 10}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.4, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.1, summary->at(0).victimRate);
}

TEST_F(L2CacheTaskCalculatorUTest, Chip15NegativeCoefficients)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {100, 10, 10, 10, 5, 5}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V6_1_0, "0x00,0x81,0x82,0x83,0x74,0x75")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.6, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.1, summary->at(0).victimRate);
}

TEST_F(L2CacheTaskCalculatorUTest, Chip19UsesSameEventsAsChip15)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {100, 10, 10, 10, 5, 5}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V6_1_1, "0x00,0x81,0x82,0x83,0x74,0x75")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.6, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.1, summary->at(0).victimRate);
}

bool HasLogLevel(const std::vector<TestLogMessage> &logs, const std::string &level)
{
    for (const auto &item : logs)
    {
        if (item.level == level)
        {
            return true;
        }
    }
    return false;
}

bool HasLogMessage(const std::vector<TestLogMessage> &logs, const std::string &level, const std::string &part)
{
    for (const auto &item : logs)
    {
        if (item.level == level && item.message.find(part) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

TEST_F(L2CacheTaskCalculatorUTest, EmptyParseLogsExactMessageAndSkipsSummary)
{
    Infra::DataInventory inventory;
    L2CacheTaskCalculator calculator;
    StartTestLogCapture();
    const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c"));
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "l2 cache parser data is empty"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
}

TEST_F(L2CacheTaskCalculatorUTest, EmptyEventsSkipsSummaryEvenWithInventory)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "")));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
}

TEST_F(L2CacheTaskCalculatorUTest, HaSocPmuPreloadedInventoryProducesSummary)
{
    // HA-style: Calculator consumes preloaded inventory without L2 binary on
    // disk.
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(4, 5, 6, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_EQ(4U, summary->at(0).taskType);
    EXPECT_EQ(5U, summary->at(0).streamId);
    EXPECT_EQ(6U, summary->at(0).taskId);
    EXPECT_DOUBLE_EQ(0.333333, summary->at(0).hitRate);
}

TEST_F(L2CacheTaskCalculatorUTest, InvalidPlatformDoesNotWriteSummary)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V5_1_0, "0x59,0x5b,0x5c")));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
}

TEST_F(L2CacheTaskCalculatorUTest, MatchesPythonHitFiveOverTwoMillion)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {2000000, 5, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.000003, summary->at(0).hitRate);
}

TEST_F(L2CacheTaskCalculatorUTest, MatchesPythonNegativeHitViaPlatform15)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {2000000, 2000005, 0, 0, 0, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V6_1_0, "0x00,0x81,0x82,0x83,0x74,0x75")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(-0.000003, summary->at(0).hitRate);
}

TEST_F(L2CacheTaskCalculatorUTest, WideCountCancellationAndNegativeDifferenceRemainSigned)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {100, 9223372036854775808ULL, 0, 0, 0, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V6_1_1, "0x00,0x81,0x82,0x83,0x74,0x75")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    EXPECT_LT(summary->at(0).hitRate, 0.0);
}

TEST_F(L2CacheTaskCalculatorUTest, Uint64MaximumOutsideSignedResultRangeReturnsError)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {18446744073709551615ULL, 1, 0}));
    L2CacheTaskCalculator calculator;
    EXPECT_EQ(ANALYSIS_ERROR, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
}

TEST_F(L2CacheTaskCalculatorUTest, HaPreloadedMergedInventoryMatchesPython)
{
    // Preload HA + L2 rows in inventory (no DB merge).
    const std::vector<HalL2CacheRaw> rows = {
        MakeRaw(0, 2, 3, {300, 100, 120, 60, 40}),
        MakeRaw(0, 2, 4, {3, 0, 1, 0, 0}),
        MakeRaw(1, 10, 100, {100, 100, 50, 30, 20}),
        MakeRaw(1, 11, 101, {7, 0, 3, 0, 1}),
    };
    Infra::DataInventory inventory;
    InjectRaws(inventory, rows);
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V4_1_0, "0xfb,0xfc,0x90,0x91,0x9c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(4UL, summary->size());
    EXPECT_DOUBLE_EQ(0.45, summary->at(0).hitRate);
    EXPECT_DOUBLE_EQ(0.1, summary->at(0).victimRate);
    EXPECT_DOUBLE_EQ(0.333333, summary->at(1).hitRate);
    EXPECT_DOUBLE_EQ(0.4, summary->at(2).hitRate);
    EXPECT_DOUBLE_EQ(0.428571, summary->at(3).hitRate);
    EXPECT_DOUBLE_EQ(0.142857, summary->at(3).victimRate);
}

TEST_F(L2CacheTaskCalculatorUTest, TaskRowsProduceSummaryFromContextEvents)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {100, 50, 10, 0, 0, 0, 0, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V4_1_0, "0xfb,0xfc,0x90,0x91,0x9c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(0.066667, summary->front().hitRate);
    EXPECT_DOUBLE_EQ(0.0, summary->front().victimRate);
}


TEST_F(L2CacheTaskCalculatorUTest, SkipSuccessCasesDoNotLogError)
{
    {
        Infra::DataInventory inventory;
        L2CacheTaskCalculator calculator;
        StartTestLogCapture();
        const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V2_1_0, ""));
        const auto logs = StopTestLogCapture();
        ASSERT_EQ(ANALYSIS_OK, status);
        EXPECT_TRUE(HasLogMessage(logs, "[INFO]", "l2 cache events are empty"));
        EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
        EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    }
    {
        Infra::DataInventory inventory;
        InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
        L2CacheTaskCalculator calculator;
        StartTestLogCapture();
        const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V5_1_0, "0x59,0x5b,0x5c"));
        const auto logs = StopTestLogCapture();
        ASSERT_EQ(ANALYSIS_OK, status);
        EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "invalid platform"));
        EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
        EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    }
    {
        Infra::DataInventory inventory;
        InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
        L2CacheTaskCalculator calculator;
        StartTestLogCapture();
        const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59"));
        const auto logs = StopTestLogCapture();
        ASSERT_EQ(ANALYSIS_OK, status);
        EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "invalid l2 cache events"));
        EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
        EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    }
}

TEST_F(L2CacheTaskCalculatorUTest, InvalidEventNameWarnsAndSkipsWithoutError)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    StartTestLogCapture();
    const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,not_an_event"));
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "Option --L2_cache_events value should be in the L2 cache event list"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
}

TEST_F(L2CacheTaskCalculatorUTest, NineLegalEventsWarnsAndSkipsWithoutError)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {3, 1, 0}));
    L2CacheTaskCalculator calculator;
    StartTestLogCapture();
    const uint32_t status =
        calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c,0x62,0x6a,0x6c,0x71,0x74,0x77"));
    const auto logs = StopTestLogCapture();
    ASSERT_EQ(ANALYSIS_OK, status);
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
    EXPECT_TRUE(HasLogMessage(logs, "[WARN]", "Option --L2_cache_events number should less than"));
    EXPECT_FALSE(HasLogLevel(logs, "[ERROR]"));
}

TEST_F(L2CacheTaskCalculatorUTest, OverflowStillLogsErrorAndFails)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {18446744073709551615ULL, 1, 0}));
    L2CacheTaskCalculator calculator;
    StartTestLogCapture();
    const uint32_t status = calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c"));
    const auto logs = StopTestLogCapture();
    EXPECT_EQ(ANALYSIS_ERROR, status);
    EXPECT_TRUE(HasLogLevel(logs, "[ERROR]"));
    EXPECT_EQ(nullptr, inventory.GetPtr<std::vector<L2CacheSummaryRow>>());
}

// Frozen from /usr/bin/python3 3.13.5: round(x, 6) of exact binary64 values.
// 1/128 -> 0.007812; 3/128 -> 0.023438; -1/128 -> -0.007812; -3/128 -> -0.023438;
// 246913/2000000 (0.1234565) -> 0.123456; neighbors and tighter half sides below.
TEST_F(L2CacheTaskCalculatorUTest, SixDecimalHalfEvenBoundariesMatchPythonRound)
{
    const char *v2 = "0x59,0x5b,0x5c";
    const char *v6 = "0x00,0x81,0x82,0x83,0x74,0x75";
    struct Case
    {
        uint32_t chip;
        const char *events;
        std::vector<uint64_t> raw;
        double expected;
    };
    const std::vector<Case> cases = {
        {CHIP_V2_1_0, v2, {128, 1, 0}, 0.007812},
        {CHIP_V2_1_0, v2, {128, 3, 0}, 0.023438},
        {CHIP_V6_1_0, v6, {128, 129, 0, 0, 0, 0}, -0.007812},
        {CHIP_V6_1_0, v6, {128, 131, 0, 0, 0, 0}, -0.023438},
        {CHIP_V2_1_0, v2, {2000000, 246913, 0}, 0.123456},
        {CHIP_V2_1_0, v2, {128000000, 999999, 0}, 0.007812},
        {CHIP_V2_1_0, v2, {128000000, 1000001, 0}, 0.007813},
        {CHIP_V2_1_0, v2, {128000000, 2999999, 0}, 0.023437},
        {CHIP_V2_1_0, v2, {128000000, 3000001, 0}, 0.023438},
        {CHIP_V6_1_0, v6, {128000000, 128999999, 0, 0, 0, 0}, -0.007812},
        {CHIP_V6_1_0, v6, {128000000, 129000001, 0, 0, 0, 0}, -0.007813},
        {CHIP_V2_1_0, v2, {2000000, 246912, 0}, 0.123456},
        {CHIP_V2_1_0, v2, {2000000, 246914, 0}, 0.123457},
    };
    for (const auto &item : cases)
    {
        Infra::DataInventory inventory;
        InjectRaw(inventory, MakeRaw(1, 2, 3, item.raw));
        L2CacheTaskCalculator calculator;
        ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(item.chip, item.events)))
            << "expected " << item.expected;
        const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
        ASSERT_NE(nullptr, summary);
        ASSERT_EQ(1UL, summary->size());
        EXPECT_DOUBLE_EQ(item.expected, summary->at(0).hitRate) << "expected " << item.expected;
    }
}

TEST_F(L2CacheTaskCalculatorUTest, HalfMicroHalfEvenGoesToSignedZero)
{
    // 1/2000000 = 0.5e-6, Python round(..., 6) is signed zero (even).
    {
        Infra::DataInventory inventory;
        InjectRaw(inventory, MakeRaw(1, 2, 3, {2000000, 1, 0}));
        L2CacheTaskCalculator calculator;
        ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
        const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
        ASSERT_NE(nullptr, summary);
        ASSERT_EQ(1UL, summary->size());
        EXPECT_DOUBLE_EQ(0.0, summary->at(0).hitRate);
        EXPECT_FALSE(std::signbit(summary->at(0).hitRate));
    }
    {
        Infra::DataInventory inventory;
        InjectRaw(inventory, MakeRaw(1, 2, 3, {2000000, 2000001, 0, 0, 0, 0}));
        L2CacheTaskCalculator calculator;
        ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V6_1_0, "0x00,0x81,0x82,0x83,0x74,0x75")));
        const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
        ASSERT_NE(nullptr, summary);
        ASSERT_EQ(1UL, summary->size());
        EXPECT_DOUBLE_EQ(0.0, summary->at(0).hitRate);
        EXPECT_TRUE(std::signbit(summary->at(0).hitRate));
    }
}

TEST_F(L2CacheTaskCalculatorUTest, TwoPow33RatioKeepsOriginalDouble)
{
    Infra::DataInventory inventory;
    InjectRaw(inventory, MakeRaw(1, 2, 3, {1, 8589934592ULL, 0}));
    L2CacheTaskCalculator calculator;
    ASSERT_EQ(ANALYSIS_OK, calculator.Run(inventory, CreateContext(CHIP_V2_1_0, "0x59,0x5b,0x5c")));
    const auto summary = inventory.GetPtr<std::vector<L2CacheSummaryRow>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1UL, summary->size());
    EXPECT_DOUBLE_EQ(8589934592.0, summary->at(0).hitRate);
}

}  // namespace Domain
}  // namespace Analysis
