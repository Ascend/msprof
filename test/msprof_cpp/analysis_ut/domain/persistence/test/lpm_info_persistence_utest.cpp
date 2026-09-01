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

#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/freq_persistence.h"
#include "analysis/csrc/domain/services/persistence/device/lpm_info_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Utils;

namespace {
const std::string DEVICE_PATH = "./lpm_info_device_0";
const std::string SQLITE_PATH = "./lpm_info_device_0/sqlite";
using QueryData = std::vector<std::tuple<uint64_t, uint32_t>>;
}  // namespace

class LpmInfoPersistenceUTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(File::CreateDir(DEVICE_PATH));
        ASSERT_TRUE(File::CreateDir(SQLITE_PATH));
        context_.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        ASSERT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

    DataInventory dataInventory_;
    DeviceContext context_;
};

TEST_F(LpmInfoPersistenceUTest, ShouldSaveFreqAndVoltageData)
{
    auto data = std::make_shared<HalLpmInfoData>();
    data->freqData.emplace_back(100, 800);
    data->freqData.emplace_back(120, 1800);
    data->aicVoltageData.emplace_back(100, 725);
    data->busVoltageData.emplace_back(100, 850);
    ASSERT_TRUE(dataInventory_.Inject(data));

    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    QueryData freqResult;
    DBRunner freqRunner(File::PathJoin({SQLITE_PATH, "freq.db"}));
    ASSERT_TRUE(freqRunner.QueryData("SELECT syscnt, freq FROM FreqParse ORDER BY rowid", freqResult));
    ASSERT_EQ(2U, freqResult.size());
    EXPECT_EQ(100U, std::get<0>(freqResult[0]));
    EXPECT_EQ(800U, std::get<1>(freqResult[0]));
    EXPECT_EQ(120U, std::get<0>(freqResult[1]));
    EXPECT_EQ(1800U, std::get<1>(freqResult[1]));

    QueryData aicVoltageResult;
    QueryData busVoltageResult;
    DBRunner voltageRunner(File::PathJoin({SQLITE_PATH, "voltage.db"}));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM AicVoltage ORDER BY rowid", aicVoltageResult));
    ASSERT_TRUE(voltageRunner.QueryData("SELECT syscnt, voltage FROM BusVoltage ORDER BY rowid", busVoltageResult));
    ASSERT_EQ(1U, aicVoltageResult.size());
    EXPECT_EQ(725U, std::get<1>(aicVoltageResult[0]));
    ASSERT_EQ(1U, busVoltageResult.size());
    EXPECT_EQ(850U, std::get<1>(busVoltageResult[0]));
}

TEST_F(LpmInfoPersistenceUTest, ShouldAppendLpmFrequencyToExistingFrequencyData)
{
    auto freqData = std::make_shared<std::vector<HalFreqLpmData>>();
    HalFreqLpmData originalFreqData;
    originalFreqData.sysCnt = 90;
    originalFreqData.freq = 1600;
    freqData->push_back(originalFreqData);
    ASSERT_TRUE(dataInventory_.Inject(freqData));
    auto lpmInfoData = std::make_shared<HalLpmInfoData>();
    lpmInfoData->freqData.emplace_back(100, 1850);
    lpmInfoData->freqData.emplace_back(120, 1800);
    ASSERT_TRUE(dataInventory_.Inject(lpmInfoData));

    FreqPersistence freqPersistence;
    LpmInfoPersistence lpmInfoPersistence;
    ASSERT_EQ(ANALYSIS_OK, freqPersistence.Run(dataInventory_, context_));
    ASSERT_EQ(ANALYSIS_OK, lpmInfoPersistence.Run(dataInventory_, context_));

    QueryData freqResult;
    DBRunner freqRunner(File::PathJoin({SQLITE_PATH, "freq.db"}));
    ASSERT_TRUE(freqRunner.QueryData("SELECT syscnt, freq FROM FreqParse ORDER BY rowid", freqResult));
    ASSERT_EQ(3U, freqResult.size());
    EXPECT_EQ(90U, std::get<0>(freqResult[0]));
    EXPECT_EQ(1600U, std::get<1>(freqResult[0]));
    EXPECT_EQ(100U, std::get<0>(freqResult[1]));
    EXPECT_EQ(1850U, std::get<1>(freqResult[1]));
    EXPECT_EQ(120U, std::get<0>(freqResult[2]));
    EXPECT_EQ(1800U, std::get<1>(freqResult[2]));
}

TEST_F(LpmInfoPersistenceUTest, ShouldNotCreateDatabaseWhenDataIsEmpty)
{
    auto data = std::make_shared<HalLpmInfoData>();
    ASSERT_TRUE(dataInventory_.Inject(data));

    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    EXPECT_FALSE(File::Exist(File::PathJoin({SQLITE_PATH, "freq.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({SQLITE_PATH, "voltage.db"})));
}

TEST_F(LpmInfoPersistenceUTest, ShouldCreateOnlyFreqDbWhenVoltageIsEmpty)
{
    auto data = std::make_shared<HalLpmInfoData>();
    data->freqData.emplace_back(100, 1850);
    ASSERT_TRUE(dataInventory_.Inject(data));

    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    EXPECT_TRUE(File::Exist(File::PathJoin({SQLITE_PATH, "freq.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({SQLITE_PATH, "voltage.db"})));
}

TEST_F(LpmInfoPersistenceUTest, ShouldCreateOnlyVoltageDbWhenFreqIsEmpty)
{
    auto data = std::make_shared<HalLpmInfoData>();
    data->aicVoltageData.emplace_back(100, 725);
    ASSERT_TRUE(dataInventory_.Inject(data));

    LpmInfoPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    EXPECT_FALSE(File::Exist(File::PathJoin({SQLITE_PATH, "freq.db"})));
    EXPECT_TRUE(File::Exist(File::PathJoin({SQLITE_PATH, "voltage.db"})));
}

TEST_F(LpmInfoPersistenceUTest, ShouldReturnErrorWhenDataIsMissing)
{
    LpmInfoPersistence persistence;
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
}
}  // namespace Domain
}  // namespace Analysis
