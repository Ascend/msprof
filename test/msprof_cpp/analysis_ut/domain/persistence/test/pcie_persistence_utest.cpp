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
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/pcie_persistence.h"
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
