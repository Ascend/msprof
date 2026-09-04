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

#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/hash_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/mc2_comm_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/type_info_db_dumper.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/parser/host/cann/capture_mc2_cpp_enable.h"
#include "test/msprof_cpp/analysis_ut/domain/data_process/test/reserve_mock_utils.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"

using namespace Analysis::Domain;

const std::string TEST_HOST_FILE_PATH = ".";
using HashData = Analysis::Domain::Host::Cann::HashData;
using TypeData = Analysis::Domain::Host::Cann::TypeData;
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using DBRunner = Analysis::Infra::DBRunner;
using namespace Analysis::Utils;

class KernelParserWorkerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"}))) {
            File::RemoveDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"}), 0);
        }
        File::CreateDir(TEST_HOST_FILE_PATH);
        File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "data"}));
        File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "data", "sqlite"}));
        TypeInfoData typeInfoData{{1, {{1, "123"}}}};
        std::unordered_map<uint64_t, std::string> hashData{{0, "a"},
                                                           {1, "b"}};
        MOCKER_CPP(&HashData::GetAll).stubs().will(returnValue(hashData));
        MOCKER_CPP(&TypeData::GetAll).stubs().will(returnValue(typeInfoData));
        MOCKER_CPP(&HostTraceWorker::Run).stubs().will(returnValue(true));
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(File::PathJoin({TEST_HOST_FILE_PATH, "data"}), 0);
        if (File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"}))) {
            File::RemoveDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"}), 0);
        }
    }

    void StubContextLoadSuccess()
    {
        MOCKER_CPP(&Analysis::Domain::Environment::Context::Load).stubs().will(returnValue(true));
    }

    void StubPlatformVersion(Analysis::Domain::Environment::Chip chip)
    {
        StubContextLoadSuccess();
        MOCKER_CPP(&Analysis::Domain::Environment::Context::GetPlatformVersion)
            .stubs()
            .will(returnValue(static_cast<uint16_t>(chip)));
    }

    void UseRealHostTraceWorker()
    {
        MOCKER_CPP(&HostTraceWorker::Run).reset();
    }
};

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnSuccessWhenAllFunctionWell)
{
    StubContextLoadSuccess();
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 0);
}
TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenHostTraceWorkerReturnFailed)
{
    StubContextLoadSuccess();
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&HostTraceWorker::Run).reset();
    MOCKER_CPP(&HostTraceWorker::Run).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenDumpHashFailed)
{
    StubContextLoadSuccess();
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnFailedWhenTypeInfoDumpFailed)
{
    StubContextLoadSuccess();
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, TestKernelParserWorkerShouldReturnErrorWhenContextLoadFailed)
{
    KernelParserWorker kernelParserWorker(TEST_HOST_FILE_PATH);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::Load).stubs().will(returnValue(false));
    auto res = kernelParserWorker.Run();
    EXPECT_EQ(res, 1);
}

TEST_F(KernelParserWorkerUtest, ShouldSkipCaptureParseWhenCppParserDisabledEvenIfTruncatedRecordExists)
{
    // C++ Capture/MC2 默认关闭：截断 bin 不会进入 Host C++ 解析，Host 其它输出仍成功。
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    std::vector<uint8_t> truncatedInput{0};
    ASSERT_TRUE(WriteBin(truncatedInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.compact.capture_stream_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_1_0);

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    EXPECT_EQ(ANALYSIS_OK, worker.Run());
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "stream_info.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldSkipMc2ParseWhenCppParserDisabledEvenIfTruncatedRecordExists)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    std::vector<uint8_t> truncatedInput{0};
    ASSERT_TRUE(WriteBin(truncatedInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.additional.mc2_comm_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_3_0);

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    EXPECT_EQ(ANALYSIS_OK, worker.Run());
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "mc2_comm_info.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldNotInvokeCaptureDumperWhenCppParserDisabled)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofCompactInfo capture{};
    capture.timeStamp = 123;
    capture.data.captureStreamInfo.deviceId = 0;
    capture.data.captureStreamInfo.modelId = 7;
    capture.data.captureStreamInfo.originalStreamId = 52;
    capture.data.captureStreamInfo.modelStreamId = 70;
    capture.data.captureStreamInfo.captureStatus = 0;
    std::vector<MsprofCompactInfo> captureInput{capture};
    ASSERT_TRUE(WriteBin(captureInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.compact.capture_stream_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_1_0);
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    // 即使预置与 dumper 冲突的路径，默认关闭时也不会走 Capture dump。
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "stream_info.db"})));

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    EXPECT_EQ(ANALYSIS_OK, worker.Run());
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldNotInvokeMc2DumperWhenCppParserDisabled)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofAdditionalInfo mc2{};
    auto payload = ReinterpretConvert<Analysis::Domain::Host::Cann::MsprofMc2CommInfo *>(mc2.data);
    payload->groupName = 99;
    payload->rankSize = 2;
    payload->streamId = 52;
    payload->streamSize = 1;
    payload->commStreamIds[0] = 100;
    std::vector<MsprofAdditionalInfo> mc2Input{mc2};
    ASSERT_TRUE(WriteBin(mc2Input, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.additional.mc2_comm_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_3_0);
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "mc2_comm_info.db"})));

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    EXPECT_EQ(ANALYSIS_OK, worker.Run());
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldSkipCaptureFormattingWhenCppParserDisabled)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofCompactInfo capture{};
    capture.timeStamp = 123;
    capture.data.captureStreamInfo.modelId = 7;
    capture.data.captureStreamInfo.originalStreamId = 52;
    capture.data.captureStreamInfo.modelStreamId = 70;
    std::vector<MsprofCompactInfo> captureInput{capture};
    ASSERT_TRUE(WriteBin(captureInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.compact.capture_stream_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_1_0);
    Analysis::Test::StubReserveFailureForVector<CaptureStreamInfoData>();

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    auto result = worker.Run();
    Analysis::Test::ResetReserveFailureForVector<CaptureStreamInfoData>();
    EXPECT_EQ(ANALYSIS_OK, result);
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldSkipMc2FormattingWhenCppParserDisabled)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofAdditionalInfo mc2{};
    auto payload = ReinterpretConvert<Analysis::Domain::Host::Cann::MsprofMc2CommInfo *>(mc2.data);
    payload->groupName = 99;
    payload->rankSize = 2;
    payload->streamId = 52;
    payload->streamSize = 1;
    payload->commStreamIds[0] = 100;
    std::vector<MsprofAdditionalInfo> mc2Input{mc2};
    ASSERT_TRUE(WriteBin(mc2Input, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.additional.mc2_comm_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_3_0);
    Analysis::Test::StubReserveFailureForVector<Mc2CommInfoData>();

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    auto result = worker.Run();
    Analysis::Test::ResetReserveFailureForVector<Mc2CommInfoData>();
    EXPECT_EQ(ANALYSIS_OK, result);
    EXPECT_TRUE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_hash.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldNotCreateCaptureAndMc2DatabasesWhenCppParserDisabled)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofCompactInfo capture{};
    capture.timeStamp = 123;
    capture.data.captureStreamInfoV2.deviceId = 0;
    capture.data.captureStreamInfoV2.modelId = 7;
    capture.data.captureStreamInfoV2.originalStreamId = 70002;
    capture.data.captureStreamInfoV2.streamId = 70003;
    capture.data.captureStreamInfoV2.captureStatus = 0;
    std::vector<MsprofCompactInfo> captureInput{capture};
    ASSERT_TRUE(WriteBin(captureInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.compact.capture_stream_info_v2.slice_0"));

    MsprofAdditionalInfo mc2{};
    auto payload = ReinterpretConvert<Analysis::Domain::Host::Cann::MsprofMc2CommInfo *>(mc2.data);
    payload->groupName = 99;
    payload->rankSize = 2;
    payload->rankId = 0;
    payload->usrRankId = 0;
    payload->streamId = 70002;
    payload->streamSize = 2;
    payload->commStreamIds[0] = 70004;
    payload->commStreamIds[1] = 70005;
    std::vector<MsprofAdditionalInfo> mc2Input{mc2};
    ASSERT_TRUE(WriteBin(mc2Input, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.additional.mc2_comm_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V3_3_0);

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    ASSERT_EQ(ANALYSIS_OK, worker.Run());

    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "stream_info.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "mc2_comm_info.db"})));
}

TEST_F(KernelParserWorkerUtest, ShouldSkipCaptureAndMc2WhenCppParserDisabledOnAnyPlatform)
{
    ASSERT_FALSE(Host::Cann::kEnableCaptureStreamMc2CppParser);
    UseRealHostTraceWorker();
    MsprofCompactInfo capture{};
    capture.data.captureStreamInfo.modelStreamId = 70;
    std::vector<MsprofCompactInfo> captureInput{capture};
    ASSERT_TRUE(WriteBin(captureInput, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.compact.capture_stream_info.slice_0"));

    MsprofAdditionalInfo mc2{};
    auto payload = ReinterpretConvert<Analysis::Domain::Host::Cann::MsprofMc2CommInfo *>(mc2.data);
    payload->streamId = 52;
    std::vector<MsprofAdditionalInfo> mc2Input{mc2};
    ASSERT_TRUE(WriteBin(mc2Input, File::PathJoin({TEST_HOST_FILE_PATH, "data"}),
                         "unaging.additional.mc2_comm_info.slice_0"));
    StubPlatformVersion(Analysis::Domain::Environment::Chip::CHIP_V1_1_0);

    KernelParserWorker worker(TEST_HOST_FILE_PATH);
    ASSERT_EQ(ANALYSIS_OK, worker.Run());
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "stream_info.db"})));
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "mc2_comm_info.db"})));
}
