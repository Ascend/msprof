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

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/summary/hbm_summary_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Application
{
namespace
{
const std::string BASE_PATH = "./hbm_summary_assembler_utest";
const std::string PROF_PATH = Utils::File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = Utils::File::PathJoin({PROF_PATH, Common::OUTPUT_PATH});

class HbmSummaryAssemblerUTest : public testing::Test
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

TEST_F(HbmSummaryAssemblerUTest, ShouldGeneratePythonCompatibleCsv)
{
    Domain::HbmSummaryData average;
    average.deviceId = 1;
    average.isAverage = true;
    average.readBandwidth = 40.1364;
    average.writeBandwidth = 83.4186;
    Domain::HbmSummaryData hbm0;
    hbm0.deviceId = 1;
    hbm0.hbmId = 0;
    hbm0.readBandwidth = 44.5701;
    hbm0.writeBandwidth = 86.3474;
    Domain::HbmSummaryData hbm255;
    hbm255.deviceId = 1;
    hbm255.hbmId = UINT8_MAX;
    hbm255.readBandwidth = 1.25;
    hbm255.writeBandwidth = 2.5;
    std::vector<Domain::HbmSummaryData> source{average, hbm0, hbm255};
    std::shared_ptr<std::vector<Domain::HbmSummaryData>> data;
    MAKE_SHARED_NO_OPERATION(data, std::vector<Domain::HbmSummaryData>, source);
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(data));
    HbmSummaryAssembler assembler(PROCESSOR_NAME_HBM, PROF_PATH);

    ASSERT_TRUE(assembler.Run(inventory));
    const auto files = Utils::File::GetOriginData(RESULT_PATH, {HBM_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(4UL, lines.size());
    EXPECT_EQ("Device_id,Metric,Read(MB/s),Write(MB/s)", lines[0]);
    EXPECT_EQ("1,Average,40.136,83.419", lines[1]);
    EXPECT_EQ("1,0,44.57,86.347", lines[2]);
    EXPECT_EQ("1,255,1.25,2.5", lines[3]);
}

TEST_F(HbmSummaryAssemblerUTest, ShouldSkipMissingDataAndFailOnEmptyData)
{
    HbmSummaryAssembler assembler(PROCESSOR_NAME_HBM, PROF_PATH);
    Infra::DataInventory inventory;
    EXPECT_TRUE(assembler.Run(inventory));

    std::shared_ptr<std::vector<Domain::HbmSummaryData>> empty;
    MAKE_SHARED0_NO_OPERATION(empty, std::vector<Domain::HbmSummaryData>);
    ASSERT_TRUE(inventory.Inject(empty));
    EXPECT_FALSE(assembler.Run(inventory));
}

TEST_F(HbmSummaryAssemblerUTest, ShouldExportMissingDirectionAsZero)
{
    Domain::HbmSummaryData readOnly;
    readOnly.deviceId = 1;
    readOnly.hbmId = 0;
    readOnly.readBandwidth = 88;
    Domain::HbmSummaryData writeOnly;
    writeOnly.deviceId = 1;
    writeOnly.hbmId = 1;
    writeOnly.writeBandwidth = 12;
    Domain::HbmSummaryData measuredZero;
    measuredZero.deviceId = 1;
    measuredZero.hbmId = 2;
    measuredZero.readBandwidth = 0;
    measuredZero.writeBandwidth = 0;
    auto data = std::make_shared<std::vector<Domain::HbmSummaryData>>(
        std::vector<Domain::HbmSummaryData>{readOnly, writeOnly, measuredZero});
    Infra::DataInventory inventory;
    ASSERT_TRUE(inventory.Inject(data));
    HbmSummaryAssembler assembler(PROCESSOR_NAME_HBM, PROF_PATH);
    ASSERT_TRUE(assembler.Run(inventory));

    const auto files = Utils::File::GetOriginData(RESULT_PATH, {HBM_SUMMARY_NAME}, {});
    ASSERT_EQ(1UL, files.size());
    Utils::FileReader reader(files[0]);
    std::vector<std::string> lines;
    ASSERT_EQ(ANALYSIS_OK, reader.ReadText(lines));
    ASSERT_EQ(4UL, lines.size());
    EXPECT_EQ("1,0,88,0", lines[1]);
    EXPECT_EQ("1,1,0,12", lines[2]);
    EXPECT_EQ("1,2,0,0", lines[3]);
}
}  // namespace
}  // namespace Application
}  // namespace Analysis
