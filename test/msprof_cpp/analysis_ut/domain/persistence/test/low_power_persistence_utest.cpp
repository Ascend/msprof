/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * -------------------------------------------------------------------------*/

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"

#include <tuple>

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/low_power_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const std::string DEVICE_PATH = "./low_power_device";
const std::string DB_PATH = "./low_power_device/sqlite/lowpower.db";
constexpr size_t LOW_POWER_BASE_COLUMN_COUNT = 2;
constexpr double HWTS_FREQUENCY_MHZ = 100.0;
constexpr uint64_t REFERENCE_SYSCNT = 1000;
constexpr uint64_t HOST_MONOTONIC_NS = 5000;
constexpr uint64_t LOW_POWER_SYSCNT = 1100;
constexpr double EXPECTED_TIMESTAMP_NS = 6000.0;
}

class LowPowerPersistenceUTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(File::CreateDir(DEVICE_PATH));
        ASSERT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, SQLITE})));
        context_.deviceContextInfo.deviceFilePath = DEVICE_PATH;
        context_.deviceContextInfo.deviceInfo.hwtsFrequency = HWTS_FREQUENCY_MHZ;
        context_.deviceContextInfo.deviceStart.cntVct = REFERENCE_SYSCNT;
        context_.deviceContextInfo.hostStartLog.clockMonotonicRaw = HOST_MONOTONIC_NS;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

    DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(LowPowerPersistenceUTest, ShouldSavePythonCompatibleLowPowerTableWithNanosecondTimestamp)
{
    auto source = std::make_shared<std::vector<HalSocProfileData>>();
    HalSocProfileData item;
    item.type = SOC_PROFILE_LOW_POWER;
    item.lowPower.sysCnt = LOW_POWER_SYSCNT;
    item.lowPower.dieId = 3;
    for (uint32_t i = 0; i < LOW_POWER_SAMPLE_COUNT; ++i) {
        item.lowPower.sampleData[i] = static_cast<uint16_t>(10 + i);
    }
    source->push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    LowPowerPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<std::tuple<double, uint32_t, uint32_t, uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT timestamp, die_id, data0_hard, data9_soft FROM LowPower", rows));
    ASSERT_EQ(1UL, rows.size());
    ASSERT_DOUBLE_EQ(EXPECTED_TIMESTAMP_NS, std::get<0>(rows[0]));
    ASSERT_EQ(3U, std::get<1>(rows[0]));
    ASSERT_EQ(10U, std::get<2>(rows[0]));
    ASSERT_EQ(29U, std::get<3>(rows[0]));

    const auto columns = runner.GetTableColumns("LowPower");
    ASSERT_EQ(LOW_POWER_BASE_COLUMN_COUNT + LOW_POWER_SAMPLE_COUNT, columns.size());
    for (const auto& column : columns) {
        ASSERT_EQ("NUMERIC", column.type);
    }
}

TEST_F(LowPowerPersistenceUTest, ShouldNotCreateDatabaseForEmptyData)
{
    auto source = std::make_shared<std::vector<HalSocProfileData>>();
    ASSERT_TRUE(dataInventory_.Inject(source));

    LowPowerPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    ASSERT_FALSE(File::Exist(DB_PATH));
}

TEST_F(LowPowerPersistenceUTest, ShouldNotCreateDatabaseWhenNoLowPowerDataRemainsAfterFiltering)
{
    auto source = std::make_shared<std::vector<HalSocProfileData>>();
    source->emplace_back();
    ASSERT_TRUE(dataInventory_.Inject(source));

    LowPowerPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    ASSERT_FALSE(File::Exist(DB_PATH));
}

TEST_F(LowPowerPersistenceUTest, ShouldReturnErrorWhenSavingLowPowerDataFails)
{
    auto source = std::make_shared<std::vector<HalSocProfileData>>();
    HalSocProfileData item;
    item.type = SOC_PROFILE_LOW_POWER;
    item.lowPower.sysCnt = LOW_POWER_SYSCNT;
    source->push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    LowPowerPersistence persistence;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}

}  // namespace Domain
}  // namespace Analysis
