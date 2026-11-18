/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include <dlfcn.h>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"

namespace LogMacroScope
{
void EmitMessages(int& evaluated)
{
    DEBUG("debug-probe %", ++evaluated);
    INFO("info-probe");
    WARN("warn-probe");
    ERROR("error-probe");
}
}

using namespace Analysis;
using namespace Analysis::Utils;

class LogUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        Log::GetInstance().level_.store(LogLevel::INFO);
    }
};

TEST_F(LogUTest, TestFormatShouldReturnFormatStringWhenNoArgs)
{
    ASSERT_EQ("test format", Format("test format"));
}

TEST_F(LogUTest, TestFormatShouldReplacePercentSignWhenArgs)
{
    ASSERT_EQ("test format key: 1", Format("test format %: %", "key", 1));
}

TEST_F(LogUTest, TestFormatShouldReturnPercentSignWhenDoublePercentSign)
{
    ASSERT_EQ("test format key: 1 %", Format("test format %: % %%", "key", 1));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenEmptyLogPath)
{
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init(""));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenCreateLogDirFail)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(false));
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestInitShouldReturnErrorWhenOpenLogFileFail)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&File::Access)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&FileWriter::Open).stubs();
    ASSERT_EQ(ANALYSIS_ERROR, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestInitShouldReturnOKWhenOpenLogSuccess)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&File::CreateDir)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&File::Access)
        .stubs().will(returnValue(true));
    MOCKER_CPP(&FileWriter::IsOpen)
        .stubs().will(returnValue(true));
    ASSERT_EQ(ANALYSIS_OK, Log::GetInstance().Init("test"));
}

TEST_F(LogUTest, TestGetTimeShouldReturnCurrentTime)
{
    std::string currentTime = Log::GetTime();
    const std::string timeFormat = "yyyy-mm-dd HH:MM:SS";
    ASSERT_EQ(timeFormat.size(), currentTime.size());
}

TEST_F(LogUTest, TestGetTimeReturnCurrentTimeFailed)
{
    MOCKER(&localtime_r)
        .stubs().will(returnValue(static_cast<tm*>(nullptr)));
    std::string currentTime = Log::GetTime();
    const std::string defaultTime = "9999-99-99 99:99:99";
    ASSERT_EQ(defaultTime, currentTime);
}

TEST_F(LogUTest, TestLogMsg)
{
    GlobalMockObject::verify();
    INFO("Test: %", "LogMsg");
}

TEST_F(LogUTest, TestPrintMsg)
{
    GlobalMockObject::verify();
    const std::string currTime = "2023-11-15 00:00:00";
    MOCKER_CPP(&Log::GetTime)
        .stubs().will(returnValue(currTime));
    testing::internal::CaptureStdout();
    PRINT_INFO("Test: %", "PrintMsg");
    const std::string stdOut = testing::internal::GetCapturedStdout();
    const std::string out = Format("2023-11-15 00:00:00 [INFO] [%] log_utest.cpp: Test: PrintMsg\n", getpid());
    ASSERT_EQ(out, stdOut);
}

TEST_F(LogUTest, TestGetFileNameShouldReturnFileNameWhenInputPath)
{
    ASSERT_EQ("test", Log::GetFileName("/ut/log/test"));
}

namespace
{
std::string logTestLibrary;
int LocateLogTestLibrary(const void*, Dl_info* result)
{
    result->dli_fname = logTestLibrary.c_str();
    return 1;
}
struct ScopedLogDirectory
{
    std::string path;
    ScopedLogDirectory()
    {
        char temporary[] = "/tmp/msprof-log-utest-XXXXXX";
        const auto created = mkdtemp(temporary);
        if (created == nullptr) { throw std::runtime_error("Cannot create log test directory"); }
        path = created;
        File::CreateDir(File::PathJoin({path, "analysis"}));
        File::CreateDir(File::PathJoin({path, "analysis", "msconfig"}));
        logTestLibrary = File::PathJoin({path, "analysis", "lib64", "msprof_analysis.so"});
    }
    ~ScopedLogDirectory()
    {
        Log::GetInstance().logWriter_.Close();
        File::RemoveDir(path, 0);
    }
    std::string Config() const { return File::PathJoin({path, "analysis", "msconfig", "msprof_config.json"}); }
};
std::string ReadLogFile(const std::string& path)
{
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
}

TEST_F(LogUTest, FourThresholdsWriteExpectedMessagesWithoutEvaluatingFilteredArguments)
{
    ScopedLogDirectory temporary;
    MOCKER(&dladdr).stubs().will(invoke(LocateLogTestLibrary));
    const std::vector<std::string> levels = {"DEBUG", "INFO", "WARN", "ERROR"};
    for (size_t threshold = 0; threshold < levels.size(); ++threshold)
    {
        {
            std::ofstream config(temporary.Config());
            config << "{\"log\":{\"level\":" << threshold << "}}";
        }
        const auto directory = File::PathJoin({temporary.path, levels[threshold]});
        ASSERT_EQ(Log::GetInstance().Init(directory), ANALYSIS_OK);
        int evaluated = 0;
        LogMacroScope::EmitMessages(evaluated);
        EXPECT_EQ(evaluated, threshold == 0 ? 1 : 0);
        const auto text = ReadLogFile(File::PathJoin({directory, "msprof_analysis_" + std::to_string(getpid()) + ".log"}));
        const std::vector<std::string> probes = {"debug-probe", "info-probe", "warn-probe", "error-probe"};
        for (size_t index = 0; index < probes.size(); ++index)
        {
            EXPECT_EQ(text.find(probes[index]) != std::string::npos, index >= threshold);
        }
    }
}

TEST_F(LogUTest, FailedInitializationPreservesOriginalSinkAndLevel)
{
    ScopedLogDirectory temporary;
    MOCKER(&dladdr).stubs().will(invoke(LocateLogTestLibrary));
    { std::ofstream config(temporary.Config()); config << "{\"log\":{\"level\":0}}"; }
    const auto directory = File::PathJoin({temporary.path, "original"});
    ASSERT_EQ(Log::GetInstance().Init(directory), ANALYSIS_OK);
    { std::ofstream config(temporary.Config()); config << "{\"log\":{\"level\":3}}"; }
    EXPECT_EQ(Log::GetInstance().Init(""), ANALYSIS_ERROR);
    MOCKER_CPP(&File::CreateDir).stubs().will(returnValue(false));
    EXPECT_EQ(Log::GetInstance().Init("unavailable"), ANALYSIS_ERROR);
    GlobalMockObject::verify();
    MOCKER_CPP(&FileWriter::Open).stubs();
    EXPECT_EQ(Log::GetInstance().Init(directory), ANALYSIS_ERROR);
    GlobalMockObject::verify();
    EXPECT_TRUE(Log::GetInstance().ShouldLog(LogLevel::DEBUG));
    DEBUG("original-sink-still-active");
    const auto path = File::PathJoin({directory, "msprof_analysis_" + std::to_string(getpid()) + ".log"});
    EXPECT_NE(ReadLogFile(path).find("original-sink-still-active"), std::string::npos);
}

TEST_F(LogUTest, InvalidConfigurationWarnsAndUsesInfo)
{
    ScopedLogDirectory temporary;
    MOCKER(&dladdr).stubs().will(invoke(LocateLogTestLibrary));
    const std::vector<std::string> cases = {"[]", "{", "{\"log\":[]}",
        "{\"log\":{\"level\":-1}}", "{\"log\":{\"level\":4}}",
        "{\"log\":{\"level\":18446744073709551615}}", "{\"log\":{\"level\":1e10000}}",
        "{\"log\":{\"level\":true}}", "{\"log\":{\"level\":false}}", "{\"log\":{\"level\":1.0}}",
        "{\"log\":{\"level\":null}}", "{\"log\":{\"level\":[]}}",
        "{\"log\":{\"level\":\"DEBUG\"}}", "{\"log\":{\"level\":\"0\"}}"};
    for (const auto& content : cases)
    {
        { std::ofstream config(temporary.Config()); config << content; }
        testing::internal::CaptureStdout();
        EXPECT_EQ(Log::GetInstance().Init(File::PathJoin({temporary.path, "logs"})), ANALYSIS_OK);
        const auto console = testing::internal::GetCapturedStdout();
        EXPECT_NE(console.find("[WARN]"), std::string::npos);
        EXPECT_FALSE(Log::GetInstance().ShouldLog(LogLevel::DEBUG));
        EXPECT_TRUE(Log::GetInstance().ShouldLog(LogLevel::INFO));
    }
}

TEST_F(LogUTest, MissingConfigurationUsesInfoWithoutWarning)
{
    ScopedLogDirectory temporary;
    MOCKER(&dladdr).stubs().will(invoke(LocateLogTestLibrary));
    testing::internal::CaptureStdout();
    EXPECT_EQ(Log::GetInstance().Init(File::PathJoin({temporary.path, "logs"})), ANALYSIS_OK);
    const auto console = testing::internal::GetCapturedStdout();
    EXPECT_EQ(console.find("[WARN]"), std::string::npos);
    EXPECT_FALSE(Log::GetInstance().ShouldLog(LogLevel::DEBUG));
    EXPECT_TRUE(Log::GetInstance().ShouldLog(LogLevel::INFO));
}

TEST_F(LogUTest, ConfigurationLargerThan64KiBUsesConfiguredLevel)
{
    ScopedLogDirectory temporary;
    MOCKER(&dladdr).stubs().will(invoke(LocateLogTestLibrary));
    {
        std::ofstream config(temporary.Config());
        config << std::string(65537, ' ') << "{\"log\":{\"level\":0}}";
    }
    testing::internal::CaptureStdout();
    EXPECT_EQ(Log::GetInstance().Init(File::PathJoin({temporary.path, "logs"})), ANALYSIS_OK);
    const auto console = testing::internal::GetCapturedStdout();
    EXPECT_EQ(console.find("[WARN]"), std::string::npos);
    EXPECT_TRUE(Log::GetInstance().ShouldLog(LogLevel::DEBUG));
}
