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

#include "gtest/gtest.h"

#include "analysis/csrc/domain/services/parser/host/cann/addition_info_parser.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;

namespace {
const std::string MC2_ROOT = "./mc2_comm_parser";
const std::string MC2_DATA_PATH = File::PathJoin({MC2_ROOT, "host", "data"});

MsprofAdditionalInfo MakeMc2Info(uint64_t groupName, uint32_t streamSize)
{
    MsprofAdditionalInfo info{};
    auto payload = ReinterpretConvert<MsprofMc2CommInfo *>(info.data);
    payload->groupName = groupName;
    payload->rankSize = 8;
    payload->rankId = 1;
    payload->usrRankId = 2;
    payload->streamId = 52;
    payload->streamSize = streamSize;
    for (uint32_t i = 0; i < MC2_COMM_STREAM_MAX_NUM; ++i) {
        payload->commStreamIds[i] = 100 + i;
    }
    return info;
}
}

class Mc2CommInfoParserUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Exist(MC2_ROOT)) {
            EXPECT_TRUE(File::RemoveDir(MC2_ROOT, 0));
        }
        EXPECT_TRUE(File::CreateDir(MC2_ROOT));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({MC2_ROOT, "host"})));
        EXPECT_TRUE(File::CreateDir(MC2_DATA_PATH));
    }

    void TearDown() override
    {
        if (File::Exist(MC2_ROOT)) {
            EXPECT_TRUE(File::RemoveDir(MC2_ROOT, 0));
        }
    }
};

TEST_F(Mc2CommInfoParserUTest, ShouldNotLookupMc2CommInfoSlice)
{
    std::vector<MsprofAdditionalInfo> input{MakeMc2Info(7466789422691968299ULL, 8)};
    ASSERT_TRUE(WriteBin(input, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldNotLookupAgingMc2CommInfoSlice)
{
    std::vector<MsprofAdditionalInfo> input{MakeMc2Info(1, 9)};
    ASSERT_TRUE(WriteBin(input, MC2_DATA_PATH, "aging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldNotLookupMc2SlicesInAnyOrder)
{
    std::vector<MsprofAdditionalInfo> unaging0{MakeMc2Info(10, 0)};
    std::vector<MsprofAdditionalInfo> unaging1{MakeMc2Info(11, 1)};
    std::vector<MsprofAdditionalInfo> aging{MakeMc2Info(20, 1)};
    ASSERT_TRUE(WriteBin(unaging1, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_1"));
    ASSERT_TRUE(WriteBin(unaging0, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_0"));
    ASSERT_TRUE(WriteBin(aging, MC2_DATA_PATH, "aging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldSkipRecordWithInvalidMagic)
{
    auto info = MakeMc2Info(1, 1);
    info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
    std::vector<MsprofAdditionalInfo> input{info};
    ASSERT_TRUE(WriteBin(input, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldNotLookupMixedValidAndInvalidMagicRecords)
{
    auto invalid = MakeMc2Info(1, 1);
    invalid.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
    auto valid = MakeMc2Info(2, 1);
    std::vector<MsprofAdditionalInfo> input{invalid, valid};
    ASSERT_TRUE(WriteBin(input, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldReturnTrueAndEmptyWhenNoFileExists)
{
    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(Mc2CommInfoParserUTest, ShouldReturnTrueAndEmptyWhenTruncatedRecordExists)
{
    std::vector<uint8_t> input(sizeof(MsprofAdditionalInfo) - 1, 0);
    ASSERT_TRUE(WriteBin(input, MC2_DATA_PATH, "unaging.additional.mc2_comm_info.slice_0"));

    Mc2CommInfoParser parser(MC2_DATA_PATH);
    auto output = parser.ParseData<ParserAdditionalInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}
