/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include <tuple>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/inter_soc_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const std::string DEVICE_PATH = "./inter_soc_device";
const std::string DB_PATH = "./inter_soc_device/sqlite/soc_profiler.db";
constexpr double HWTS_FREQUENCY_MHZ = 100.0;
constexpr uint64_t REFERENCE_SYSCNT = 1000;
constexpr uint64_t HOST_MONOTONIC_NS = 5000;
constexpr uint64_t INTER_SOC_SYSCNT = 1100;
constexpr double EXPECTED_TIMESTAMP_NS = 6000.0;
using InterSocQueryRow = std::tuple<uint32_t, uint32_t, double>;
}

class InterSocPersistenceUTest : public testing::Test {
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

TEST_F(InterSocPersistenceUTest, ShouldSaveInterSocDataFromSocProfile)
{
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData item;
    item.type = SOC_PROFILE_INTER_SOC;
    item.interSoc.sysCnt = INTER_SOC_SYSCNT;
    item.interSoc.l2BufferBwLevel = 3;
    item.interSoc.mataBwLevel = 5;
    source->items.push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    InterSocPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<InterSocQueryRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT l2_buffer_bw_level, mata_bw_level, sys_time FROM InterSoc", rows));
    ASSERT_EQ(1UL, rows.size());
    ASSERT_EQ(3U, std::get<0>(rows[0]));
    ASSERT_EQ(5U, std::get<1>(rows[0]));
    ASSERT_DOUBLE_EQ(EXPECTED_TIMESTAMP_NS, std::get<2>(rows[0]));
}

TEST_F(InterSocPersistenceUTest, ShouldNotCreateDatabaseForEmptyData)
{
    auto source = std::make_shared<HalSocProfileData>();
    ASSERT_TRUE(dataInventory_.Inject(source));

    InterSocPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    ASSERT_FALSE(File::Exist(DB_PATH));
}

TEST_F(InterSocPersistenceUTest, ShouldNotCreateDatabaseWhenNoInterSocDataRemainsAfterFiltering)
{
    auto source = std::make_shared<HalSocProfileData>();
    source->items.emplace_back();
    ASSERT_TRUE(dataInventory_.Inject(source));

    InterSocPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    ASSERT_FALSE(File::Exist(DB_PATH));
}

TEST_F(InterSocPersistenceUTest, ShouldReturnErrorWhenDataIsNull)
{
    InterSocPersistence persistence;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
}

}  // namespace Domain
}  // namespace Analysis