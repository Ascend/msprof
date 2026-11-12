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

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/pcie_persistence.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
namespace {
const std::string PCIE_ROOT = "./pcie_persistence";
const std::string PCIE_DEVICE_DIR = "./pcie_persistence/device_0";
const std::string PCIE_SQLITE_DIR = "./pcie_persistence/device_0/sqlite";
const std::string PCIE_DB_PATH = "./pcie_persistence/device_0/sqlite/pcie.db";
using PcieRow = std::tuple<uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>;

HalPcieData MakePcieData(uint64_t timestamp, uint32_t deviceId, uint32_t firstMetric)
{
    HalPcieData data{};
    data.timestamp = timestamp;
    data.deviceId = deviceId;
    data.txPBandwidthMin = firstMetric;
    data.txPBandwidthMax = firstMetric + 1;
    data.txPBandwidthAvg = firstMetric + 2;
    data.txNpBandwidthMin = firstMetric + 3;
    data.txNpBandwidthMax = firstMetric + 4;
    data.txNpBandwidthAvg = firstMetric + 5;
    data.txCplBandwidthMin = firstMetric + 6;
    data.txCplBandwidthMax = firstMetric + 7;
    data.txCplBandwidthAvg = firstMetric + 8;
    data.txNpLatencyMin = firstMetric + 9;
    data.txNpLatencyMax = firstMetric + 10;
    data.txNpLatencyAvg = firstMetric + 11;
    data.rxPBandwidthMin = firstMetric + 12;
    data.rxPBandwidthMax = firstMetric + 13;
    data.rxPBandwidthAvg = firstMetric + 14;
    data.rxNpBandwidthMin = firstMetric + 15;
    data.rxNpBandwidthMax = firstMetric + 16;
    data.rxNpBandwidthAvg = firstMetric + 17;
    data.rxCplBandwidthMin = firstMetric + 18;
    data.rxCplBandwidthMax = firstMetric + 19;
    data.rxCplBandwidthAvg = firstMetric + 20;
    return data;
}

PcieRow ToRow(const HalPcieData &data)
{
    return {data.timestamp, data.deviceId, data.txPBandwidthMin, data.txPBandwidthMax, data.txPBandwidthAvg,
            data.txNpBandwidthMin, data.txNpBandwidthMax, data.txNpBandwidthAvg, data.txCplBandwidthMin,
            data.txCplBandwidthMax, data.txCplBandwidthAvg, data.txNpLatencyMin, data.txNpLatencyMax,
            data.txNpLatencyAvg, data.rxPBandwidthMin, data.rxPBandwidthMax, data.rxPBandwidthAvg,
            data.rxNpBandwidthMin, data.rxNpBandwidthMax, data.rxNpBandwidthAvg, data.rxCplBandwidthMin,
            data.rxCplBandwidthMax, data.rxCplBandwidthAvg};
}
}  // namespace

class PciePersistenceUtest : public testing::Test {
protected:
    void SetUp() override
    {
        if (Utils::File::Exist(PCIE_ROOT)) {
            ASSERT_TRUE(Utils::File::RemoveDir(PCIE_ROOT, 0));
        }
        ASSERT_TRUE(Utils::File::CreateDir(PCIE_ROOT));
        ASSERT_TRUE(Utils::File::CreateDir(PCIE_DEVICE_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(PCIE_SQLITE_DIR));
        records_ = std::make_shared<std::vector<HalPcieData>>();
        ASSERT_TRUE(dataInventory_.Inject(records_));
        context_.deviceContextInfo.deviceFilePath = PCIE_DEVICE_DIR;
        context_.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        if (Utils::File::Exist(PCIE_ROOT)) {
            ASSERT_TRUE(Utils::File::RemoveDir(PCIE_ROOT, 0));
        }
    }

    uint32_t Run(const std::vector<HalPcieData> &records)
    {
        *records_ = records;
        PciePersistence persistence;
        return persistence.Run(dataInventory_, context_);
    }

    Infra::DataInventory dataInventory_;
    DeviceContext context_;
    std::shared_ptr<std::vector<HalPcieData>> records_;
};

TEST_F(PciePersistenceUtest, ShouldSaveAllTwentyThreeColumnsInParserOrder)
{
    const auto first = MakePcieData(UINT64_C(0x100000001), UINT32_MAX, 1);
    const auto second = MakePcieData(UINT64_C(0x200000002), 7, 101);
    ASSERT_EQ(ANALYSIS_OK, Run({first, second}));

    Infra::DBRunner runner(PCIE_DB_PATH);
    std::vector<PcieRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PcieOriginalData ORDER BY rowid", rows));
    ASSERT_EQ(2UL, rows.size());
    EXPECT_EQ(ToRow(first), rows[0]);
    EXPECT_EQ(ToRow(second), rows[1]);
    std::vector<std::tuple<std::string>> stageTables;
    ASSERT_TRUE(runner.QueryData(
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'PcieOriginalDataStage_%'", stageTables));
    EXPECT_TRUE(stageTables.empty());
}

TEST_F(PciePersistenceUtest, ShouldReplaceRowsWhenSameRawDataIsParsedAgain)
{
    const auto first = MakePcieData(100, 0, 1);
    const auto second = MakePcieData(200, 0, 101);
    ASSERT_EQ(ANALYSIS_OK, Run({first, second}));
    ASSERT_EQ(ANALYSIS_OK, Run({first, second}));

    Infra::DBRunner runner(PCIE_DB_PATH);
    std::vector<PcieRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PcieOriginalData ORDER BY rowid", rows));
    ASSERT_EQ(2UL, rows.size());
    EXPECT_EQ(ToRow(first), rows[0]);
    EXPECT_EQ(ToRow(second), rows[1]);

    const auto replacement = MakePcieData(300, 0, 201);
    ASSERT_EQ(ANALYSIS_OK, Run({replacement}));
    rows.clear();
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PcieOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(ToRow(replacement), rows[0]);
}

TEST_F(PciePersistenceUtest, ShouldMapColumnsByNameWhenExistingTableOrderDiffers)
{
    Infra::PCIeDB database;
    auto columns = database.GetTableCols("PcieOriginalData");
    ASSERT_EQ(23UL, columns.size());
    std::swap(columns[2], columns[3]);
    Infra::DBRunner runner(PCIE_DB_PATH);
    ASSERT_TRUE(runner.CreateTable("PcieOriginalData", columns));

    const auto data = MakePcieData(100, 0, 11);
    ASSERT_EQ(ANALYSIS_OK, Run({data}));
    std::vector<std::tuple<uint32_t, uint32_t>> metrics;
    ASSERT_TRUE(runner.QueryData(
        "SELECT tx_p_bandwidth_min, tx_p_bandwidth_max FROM PcieOriginalData", metrics));
    ASSERT_EQ(1UL, metrics.size());
    EXPECT_EQ(data.txPBandwidthMin, std::get<0>(metrics[0]));
    EXPECT_EQ(data.txPBandwidthMax, std::get<1>(metrics[0]));
}

TEST_F(PciePersistenceUtest, ShouldKeepPreviousRowsIfReplacementInsertFails)
{
    const auto original = MakePcieData(100, 0, 1);
    ASSERT_EQ(ANALYSIS_OK, Run({original}));
    Infra::DBRunner runner(PCIE_DB_PATH);
    ASSERT_TRUE(runner.UpdateData(
        "CREATE TRIGGER reject_pcie_insert BEFORE INSERT ON PcieOriginalData "
        "BEGIN SELECT RAISE(FAIL, 'injected insert failure'); END;"));

    EXPECT_EQ(ANALYSIS_ERROR, Run({MakePcieData(200, 0, 101)}));
    std::vector<PcieRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PcieOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(ToRow(original), rows[0]);
    std::vector<std::tuple<std::string>> stageTables;
    ASSERT_TRUE(runner.QueryData(
        "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'PcieOriginalDataStage_%'", stageTables));
    EXPECT_TRUE(stageTables.empty());

    ASSERT_TRUE(runner.UpdateData("DROP TRIGGER reject_pcie_insert;"));
    const auto replacement = MakePcieData(200, 0, 101);
    ASSERT_EQ(ANALYSIS_OK, Run({replacement}));
    rows.clear();
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PcieOriginalData", rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(ToRow(replacement), rows[0]);
}

TEST_F(PciePersistenceUtest, ShouldNotCreateDatabaseForEmptyInput)
{
    EXPECT_EQ(ANALYSIS_OK, Run({}));
    EXPECT_FALSE(Utils::File::Exist(PCIE_DB_PATH));
}

TEST_F(PciePersistenceUtest, ShouldReturnErrorForMissingData)
{
    dataInventory_.RemoveRestData({});
    PciePersistence persistence;
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
    EXPECT_FALSE(Utils::File::Exist(PCIE_DB_PATH));
}

TEST_F(PciePersistenceUtest, ShouldReturnErrorWhenSqliteDirectoryIsMissing)
{
    ASSERT_TRUE(Utils::File::RemoveDir(PCIE_SQLITE_DIR, 0));
    EXPECT_EQ(ANALYSIS_ERROR, Run({MakePcieData(1, 0, 1)}));
    EXPECT_FALSE(Utils::File::Exist(PCIE_DB_PATH));
}

TEST_F(PciePersistenceUtest, ShouldReturnErrorWhenDatabasePathIsDirectory)
{
    ASSERT_TRUE(Utils::File::CreateDir(PCIE_DB_PATH));
    EXPECT_EQ(ANALYSIS_ERROR, Run({MakePcieData(1, 0, 1)}));
}

}  // namespace Domain
}  // namespace Analysis
