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

#include <string>
#include <tuple>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/persistence/device/chip_trans_persistence.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const std::string DEVICE_PATH = "./chip_trans_device";
const std::string DB_PATH = "./chip_trans_device/sqlite/chip_trans.db";
constexpr double HWTS_FREQUENCY_MHZ = 100.0;
constexpr uint64_t REFERENCE_SYSCNT = 1000;
constexpr uint64_t HOST_MONOTONIC_NS = 5000;
constexpr uint64_t CHIP_TRANS_SYSCNT = 1100;
constexpr double EXPECTED_TIMESTAMP_NS = 6000.0;
constexpr uint64_t INVALID_BW = 0xFFFFFFFFFFFFFFFFULL;
using PcieV6QueryRow = std::tuple<int32_t, double, uint64_t, uint64_t>;
using PaQueryRow = std::tuple<int32_t, std::string, std::string, double>;
using PcieQueryRow = std::tuple<int32_t, uint64_t, uint64_t, double>;
}

class ChipTransPersistenceUTest : public testing::Test {
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

TEST_F(ChipTransPersistenceUTest, ShouldSaveV6PcieInfoAndFilterInvalidBandwidth)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData valid;
    valid.type = SOC_PROFILE_CHIP_TRANS;
    valid.chipTrans.sysCnt = CHIP_TRANS_SYSCNT;
    valid.chipTrans.transType = CHIP_TRANS_V6;
    valid.chipTrans.id = 2;
    valid.chipTrans.value1 = 300;
    valid.chipTrans.value2 = 400;
    source->items.push_back(valid);

    HalLogData invalid = valid;
    invalid.chipTrans.id = 3;
    invalid.chipTrans.value1 = INVALID_BW;
    source->items.push_back(invalid);
    invalid.chipTrans.value1 = 300;
    invalid.chipTrans.value2 = INVALID_BW;
    source->items.push_back(invalid);
    invalid.chipTrans.value1 = INVALID_BW;
    source->items.push_back(invalid);
    invalid.chipTrans.transType = CHIP_TRANS_INVALID;
    source->items.push_back(invalid);
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<PcieV6QueryRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT die_id, sys_time, pcie_write_bandwidth, pcie_read_bandwidth FROM PcieInfoV6",
                                 rows));
    ASSERT_EQ(1UL, rows.size());
    ASSERT_EQ(2U, std::get<0>(rows[0]));
    ASSERT_DOUBLE_EQ(EXPECTED_TIMESTAMP_NS, std::get<1>(rows[0]));
    ASSERT_EQ(300ULL, std::get<2>(rows[0]));
    ASSERT_EQ(400ULL, std::get<3>(rows[0]));
}

TEST_F(ChipTransPersistenceUTest, ShouldSaveV4PaAndPcieTables)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData pa;
    pa.type = SOC_PROFILE_CHIP_TRANS;
    pa.chipTrans.sysCnt = CHIP_TRANS_SYSCNT;
    pa.chipTrans.transType = CHIP_TRANS_PA;
    pa.chipTrans.id = 8;
    pa.chipTrans.value1 = 111;
    pa.chipTrans.value2 = 222;
    source->items.push_back(pa);

    HalLogData pcie = pa;
    pcie.chipTrans.transType = CHIP_TRANS_PCIE;
    pcie.chipTrans.id = 9;
    pcie.chipTrans.value1 = 333;
    pcie.chipTrans.value2 = 444;
    source->items.push_back(pcie);
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<PaQueryRow> paRows;
    ASSERT_TRUE(runner.QueryData(
        "SELECT pa_link_id, pa_link_traffic_monit_rx, pa_link_traffic_monit_tx, sys_time FROM PaLinkInfo", paRows));
    ASSERT_EQ(1UL, paRows.size());
    ASSERT_EQ(8U, std::get<0>(paRows[0]));
    ASSERT_EQ("111", std::get<1>(paRows[0]));
    ASSERT_EQ("222", std::get<2>(paRows[0]));
    ASSERT_DOUBLE_EQ(EXPECTED_TIMESTAMP_NS, std::get<3>(paRows[0]));

    std::vector<PcieQueryRow> pcieRows;
    ASSERT_TRUE(runner.QueryData("SELECT pcie_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time FROM PcieInfo",
                                 pcieRows));
    ASSERT_EQ(1UL, pcieRows.size());
    ASSERT_EQ(9U, std::get<0>(pcieRows[0]));
    ASSERT_EQ(333ULL, std::get<1>(pcieRows[0]));
    ASSERT_EQ(444ULL, std::get<2>(pcieRows[0]));
    ASSERT_DOUBLE_EQ(EXPECTED_TIMESTAMP_NS, std::get<3>(pcieRows[0]));
}

TEST_F(ChipTransPersistenceUTest, ShouldNotCreateV6DatabaseWhenNoChipTransDataExists)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    EXPECT_FALSE(File::Exist(DB_PATH));
}

TEST_F(ChipTransPersistenceUTest, ShouldNotCreateDatabaseWhenAllBandwidthDataIsInvalid)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData item;
    item.chipTrans = HalChipTransData{};
    item.type = SOC_PROFILE_CHIP_TRANS;
    item.chipTrans.transType = CHIP_TRANS_V6;
    item.chipTrans.value1 = INVALID_BW;
    source->items.push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));
    EXPECT_FALSE(File::Exist(DB_PATH));
}

TEST_F(ChipTransPersistenceUTest, ShouldReturnErrorWhenDataIsNull)
{
    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context_));
}

TEST_F(ChipTransPersistenceUTest, ShouldSaveDefaultDieIdAsNegativeOne)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData item;
    item.chipTrans = HalChipTransData{};
    item.type = SOC_PROFILE_CHIP_TRANS;
    item.chipTrans.transType = CHIP_TRANS_V6;
    item.chipTrans.sysCnt = CHIP_TRANS_SYSCNT;
    source->items.push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<PcieV6QueryRow> rows;
    ASSERT_TRUE(runner.QueryData("SELECT die_id, sys_time, pcie_write_bandwidth, pcie_read_bandwidth FROM PcieInfoV6",
                                 rows));
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(-1, std::get<0>(rows[0]));
}

TEST_F(ChipTransPersistenceUTest, ShouldNotCreateLegacyDatabaseWhenNoChipTransDataExists)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    ASSERT_TRUE(dataInventory_.Inject(std::make_shared<HalSocProfileData>()));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    EXPECT_FALSE(File::Exist(DB_PATH));
}

TEST_F(ChipTransPersistenceUTest, ShouldNotCreatePcieTableWhenOnlyPaDataExists)
{
    context_.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    auto source = std::make_shared<HalSocProfileData>();
    HalLogData item;
    item.chipTrans = HalChipTransData{};
    item.type = SOC_PROFILE_CHIP_TRANS;
    item.chipTrans.transType = CHIP_TRANS_PA;
    item.chipTrans.sysCnt = CHIP_TRANS_SYSCNT;
    item.chipTrans.id = 8;
    source->items.push_back(item);
    ASSERT_TRUE(dataInventory_.Inject(source));

    ChipTransPersistence persistence;
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context_));

    DBRunner runner(DB_PATH);
    std::vector<PaQueryRow> paRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM PaLinkInfo", paRows));
    ASSERT_EQ(1UL, paRows.size());
    EXPECT_EQ(8, std::get<0>(paRows[0]));
    EXPECT_FALSE(runner.CheckTableExists("PcieInfo"));
}

}  // namespace Domain
}  // namespace Analysis
