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
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/mc2_comm_info_dumper.h"
#include "analysis/csrc/infrastructure/dfx/log.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Host::Cann;
using namespace mockcpp;

namespace {
std::string capturedLogMessage;

std::shared_ptr<ParserCompactInfo> MakeCaptureInfo(uint16_t deviceId, uint32_t modelId, uint32_t originalStreamId,
                                                   uint32_t streamId, uint16_t captureStatus, uint64_t timeStamp)
{
    auto info = std::make_shared<ParserCompactInfo>();
    info->timeStamp = timeStamp;
    info->data.captureStreamInfo.deviceId = deviceId;
    info->data.captureStreamInfo.modelId = modelId;
    info->data.captureStreamInfo.originalStreamId = originalStreamId;
    info->data.captureStreamInfo.streamId = streamId;
    info->data.captureStreamInfo.captureStatus = captureStatus;
    return info;
}

std::shared_ptr<ParserAdditionalInfo> MakeMc2Info(uint64_t groupName, uint32_t rankSize, uint32_t rankId,
                                                  uint32_t usrRankId, uint32_t streamId, uint32_t streamSize)
{
    auto info = std::make_shared<ParserAdditionalInfo>();
    auto payload = Analysis::Utils::ReinterpretConvert<MsprofMc2CommInfo *>(info->data);
    payload->groupName = groupName;
    payload->rankSize = rankSize;
    payload->rankId = rankId;
    payload->usrRankId = usrRankId;
    payload->streamId = streamId;
    payload->streamSize = streamSize;
    for (uint32_t i = 0; i < MC2_COMM_STREAM_MAX_NUM; ++i) {
        payload->commStreamIds[i] = 100 + i;
    }
    return info;
}

void CaptureLogMessage(Analysis::Log *, const std::string &message, const std::string &, const std::string &,
                       const uint32_t &)
{
    capturedLogMessage = message;
}
}  // namespace

TEST(CaptureStreamInfoDumperUTest, ShouldMatchPythonGlobalModelEndDeduplication)
{
    std::vector<std::shared_ptr<ParserCompactInfo>> input{
        MakeCaptureInfo(0, 1, 10, 20, 0, 20),
        MakeCaptureInfo(0, 1, 10, 20, 0, 10),
        MakeCaptureInfo(0, 1, 10, 20, 0, 10),
        MakeCaptureInfo(0, 1, UINT32_MAX, UINT32_MAX, 1, 30),
        MakeCaptureInfo(1, 1, UINT32_MAX, UINT32_MAX, 1, 40),
    };

    CaptureStreamInfoDumper dumper(".");
    auto output = dumper.FormatData(input);
    ASSERT_EQ(3ul, output.size());

    EXPECT_EQ(0, output[0].deviceId);
    EXPECT_EQ(1u, output[0].modelId);
    EXPECT_EQ(10u, output[0].originalStreamId);
    EXPECT_EQ(20u, output[0].streamId);
    EXPECT_EQ(0u, output[0].batchId);
    EXPECT_EQ(0, output[0].captureStatus);
    EXPECT_EQ(10ul, output[0].timeStamp);

    EXPECT_EQ(1u, output[1].batchId);
    EXPECT_EQ(20ul, output[1].timeStamp);
    EXPECT_EQ(1, output[2].captureStatus);
    EXPECT_EQ(30ul, output[2].timeStamp);
}

TEST(CaptureStreamInfoDumperUTest, ShouldKeepSeparateBatchCountersForEachDeviceAndStream)
{
    std::vector<std::shared_ptr<ParserCompactInfo>> input{
        MakeCaptureInfo(0, 1, 10, 20, 0, 10),
        MakeCaptureInfo(1, 1, 10, 20, 0, 11),
        MakeCaptureInfo(0, 1, 11, 21, 0, 12),
        MakeCaptureInfo(0, 1, 10, 20, 0, 13),
    };

    CaptureStreamInfoDumper dumper(".");
    auto output = dumper.FormatData(input);
    ASSERT_EQ(4ul, output.size());
    EXPECT_EQ(0u, output[0].batchId);
    EXPECT_EQ(0u, output[1].batchId);
    EXPECT_EQ(0u, output[2].batchId);
    EXPECT_EQ(1u, output[3].batchId);
}

TEST(CaptureStreamInfoDumperUTest, ShouldLogMismatchedStartAndEndModelIds)
{
    const std::string expectedMessage =
        "CaptureStreamInfoDumper: Capture start model ids are {7}, end model ids are {}.";
    capturedLogMessage.clear();
    MOCKER_CPP(&Analysis::Log::LogMsg).stubs().will(invoke(CaptureLogMessage));
    std::vector<std::shared_ptr<ParserCompactInfo>> input{MakeCaptureInfo(0, 7, 10, 20, 0, 10)};

    CaptureStreamInfoDumper dumper(".");
    auto output = dumper.FormatData(input);

    ASSERT_EQ(1ul, output.size());
    EXPECT_EQ(expectedMessage, capturedLogMessage);
    MOCKER_CPP(&Analysis::Log::LogMsg).reset();
}

TEST(Mc2CommInfoDumperUTest, ShouldKeepRawRowsAndAppendOneToManyCaptureMappedRowsInDeterministicOrder)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> mc2Data{
        MakeMc2Info(5862276093215481612ULL, 2, 0, 0, 20, 2),
        MakeMc2Info(9, 4, 1, 1, 99, 1),
    };
    CaptureStreamInfoData captureInput{
        {0, 1, 20, 32, 0, 0, 1},
        {0, 1, 20, 21, 0, 0, 2},
        {0, 1, 20, 21, 1, 0, 3},
        {0, 1, 20, 22, 0, 0, 4},
    };

    Mc2CommInfoDumper dumper(".");
    Mc2CommInfoInput input(mc2Data, captureInput);
    auto output = dumper.GenerateData(input);
    ASSERT_EQ(5ul, output.size());
    EXPECT_EQ("5862276093215481612", std::get<0>(output[0]));
    EXPECT_EQ(20u, std::get<4>(output[0]));
    EXPECT_EQ("9", std::get<0>(output[1]));
    EXPECT_EQ(99u, std::get<4>(output[1]));

    EXPECT_EQ(21u, std::get<4>(output[2]));
    EXPECT_EQ(22u, std::get<4>(output[3]));
    EXPECT_EQ(32u, std::get<4>(output[4]));
    EXPECT_EQ("100,101", std::get<5>(output[4]));
}

TEST(Mc2CommInfoDumperUTest, ShouldNotAppendCaptureRowsWhenKfcStreamDoesNotMatch)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> mc2Data{MakeMc2Info(1, 2, 0, 0, 99, 1)};
    CaptureStreamInfoData captureInput{{0, 1, 20, 32, 0, 0, 1}};

    Mc2CommInfoDumper dumper(".");
    Mc2CommInfoInput input(mc2Data, captureInput);
    auto output = dumper.GenerateData(input);
    ASSERT_EQ(1ul, output.size());
    EXPECT_EQ(99u, std::get<4>(output[0]));
}

TEST(Mc2CommInfoDumperUTest, ShouldLogInvalidStreamSizeCountOnce)
{
    const std::string expectedMessage = "Mc2CommInfoDumper: 2 records have stream size greater than max stream size 8.";
    capturedLogMessage.clear();
    MOCKER_CPP(&Analysis::Log::LogMsg).stubs().will(invoke(CaptureLogMessage));
    std::vector<std::shared_ptr<ParserAdditionalInfo>> mc2Data{
        MakeMc2Info(1, 2, 0, 0, 10, 9),
        MakeMc2Info(2, 2, 0, 0, 11, 10),
    };
    CaptureStreamInfoData captureData;

    Mc2CommInfoDumper dumper(".");
    Mc2CommInfoInput input(mc2Data, captureData);
    auto output = dumper.GenerateData(input);

    ASSERT_EQ(2ul, output.size());
    EXPECT_TRUE(std::get<5>(output[0]).empty());
    EXPECT_EQ(expectedMessage, capturedLogMessage);
    MOCKER_CPP(&Analysis::Log::LogMsg).reset();
}

TEST(Mc2CommInfoDumperUTest, ShouldMatchPythonWhenInvalidStreamSizeHasCaptureMapping)
{
    std::vector<std::shared_ptr<ParserAdditionalInfo>> mc2Data{MakeMc2Info(1, 2, 0, 0, 20, 9)};
    CaptureStreamInfoData captureData{{0, 1, 20, 32, 0, 0, 1}};

    Mc2CommInfoDumper dumper(".");
    Mc2CommInfoInput input(mc2Data, captureData);
    auto output = dumper.GenerateData(input);

    ASSERT_EQ(2ul, output.size());
    EXPECT_EQ(20u, std::get<4>(output[0]));
    EXPECT_EQ(32u, std::get<4>(output[1]));
    EXPECT_TRUE(std::get<5>(output[0]).empty());
    EXPECT_TRUE(std::get<5>(output[1]).empty());
}
