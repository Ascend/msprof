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

#include <tuple>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/acc_pmu_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace Analysis {
namespace Domain {
namespace {
const std::string DEVICE_PATH = "./device_0";
const std::string DB_PATH = "./device_0/sqlite/acc_pmu.db";
const int ACC_PMU_SIZE = 2;
constexpr double HWTS_FREQUENCY_MHZ = 100.0;
// acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost
using AccPmuQueryRow = std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t>;
const std::string ACC_PMU_QUERY_SQL = "SELECT acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost FROM AccPmu";
}

class AccPmuPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::vector<HalLogData>>();
        dataInventory_.Inject(data);
        context_.deviceContextInfo.deviceFilePath = DEVICE_PATH;
        context_.deviceContextInfo.deviceInfo.hwtsFrequency = HWTS_FREQUENCY_MHZ;
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
    DeviceContext context_;
};

static std::vector<HalLogData> CreateAccPmuData(int accId, int timestamp, int ost)
{
    std::vector<HalLogData> res;
    HalLogData logData{};
    logData.type = ACC_PMU;
    logData.accPmu.accId = accId;
    logData.accPmu.timestamp = timestamp;
    for (int i = 0; i < ACC_PMU_SIZE; ++i) {
        logData.accPmu.bandwidth[i] = i * ost;
        logData.accPmu.ost[i] = i * ost;
    }
    res.push_back(logData);
    return res;
}

TEST_F(AccPmuPersistenceUTest, ShouldSaveAccPmuDataSuccess)
{
    auto accPmuS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    auto accPmu = CreateAccPmuData(1, 10000, 5000);
    accPmuS->swap(accPmu);

    AccPmuPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<AccPmuQueryRow> rows;
    ASSERT_TRUE(runner.QueryData(ACC_PMU_QUERY_SQL, rows));
    ASSERT_EQ(1UL, rows.size());
    ASSERT_EQ(1U, std::get<0>(rows[0]));
    ASSERT_EQ(0ULL, std::get<1>(rows[0]));
    ASSERT_EQ(5000ULL, std::get<2>(rows[0]));
    ASSERT_EQ(0ULL, std::get<3>(rows[0]));
    ASSERT_EQ(5000ULL, std::get<4>(rows[0]));
}

TEST_F(AccPmuPersistenceUTest, ShouldSaveAccPmuDataFromSocProfile)
{
    auto source = std::make_shared<HalSocProfileData>();
    source->items = CreateAccPmuData(2, 20000, 6000);
    ASSERT_TRUE(dataInventory_.Inject(source));

    AccPmuPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<AccPmuQueryRow> rows;
    ASSERT_TRUE(runner.QueryData(ACC_PMU_QUERY_SQL, rows));
    ASSERT_EQ(1UL, rows.size());
    ASSERT_EQ(2U, std::get<0>(rows[0]));
    ASSERT_EQ(0ULL, std::get<1>(rows[0]));
    ASSERT_EQ(6000ULL, std::get<2>(rows[0]));
    ASSERT_EQ(0ULL, std::get<3>(rows[0]));
    ASSERT_EQ(6000ULL, std::get<4>(rows[0]));
}

TEST_F(AccPmuPersistenceUTest, ShouldSaveAccPmuDataFromBothSources)
{
    auto accPmuS = dataInventory_.GetPtr<std::vector<HalLogData>>();
    auto taskLog = CreateAccPmuData(1, 10000, 5000);
    accPmuS->swap(taskLog);

    auto source = std::make_shared<HalSocProfileData>();
    source->items = CreateAccPmuData(2, 20000, 6000);
    ASSERT_TRUE(dataInventory_.Inject(source));

    AccPmuPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<AccPmuQueryRow> rows;
    ASSERT_TRUE(runner.QueryData(ACC_PMU_QUERY_SQL, rows));
    ASSERT_EQ(2UL, rows.size());
    ASSERT_EQ(1U, std::get<0>(rows[0]));
    ASSERT_EQ(2U, std::get<0>(rows[1]));
}

TEST_F(AccPmuPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    AccPmuPersistence persistence;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
}
}
}