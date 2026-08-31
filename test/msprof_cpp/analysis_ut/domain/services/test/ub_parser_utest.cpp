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
#include "analysis/csrc/domain/services/parser/ub/include/ub_parser.h"
#include "analysis/csrc/domain/services/persistence/device/ub_persistence.h"
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
const std::string ROOT_DIR = "./ub_parser_utest";
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

std::vector<uint8_t> CreateUbRecord(uint16_t portId, uint64_t timestamp, uint64_t baseValue)
{
    std::vector<uint8_t> record(128, 0);
    WriteLittleEndian<uint16_t>(record, 4, portId);
    WriteLittleEndian<uint64_t>(record, 8, timestamp);
    for (uint32_t index = 0; index < 14; ++index)
    {
        WriteLittleEndian<uint64_t>(record, 16 + index * sizeof(uint64_t), baseValue + index + 1);
    }
    return record;
}

template <size_t... Index>
std::vector<uint64_t> GetUbMetricValuesImpl(const UbBwRow &row, Infra::IndexSequence<Index...>)
{
    return {static_cast<uint64_t>(std::get<Index + 3>(row))...};
}

std::vector<uint64_t> GetUbMetricValues(const UbBwRow &row)
{
    return GetUbMetricValuesImpl(row, Infra::MakeIndexSequence<14>{});
}

class UbParserUTest : public testing::Test
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
        context.deviceContextInfo.deviceInfo.deviceId = 3;
        return context;
    }

    std::vector<UbBwRow> QueryRows() const
    {
        std::shared_ptr<Infra::DBRunner> runner;
        MAKE_SHARED_RETURN_VALUE(runner, Infra::DBRunner, {}, Utils::File::PathJoin({SQLITE_DIR, "ub.db"}));
        std::vector<UbBwRow> rows;
        EXPECT_TRUE(runner->QueryData("SELECT * FROM UBBwData", rows));
        return rows;
    }
};

TEST_F(UbParserUTest, ShouldParseV6UbWithAllColumns)
{
    const std::string fileName = "ub.data.0.slice_0";
    std::vector<uint8_t> record = CreateUbRecord(7, 500, 1000);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, fileName));
    UbParser parser;
    UbPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V6_2_0)));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "ub.db"})));
    const auto parsedData = inventory.GetPtr<std::vector<HalUbBwData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(3U, parsedData->at(0).deviceId);
    EXPECT_EQ(7U, parsedData->at(0).portId);
    EXPECT_EQ(500U, parsedData->at(0).timestamp);
    EXPECT_EQ(1001U, parsedData->at(0).metrics.front());
    EXPECT_EQ(1014U, parsedData->at(0).metrics.back());
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V6_2_0)));
    const std::vector<UbBwRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(3U, std::get<0>(rows[0]));
    EXPECT_EQ(7U, std::get<1>(rows[0]));
    EXPECT_EQ(500U, std::get<2>(rows[0]));
    const std::vector<uint64_t> expectedMetrics{1001, 1002, 1003, 1004, 1005, 1006, 1007,
                                                1008, 1009, 1010, 1011, 1012, 1013, 1014};
    EXPECT_EQ(expectedMetrics, GetUbMetricValues(rows[0]));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({DATA_DIR, fileName}) + ".done"));

    Infra::DBRunner runner(Utils::File::PathJoin({SQLITE_DIR, "ub.db"}));
    EXPECT_EQ(17UL, runner.GetTableColumns("UBBwData").size());
}

TEST_F(UbParserUTest, ShouldParseContinuousUbRecordAcrossSlices)
{
    const std::vector<uint8_t> record = CreateUbRecord(7, 500, 1000);
    std::vector<uint8_t> first(record.begin(), record.begin() + 32);
    std::vector<uint8_t> second(record.begin() + 32, record.end());
    ASSERT_TRUE(WriteBin(second, DATA_DIR, "ub.data.0.slice_9"));
    ASSERT_TRUE(WriteBin(first, DATA_DIR, "ub.data.0.slice_1"));
    UbParser parser;
    UbPersistence persistence;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V6_1_0)));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext(CHIP_V6_1_0)));
    const std::vector<UbBwRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(7U, std::get<1>(rows[0]));
    EXPECT_EQ(500U, std::get<2>(rows[0]));
    EXPECT_EQ(1014U, std::get<16>(rows[0]));
}

TEST_F(UbParserUTest, ShouldReturnErrorWhenUbPersistenceFails)
{
    std::vector<uint8_t> record = CreateUbRecord(7, 500, 1000);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "ub.data.0.slice_0"));
    UbParser parser;
    UbPersistence persistence;
    Infra::DataInventory inventory;

    MOCKER_CPP(&Infra::DBRunner::CreateTable).stubs().will(returnValue(false));
    EXPECT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext(CHIP_V6_1_0)));
    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, CreateContext(CHIP_V6_1_0)));
    MOCKER_CPP(&Infra::DBRunner::CreateTable).reset();
}

TEST_F(UbParserUTest, ShouldReturnErrorWhenUbPersistenceHasNoInputData)
{
    UbPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_ERROR, persistence.Run(inventory, CreateContext(CHIP_V6_1_0)));
}
}  // namespace
}  // namespace Domain
}  // namespace Analysis
