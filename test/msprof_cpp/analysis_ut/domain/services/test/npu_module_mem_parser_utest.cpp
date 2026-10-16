/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_module_mem.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/npu_module_mem/include/npu_module_mem_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/npu_module_mem_parser_item.h"
#include "analysis/csrc/domain/services/persistence/device/npu_module_mem_persistence.h"
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

const std::string ROOT_DIR = "./npu_module_mem_parser_utest";
const std::string DATA_DIR = Utils::File::PathJoin({ROOT_DIR, "data"});
const std::string SQLITE_DIR = Utils::File::PathJoin({ROOT_DIR, "sqlite"});

std::vector<uint8_t> CreateNpuModuleMemRecord(uint32_t moduleId, uint64_t syscnt, uint64_t totalSize)
{
    std::vector<uint8_t> record(24, 0);
    WriteLittleEndian(record, 0, moduleId);
    WriteLittleEndian(record, 8, syscnt);
    WriteLittleEndian(record, 16, totalSize);
    return record;
}

class NpuModuleMemParserUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(DATA_DIR));
        ASSERT_TRUE(Utils::File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(ROOT_DIR, 0)); }

    DeviceContext CreateContext() const
    {
        DeviceContext context;
        context.deviceContextInfo.deviceFilePath = ROOT_DIR;
        context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
        context.deviceContextInfo.deviceInfo.deviceId = 3;
        return context;
    }

    std::vector<NpuModuleMemRow> QueryRows() const
    {
        std::shared_ptr<Infra::DBRunner> runner;
        MAKE_SHARED_RETURN_VALUE(runner, Infra::DBRunner, {},
                                 Utils::File::PathJoin({SQLITE_DIR, "npu_module_mem.db"}));
        std::vector<NpuModuleMemRow> rows;
        EXPECT_TRUE(runner->QueryData("SELECT * FROM NpuModuleMem", rows));
        return rows;
    }
};

TEST_F(NpuModuleMemParserUTest, ShouldRejectUnexpectedRecordSize)
{
    std::vector<uint8_t> record(23, 0);
    uint8_t output[32] = {};
    EXPECT_NE(ANALYSIS_OK, NpuModuleMemParseItem(record.data(), record.size(), output, 0));
}

TEST_F(NpuModuleMemParserUTest, ShouldDecodePythonCompatibleLayout)
{
    std::vector<uint8_t> record(24, 0);
    WriteLittleEndian(record, 0, static_cast<uint32_t>(9));
    WriteLittleEndian(record, 4, static_cast<uint32_t>(123));
    WriteLittleEndian(record, 8, static_cast<uint64_t>(456));
    WriteLittleEndian(record, 16, static_cast<uint64_t>(789));
    HalNpuModuleMemData decoded;
    ASSERT_EQ(ANALYSIS_OK,
              NpuModuleMemParseItem(record.data(), record.size(), reinterpret_cast<uint8_t *>(&decoded), 0));
    EXPECT_EQ(9U, decoded.moduleId);
    EXPECT_EQ(456U, decoded.syscnt);
    EXPECT_EQ(789U, decoded.totalSize);
}

TEST_F(NpuModuleMemParserUTest, ShouldParseAndPersistModuleMemory)
{
    std::vector<uint8_t> record = CreateNpuModuleMemRecord(9, 456, 789);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_module_mem.data.0.slice_0"));
    NpuModuleMemParser parser;
    NpuModuleMemPersistence persistence;
    Infra::DataInventory inventory;
    const DeviceContext context = CreateContext();

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    const auto parsedData = inventory.GetPtr<std::vector<HalNpuModuleMemData>>();
    ASSERT_NE(nullptr, parsedData);
    ASSERT_EQ(1UL, parsedData->size());
    EXPECT_EQ("NPU:3", parsedData->at(0).deviceType);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));

    const std::vector<NpuModuleMemRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(9U, std::get<0>(rows[0]));
    EXPECT_EQ(456U, std::get<1>(rows[0]));
    EXPECT_EQ(789, std::get<2>(rows[0]));
    EXPECT_EQ("NPU:3", std::get<3>(rows[0]));
}

TEST_F(NpuModuleMemParserUTest, ShouldSkipPersistenceWhenRawDataIsAbsent)
{
    NpuModuleMemPersistence persistence;
    Infra::DataInventory inventory;

    EXPECT_EQ(ANALYSIS_OK, persistence.Run(inventory, CreateContext()));
    EXPECT_FALSE(Utils::File::Exist(Utils::File::PathJoin({SQLITE_DIR, "npu_module_mem.db"})));
}

TEST_F(NpuModuleMemParserUTest, ShouldPersistTooLargeTotalSizeAsMinusOne)
{
    std::vector<uint8_t> record =
        CreateNpuModuleMemRecord(9, 456, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U);
    ASSERT_TRUE(WriteBin(record, DATA_DIR, "npu_module_mem.data.0.slice_0"));
    NpuModuleMemParser parser;
    NpuModuleMemPersistence persistence;
    Infra::DataInventory inventory;
    const DeviceContext context = CreateContext();

    ASSERT_EQ(ANALYSIS_OK, parser.Run(inventory, context));
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(inventory, context));
    const std::vector<NpuModuleMemRow> rows = QueryRows();
    ASSERT_EQ(1UL, rows.size());
    EXPECT_EQ(-1, std::get<2>(rows[0]));
}
}  // namespace
}  // namespace Domain
}  // namespace Analysis
