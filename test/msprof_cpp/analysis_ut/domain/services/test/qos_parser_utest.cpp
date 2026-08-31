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

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/qos/include/qos_parser.h"
#include "analysis/csrc/domain/services/persistence/device/qos_persistence.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string ROOT_DIR = "./qos_parser_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});

template <typename ValueType>
void WriteLittleEndian(std::vector<uint8_t> &buffer, size_t offset, ValueType value)
{
    for (size_t index = 0; index < sizeof(ValueType); ++index)
    {
        buffer[offset + index] = static_cast<uint8_t>((value >> (index * 8U)) & 0xffU);
    }
}

std::vector<uint8_t> CreateQosRecord(uint64_t syscnt, uint32_t baseBandwidth)
{
    std::vector<uint8_t> record(64, 0);
    WriteLittleEndian<uint64_t>(record, 16, syscnt);
    for (uint32_t index = 0; index < 10; ++index)
    {
        WriteLittleEndian<uint32_t>(record, 24 + index * sizeof(uint32_t), baseBandwidth + index);
    }
    return record;
}

std::vector<uint8_t> CreateStarsQosRecord(uint16_t dieId, uint64_t syscnt, uint32_t baseBandwidth)
{
    std::vector<uint8_t> record(64, 0);
    WriteLittleEndian<uint16_t>(record, 0, static_cast<uint16_t>((dieId << 10U) | 0x18U));
    WriteLittleEndian<uint16_t>(record, 2, 0x6bd3U);
    WriteLittleEndian<uint64_t>(record, 8, syscnt);
    for (uint32_t index = 0; index < 10; ++index)
    {
        WriteLittleEndian<uint32_t>(record, 24 + index * sizeof(uint32_t), baseBandwidth + index);
    }
    return record;
}

class QosParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    DeviceContext CreateContext(uint32_t chipId) const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        context.deviceContextInfo.deviceInfo.chipID = chipId;
        context.deviceContextInfo.deviceInfo.hwtsFrequency = 1000;
        context.deviceContextInfo.deviceStart.cntVct = 0;
        context.deviceContextInfo.hostStartLog.clockMonotonicRaw = 1000;
        return context;
    }

    std::vector<QosBwRow> QueryRows() const
    {
        std::shared_ptr<Infra::DBRunner> runner;
        MAKE_SHARED_RETURN_VALUE(runner, Infra::DBRunner, {}, Utils::File::PathJoin({SQLITE_DIR, "qos.db"}));
        std::vector<QosBwRow> rows;
        EXPECT_TRUE(runner->QueryData("SELECT * FROM QosBwData", rows));
        return rows;
    }
};

TEST_F(QosParserUTest, ShouldParseV4Qos)
{
    const std::string fileName = "qos.data.0.slice_0";
    std::vector<uint8_t> record = CreateQosRecord(100, 10);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, fileName));
    QosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V4_1_0)));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "qos.db"})));
    const auto parsedData = inventory.GetPtr<std::vector<HalQosBwData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(1100U, parsedData->at(0).timestamp);
    EXPECT_EQ(INVALID_VALUE, parsedData->at(0).dieId);
    EXPECT_EQ(10U, parsedData->at(0).metrics.front());
    EXPECT_EQ(19U, parsedData->at(0).metrics.back());
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V4_1_0)));
    const std::vector<QosBwRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(1100U, std::get<0>(rows[0]));
    EXPECT_EQ(INVALID_VALUE, std::get<1>(rows[0]));
    EXPECT_EQ(10U, std::get<2>(rows[0]));
    EXPECT_EQ(19U, std::get<11>(rows[0]));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({DATA_DIR, fileName}) + ".done"));
}

TEST_F(QosParserUTest, ShouldParseContinuousQosRecordAcrossSlicesAndSkipCompletedSource)
{
    const std::vector<uint8_t> firstRecord = CreateQosRecord(100, 10);
    const std::vector<uint8_t> secondRecord = CreateQosRecord(200, 20);
    std::vector<uint8_t> first(firstRecord.begin(), firstRecord.begin() + 20);
    std::vector<uint8_t> second(firstRecord.begin() + 20, firstRecord.end());
    second.insert(second.end(), secondRecord.begin(), secondRecord.end());
    std::vector<uint8_t> completed = CreateQosRecord(300, 30);
    ASSERT_TRUE(WriteBin(second, DATA_DIR, "qos.data.0.slice_10"));
    ASSERT_TRUE(WriteBin(first, DATA_DIR, "qos.data.0.slice_2"));
    ASSERT_TRUE(WriteBin(completed, DATA_DIR, "qos.data.0.slice_3.done"));
    QosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V4_1_0)));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V4_1_0)));
    const std::vector<QosBwRow> rows = QueryRows();
    ASSERT_EQ(2UL, rows.size());
    EXPECT_EQ(1100U, std::get<0>(rows[0]));
    EXPECT_EQ(1200U, std::get<0>(rows[1]));
    EXPECT_EQ(10U, std::get<2>(rows[0]));
    EXPECT_EQ(20U, std::get<2>(rows[1]));
}

TEST_F(QosParserUTest, ShouldReturnSuccessWhenQosSourceDoesNotExist)
{
    QosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V4_1_0)));
    EXPECT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V4_1_0)));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "qos.db"})));
}

TEST_F(QosParserUTest, ShouldReturnErrorWhenQosPersistenceFails)
{
    std::vector<uint8_t> record = CreateQosRecord(100, 10);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "qos.data.0.slice_0"));
    QosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    MOCKER_CPP(&Infra::DBRunner::CreateTable).stubs().will(returnValue(false));
    EXPECT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V4_1_0)));
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, CreateContext(CHIP_V4_1_0)));
    MOCKER_CPP(&Infra::DBRunner::CreateTable).reset();
}

TEST_F(QosParserUTest, ShouldReturnErrorWhenQosPersistenceHasNoInputData)
{
    QosPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, CreateContext(CHIP_V4_1_0)));
}

TEST_F(QosParserUTest, ShouldParseV6StarsQosWithoutCompletingSharedSourceFile)
{
    const std::string fileName = "stars_soc_profile.data.0.slice_0";
    std::vector<uint8_t> source = CreateStarsQosRecord(2, 200, 100);
    std::vector<uint8_t> unrelated = CreateStarsQosRecord(1, 300, 200);
    source[0] = 0;
    source.insert(source.end(), unrelated.begin(), unrelated.end());
    ASSERT_TRUE(WriteBin(source, DATA_DIR, fileName));
    StarsQosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V6_1_0)));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V6_1_0)));
    const std::vector<QosBwRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(1300U, std::get<0>(rows[0]));
    EXPECT_EQ(1, std::get<1>(rows[0]));
    EXPECT_EQ(200U, std::get<2>(rows[0]));
    EXPECT_EQ(209U, std::get<11>(rows[0]));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({DATA_DIR, fileName}) + ".done"));
}

TEST_F(QosParserUTest, ShouldIgnoreV6StarsRecordWithInvalidMagicOrFunctionType)
{
    std::vector<uint8_t> invalidMagic = CreateStarsQosRecord(1, 100, 10);
    std::vector<uint8_t> invalidFunc = CreateStarsQosRecord(2, 200, 20);
    invalidMagic[2] = 0;
    invalidFunc[0] = static_cast<uint8_t>((invalidFunc[0] & 0xc0U) | 0x01U);
    invalidMagic.insert(invalidMagic.end(), invalidFunc.begin(), invalidFunc.end());
    ASSERT_TRUE(WriteBin(invalidMagic, DATA_DIR, "stars_soc_profile.data.0.slice_0"));
    StarsQosParser parser;
    QosPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V6_2_0)));
    EXPECT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V6_2_0)));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "qos.db"})));
}
}  // namespace
}  // namespace Domain
}  // namespace Analysis
