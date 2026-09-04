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

#include <fstream>

#include "analysis/csrc/domain/services/parser/host/cann/compact_info_parser.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Utils;

namespace {
const std::string CAPTURE_ROOT = "./capture_stream_parser";
const std::string CAPTURE_DATA_PATH = File::PathJoin({CAPTURE_ROOT, "host", "data"});
}

class CaptureStreamInfoParserUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Exist(CAPTURE_ROOT)) {
            EXPECT_TRUE(File::RemoveDir(CAPTURE_ROOT, 0));
        }
        EXPECT_TRUE(File::CreateDir(CAPTURE_ROOT));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CAPTURE_ROOT, "host"})));
        EXPECT_TRUE(File::CreateDir(CAPTURE_DATA_PATH));
    }

    void TearDown() override
    {
        if (File::Exist(CAPTURE_ROOT)) {
            EXPECT_TRUE(File::RemoveDir(CAPTURE_ROOT, 0));
        }
    }
};

TEST_F(CaptureStreamInfoParserUTest, ShouldNotLookupV1CaptureSlice)
{
    MsprofCompactInfo info{};
    info.timeStamp = 101;
    info.data.captureStreamInfo.deviceId = 2;
    info.data.captureStreamInfo.modelId = 3;
    info.data.captureStreamInfo.originalStreamId = 4;
    info.data.captureStreamInfo.modelStreamId = 5;
    info.data.captureStreamInfo.captureStatus = 0;
    std::vector<MsprofCompactInfo> input{info};
    ASSERT_TRUE(WriteBin(input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldNotLookupCaptureSlicesWhenBothVersionsExist)
{
    MsprofCompactInfo v1{};
    v1.timeStamp = 100;
    v1.data.captureStreamInfo.modelId = 11;
    std::vector<MsprofCompactInfo> v1Input{v1};
    ASSERT_TRUE(WriteBin(v1Input, CAPTURE_DATA_PATH, "aging.compact.capture_stream_info.slice_0"));

    MsprofCompactInfo v2{};
    v2.timeStamp = 200;
    v2.data.captureStreamInfo.modelId = 22;
    std::vector<MsprofCompactInfo> v2Input{v2};
    ASSERT_TRUE(WriteBin(v2Input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info_v2.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldNotLookupV2CaptureSlice)
{
    MsprofCompactInfo info{};
    info.timeStamp = 202;
    info.data.captureStreamInfoV2.deviceId = 6;
    info.data.captureStreamInfoV2.modelId = 70001;
    info.data.captureStreamInfoV2.originalStreamId = 70002;
    info.data.captureStreamInfoV2.streamId = 70003;
    info.data.captureStreamInfoV2.captureStatus = 1;
    std::vector<MsprofCompactInfo> input{info};
    ASSERT_TRUE(WriteBin(input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info_v2.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldNotLookupV2WhenV1MarkerExists)
{
    std::ofstream marker(File::PathJoin({CAPTURE_DATA_PATH,
                                        "unaging.compact.capture_stream_info.slice_0.done"}));
    ASSERT_TRUE(marker.good());
    marker.close();

    MsprofCompactInfo info{};
    info.data.captureStreamInfoV2.modelId = 70001;
    std::vector<MsprofCompactInfo> input{info};
    ASSERT_TRUE(WriteBin(input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info_v2.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    ASSERT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldSkipRecordWithInvalidMagic)
{
    MsprofCompactInfo info{};
    info.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM + 1;
    std::vector<MsprofCompactInfo> input{info};
    ASSERT_TRUE(WriteBin(input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldReturnTrueAndEmptyWhenNoFileExists)
{
    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}

TEST_F(CaptureStreamInfoParserUTest, ShouldReturnTrueAndEmptyWhenTruncatedRecordExists)
{
    std::vector<uint8_t> input(sizeof(MsprofCompactInfo) - 1, 0);
    ASSERT_TRUE(WriteBin(input, CAPTURE_DATA_PATH, "unaging.compact.capture_stream_info.slice_0"));

    CaptureStreamInfoParser parser(CAPTURE_DATA_PATH);
    auto output = parser.ParseData<ParserCompactInfo>();
    EXPECT_TRUE((parser.GetStatus() != Analysis::Domain::ParserStatus::ERROR));
    EXPECT_TRUE(output.empty());
}
