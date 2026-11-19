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

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/llc_summary_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/application/summary/ub_summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ub_data.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Application
{
namespace
{
const std::string BASE_PATH = "./ub_llc_summary_assembler_utest";
const std::string PROF_PATH = Utils::File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = Utils::File::PathJoin({PROF_PATH, Common::OUTPUT_PATH});

class UbLlcSummaryAssemblerUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        ASSERT_TRUE(Utils::File::CreateDir(BASE_PATH));
        ASSERT_TRUE(Utils::File::CreateDir(PROF_PATH));
        ASSERT_TRUE(Utils::File::CreateDir(RESULT_PATH));
    }

    void TearDown() override { ASSERT_TRUE(Utils::File::RemoveDir(BASE_PATH, 0)); }
};

class TestableUbSummaryAssembler : public UbSummaryAssembler
{
   public:
    using UbSummaryAssembler::UbSummaryAssembler;
    uint8_t Assemble(Infra::DataInventory &inventory) { return AssembleData(inventory); }
};

class TestableLlcSummaryAssembler : public LlcSummaryAssembler
{
   public:
    using LlcSummaryAssembler::LlcSummaryAssembler;
    uint8_t Assemble(Infra::DataInventory &inventory) { return AssembleData(inventory); }
};

TEST_F(UbLlcSummaryAssemblerUTest, ShouldExportUbTimestampAndBandwidthLikePython)
{
    Domain::UbData row{};
    row.deviceId = 0;
    row.portId = 2;
    row.timestamp = 1234567890123456789ULL;
    row.udmaRxBind = 123;
    row.udmaTxBind = 456;
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(std::make_shared<std::vector<Domain::UbData>>(1, row)));
    UbSummaryAssembler assembler(PROCESSOR_NAME_UB, PROF_PATH);

    ASSERT_TRUE(assembler.Run(inventory));
    const auto files = Utils::File::GetOriginData(RESULT_PATH, {UB_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(2UL, lines.size());
    EXPECT_EQ("Device_id,PortId,TimeStamp,UBRxPortBandWidth(MB/s),UBTxPortBandWidth(MB/s)", lines[0]);
    EXPECT_EQ("0,2,1234567890123456.789\t,123,456", lines[1]);
}

TEST_F(UbLlcSummaryAssemblerUTest, ShouldExportUbRowsAcrossCsvBatches)
{
    std::vector<Domain::UbData> rows;
    rows.reserve(CSV_ROW_BATCH + 1);
    for (std::size_t i = 0; i < CSV_ROW_BATCH + 1; ++i)
    {
        Domain::UbData row{};
        row.deviceId = 0;
        row.portId = static_cast<uint16_t>(i);
        row.timestamp = 1234567890123456789ULL;
        row.udmaRxBind = 123;
        row.udmaTxBind = 456;
        rows.push_back(row);
    }
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(std::make_shared<std::vector<Domain::UbData>>(std::move(rows))));
    UbSummaryAssembler assembler(PROCESSOR_NAME_UB, PROF_PATH);

    ASSERT_TRUE(assembler.Run(inventory));
    const auto files = Utils::File::GetOriginData(RESULT_PATH, {UB_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(CSV_ROW_BATCH + 2, lines.size());
    EXPECT_EQ("Device_id,PortId,TimeStamp,UBRxPortBandWidth(MB/s),UBTxPortBandWidth(MB/s)", lines[0]);
    EXPECT_EQ("0,0,1234567890123456.789\t,123,456", lines[1]);
    EXPECT_EQ("0,8191,1234567890123456.789\t,123,456", lines[CSV_ROW_BATCH]);
    EXPECT_EQ("0,8192,1234567890123456.789\t,123,456", lines.back());
}

TEST_F(UbLlcSummaryAssemblerUTest, ShouldExportLlcAverageAndRealIds255And256)
{
    Domain::LLcSummaryData average;
    average.deviceId = 1;
    average.mode = "read";
    average.isAverage = true;
    average.hitRate = 50;
    average.throughput = 60;
    Domain::LLcSummaryData id255;
    id255.deviceId = 1;
    id255.mode = "read";
    id255.llcId = UINT8_MAX;
    id255.hitRate = 12.3456;
    id255.throughput = 100;
    Domain::LLcSummaryData id256 = id255;
    id256.llcId = 256;
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(
        std::make_shared<std::vector<Domain::LLcSummaryData>>(
            std::vector<Domain::LLcSummaryData>{average, id255, id256})));
    LlcSummaryAssembler assembler(PROCESSOR_NAME_LLC, PROF_PATH);

    ASSERT_TRUE(assembler.Run(inventory));
    const auto files = Utils::File::GetOriginData(RESULT_PATH, {LLC_READ_WRITE_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(4UL, lines.size());
    EXPECT_EQ("Device_id,Mode,Task,Hit Rate(%),Throughput(MB/s)", lines[0]);
    EXPECT_EQ("1,read,Average,50,60", lines[1]);
    EXPECT_EQ("1,read,255,12.346,100", lines[2]);
    EXPECT_EQ("1,read,256,12.346,100", lines[3]);
}

TEST_F(UbLlcSummaryAssemblerUTest, ShouldExportLlcRowsAcrossCsvBatches)
{
    std::vector<Domain::LLcSummaryData> rows;
    rows.reserve(CSV_ROW_BATCH + 1);
    for (std::size_t i = 0; i < CSV_ROW_BATCH + 1; ++i)
    {
        Domain::LLcSummaryData row;
        row.deviceId = 1;
        row.mode = "read";
        row.llcId = static_cast<uint32_t>(i);
        row.hitRate = 12.3456;
        row.throughput = 100;
        rows.push_back(row);
    }
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(std::make_shared<std::vector<Domain::LLcSummaryData>>(std::move(rows))));
    LlcSummaryAssembler assembler(PROCESSOR_NAME_LLC, PROF_PATH);

    ASSERT_TRUE(assembler.Run(inventory));
    const auto files = Utils::File::GetOriginData(RESULT_PATH, {LLC_READ_WRITE_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(CSV_ROW_BATCH + 2, lines.size());
    EXPECT_EQ("1,read,0,12.346,100", lines[1]);
    EXPECT_EQ("1,read,8191,12.346,100", lines[CSV_ROW_BATCH]);
    EXPECT_EQ("1,read,8192,12.346,100", lines.back());
}

TEST_F(UbLlcSummaryAssemblerUTest, ShouldSkipMissingUbDataAndFailOnEmptyUbData)
{
    Infra::DataInventory inventory;
    TestableUbSummaryAssembler assembler(PROCESSOR_NAME_UB, PROF_PATH);
    EXPECT_EQ(DATA_NOT_EXIST, assembler.Assemble(inventory));
    EXPECT_TRUE(assembler.Run(inventory));
    EXPECT_TRUE(Utils::File::GetOriginData(RESULT_PATH, {UB_SUMMARY_NAME}, {}).empty());

    ASSERT_TRUE(inventory.Inject(std::make_shared<std::vector<Domain::UbData>>()));
    EXPECT_EQ(ASSEMBLE_FAILED, assembler.Assemble(inventory));
    EXPECT_FALSE(assembler.Run(inventory));
    EXPECT_TRUE(Utils::File::GetOriginData(RESULT_PATH, {UB_SUMMARY_NAME}, {}).empty());
}

TEST_F(UbLlcSummaryAssemblerUTest, ShouldSkipMissingLlcDataAndFailOnEmptyLlcData)
{
    Infra::DataInventory inventory;
    TestableLlcSummaryAssembler assembler(PROCESSOR_NAME_LLC, PROF_PATH);
    EXPECT_EQ(DATA_NOT_EXIST, assembler.Assemble(inventory));
    EXPECT_TRUE(assembler.Run(inventory));
    EXPECT_TRUE(Utils::File::GetOriginData(RESULT_PATH, {LLC_READ_WRITE_SUMMARY_NAME}, {}).empty());

    ASSERT_TRUE(inventory.Inject(std::make_shared<std::vector<Domain::LLcSummaryData>>()));
    EXPECT_EQ(ASSEMBLE_FAILED, assembler.Assemble(inventory));
    EXPECT_FALSE(assembler.Run(inventory));
    EXPECT_TRUE(Utils::File::GetOriginData(RESULT_PATH, {LLC_READ_WRITE_SUMMARY_NAME}, {}).empty());
}
}  // namespace
}  // namespace Application
}  // namespace Analysis
