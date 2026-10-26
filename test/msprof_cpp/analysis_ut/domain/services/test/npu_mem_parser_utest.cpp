/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_mem.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/npu_mem/include/npu_mem_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/npu_mem_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/persistence/device/npu_mem_persistence.h"
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
template <typename ValueType>
void WriteLittleEndian(std::vector<uint8_t> &buffer, size_t offset, ValueType value)
{
    for (size_t index = 0; index < sizeof(ValueType); ++index)
    {
        buffer[offset + index] = static_cast<uint8_t>((value >> (index * 8U)) & 0xffU);
    }
}

const std::string ROOT_DIR = "./npu_mem_parser_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});

std::vector<uint8_t> CreateNpuMemRecord(uint32_t v1Timestamp, uint32_t event, uint64_t v2Timestamp, uint64_t ddr,
                                        uint64_t hbm)
{
    std::vector<uint8_t> record(32, 0);
    WriteLittleEndian(record, 0, v1Timestamp);
    WriteLittleEndian(record, 4, event);
    WriteLittleEndian(record, 8, v2Timestamp);
    WriteLittleEndian(record, 16, ddr);
    WriteLittleEndian(record, 24, hbm);
    return record;
}

class NpuMemParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    DeviceContext CreateContext(const std::string &collectionVersion) const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
        context.deviceContextInfo.collectionVersion = collectionVersion;
        context.deviceContextInfo.hostStartLog.clockMonotonicRaw = 100;
        context.deviceContextInfo.startInfo.collectionTimeBegin = 2;
        context.deviceContextInfo.startInfo.clockMonotonicRaw = 50;
        return context;
    }

    std::vector<NpuMemRow> QueryRows() const
    {
        std::shared_ptr<Infra::DBRunner> runner;
        MAKE_SHARED_RETURN_VALUE(runner, Infra::DBRunner, {}, Utils::File::PathJoin({SQLITE_DIR, "npu_mem.db"}));
        std::vector<NpuMemRow> rows;
        EXPECT_TRUE(runner->QueryData("SELECT * FROM NpuMem", rows));
        return rows;
    }
};

TEST_F(NpuMemParserUTest, ShouldDecodeV1Record)
{
    // IIQQQ: timestamp, event, reserved, ddr, hbm.
    std::vector<uint8_t> record(32, 0);
    WriteLittleEndian(record, 0, static_cast<uint32_t>(7));
    WriteLittleEndian(record, 4, static_cast<uint32_t>(1));
    WriteLittleEndian(record, 8, static_cast<uint64_t>(5));
    WriteLittleEndian(record, 16, static_cast<uint64_t>(100));
    WriteLittleEndian(record, 24, static_cast<uint64_t>(200));
    HalNpuMemData decoded;
    ASSERT_EQ(ANALYSIS_OK, NpuMemV1ParseItem(record.data(), record.size(), reinterpret_cast<uint8_t *>(&decoded), 0));
    EXPECT_EQ(1U, decoded.event);
    EXPECT_EQ(7U, decoded.timestamp);
    EXPECT_EQ(100U, decoded.ddr);
    EXPECT_EQ(200U, decoded.hbm);
}

TEST_F(NpuMemParserUTest, ShouldDecodeV2Timestamp)
{
    std::vector<uint8_t> record(32, 0);
    WriteLittleEndian(record, 4, static_cast<uint32_t>(1));
    WriteLittleEndian(record, 8, static_cast<uint64_t>(8));
    WriteLittleEndian(record, 16, static_cast<uint64_t>(100));
    WriteLittleEndian(record, 24, static_cast<uint64_t>(200));
    HalNpuMemData decoded;
    ASSERT_EQ(ANALYSIS_OK, NpuMemV2ParseItem(record.data(), record.size(), reinterpret_cast<uint8_t *>(&decoded), 0));
    EXPECT_EQ(8U, decoded.timestamp);
}

TEST_F(NpuMemParserUTest, ShouldSelectV1ParserAndConvertTimestamp)
{
    std::vector<uint8_t> record = CreateNpuMemRecord(7, 1, 8, 100, 200);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    Infra::DataInventory inventory;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("1.0")));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(1U, parsedData->at(0).event);
    EXPECT_EQ(9050U, parsedData->at(0).timestamp);
}

TEST_F(NpuMemParserUTest, ShouldSelectV2ParserAndPersistConvertedValues)
{
    std::vector<uint8_t> record = CreateNpuMemRecord(7, 1, 8, 100, 200);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    NpuMemPersistence persistence;
    Infra::DataInventory inventory;
    const DeviceContext context = CreateContext("2.0");

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(10050U, parsedData->at(0).timestamp);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));

    const std::vector<NpuMemRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ("1", std::get<0>(rows[0]));
    EXPECT_EQ(100U, std::get<1>(rows[0]));
    EXPECT_EQ(200U, std::get<2>(rows[0]));
    EXPECT_DOUBLE_EQ(10050.0, std::get<3>(rows[0]));
    EXPECT_EQ(300U, std::get<4>(rows[0]));
}

TEST_F(NpuMemParserUTest, ShouldSkipPersistenceWhenRawDataIsAbsent)
{
    NpuMemPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext("2.0")));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "npu_mem.db"})));
}

TEST_F(NpuMemParserUTest, ShouldApplyHostCntVctCorrectionToTimestamp)
{
    std::vector<uint8_t> record = CreateNpuMemRecord(7, 1, 8, 100, 200);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    Infra::DataInventory inventory;
    DeviceContext context = CreateContext("2.0");
    context.deviceContextInfo.cpuInfo.frequency = 1000;
    context.deviceContextInfo.hostStartLog.cntVctDiff = 5;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(10055U, parsedData->at(0).timestamp);
}

TEST_F(NpuMemParserUTest, ShouldFailWhenAllRecordsHaveIncompleteTimeContext)
{
    std::vector<uint8_t> record = CreateNpuMemRecord(7, 1, 8, 100, 200);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    Infra::DataInventory inventory;
    DeviceContext context = CreateContext("2.0");
    context.deviceContextInfo.startInfo.clockMonotonicRaw = 0;

    ASSERT_EQ(PARSER_PARSE_DATA_ERROR, parser.Run(inventory, context));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    EXPECT_EQ(nullptr, parsedData);
}

TEST_F(NpuMemParserUTest, ShouldFailWhenAllTimestampConversionsOverflow)
{
    const uint64_t overflowingTimestamp = std::numeric_limits<uint64_t>::max() / 1000U + 1U;
    std::vector<uint8_t> record = CreateNpuMemRecord(0, 1, overflowingTimestamp, 100, 200);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    Infra::DataInventory inventory;

    ASSERT_EQ(PARSER_PARSE_DATA_ERROR, parser.Run(inventory, CreateContext("2.0")));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    EXPECT_EQ(nullptr, parsedData);
}
TEST_F(NpuMemParserUTest, ShouldKeepValidRecordsWhenSomeTimestampConversionsFail)
{
    auto records = CreateNpuMemRecord(7, 1, 8, 100, 200);
    const auto invalid = CreateNpuMemRecord(0, 1, std::numeric_limits<uint64_t>::max() / 1000U + 1U, 100, 200);
    records.insert(records.end(), invalid.begin(), invalid.end());
    ASSERT_TRUE(WriteBin(records, DATA_DIR, "npu_mem.data.0.slice_0"));
    NpuMemParser parser;
    Infra::DataInventory inventory;
    auto context = CreateContext("2.0");
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ(10050U, parsedData->at(0).timestamp);
}

TEST_F(NpuMemParserUTest, ShouldSucceedWhenInputIsAbsent)
{
    NpuMemParser parser;
    Infra::DataInventory inventory;
    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, CreateContext("2.0")));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuMemData>>();
    ASSERT_NE(nullptr, parsedData);
    EXPECT_TRUE(parsedData->empty());
}

}  // namespace
}  // namespace Domain
}  // namespace Analysis
