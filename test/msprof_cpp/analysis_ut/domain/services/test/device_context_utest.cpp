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
#include "analysis/csrc/domain/services/device_context/device_context.h"

#include <dirent.h>
#include <unordered_map>
#include <typeindex>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/process/process_topo.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"


namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
using namespace Analysis::Infra;

const auto PROF_DIR = "./PROF_0";
const auto DEVICE_DIR = "device_0";
const auto INFO_JSON = "info.json.0";
const auto SAMPLE_JSON = "sample.json";


class DeviceContextUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
        EXPECT_TRUE(File::CreateDir(deviceDir));
        CreateJson(deviceDir);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(PROF_DIR, 0));
    }

    static void CreateJson(const std::string &filePath)
    {
        // info.json.0
        nlohmann::json info = {
            {"platform_version", "5"},
            {"devices", "0"},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"},
                             {"aic_frequency", "1850"},
                             {"aiv_frequency", "1850"},
                             {"ai_core_num", 25},
                             {"aiv_num", 25}}}},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        FileWriter infoWriter(File::PathJoin({filePath, INFO_JSON}));
        infoWriter.WriteText(info.dump());
        // sample.json
        nlohmann::json sample = {
            {"ai_core_profiling", "on"},
            {"ai_core_metrics", "PipeUtilization"},
            {"ai_core_profiling_events", "0x416,0x417,0x9,0x302,0xc,0x303,0x54,0x55"},
            {"ai_core_profiling_mode", "task-based"},
            {"aicore_sampling_interval", 10},
            {"aiv_profiling", "on"},
            {"aiv_metrics", "PipeUtilization"},
            {"aiv_profiling_events", "0x416,0x417, "}
        };
        FileWriter sampleWriter(File::PathJoin({filePath, SAMPLE_JSON}));
        sampleWriter.WriteText(sample.dump());
    }

    static void WriteInfoJson(const std::string &filePath, const nlohmann::json &version)
    {
        nlohmann::json info = {
            {"platform_version", "5"},
            {"devices", "0"},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"},
                             {"aic_frequency", "1850"},
                             {"aiv_frequency", "1850"},
                             {"ai_core_num", 25},
                             {"aiv_num", 25}}}}
        };
        if (!version.is_null()) {
            info["version"] = version;
        }
        FileWriter writer(File::PathJoin({filePath, INFO_JSON}));
        writer.WriteText(info.dump());
    }

    static void WriteSampleJson(const std::string &filePath, const nlohmann::json &llcProfiling,
                                const std::string &aiCoreProfilingMode = "")
    {
        nlohmann::json sample = {
            {"ai_core_profiling", "off"},
            {"ai_core_metrics", ""},
            {"ai_core_profiling_events", ""},
            {"ai_core_profiling_mode", aiCoreProfilingMode},
            {"aicore_sampling_interval", 10},
            {"aiv_profiling", "off"},
            {"aiv_metrics", ""},
            {"aiv_profiling_events", ""},
            {"aiv_profiling_mode", ""},
            {"aiv_sampling_interval", 10}
        };
        if (!llcProfiling.is_null()) {
            sample["llc_profiling"] = llcProfiling;
        }
        FileWriter writer(File::PathJoin({filePath, SAMPLE_JSON}));
        writer.WriteText(sample.dump());
    }
};

TEST_F(DeviceContextUTest, ShouldReadCollectionVersionOneFromInfoJson)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteInfoJson(deviceDir, "1.0");
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetInfoJson());
    EXPECT_EQ("1.0", context.deviceContextInfo.deviceInfo.collectionVersion);
}

TEST_F(DeviceContextUTest, ShouldReadCollectionVersionTwoFromInfoJson)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteInfoJson(deviceDir, "2.0");
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetInfoJson());
    EXPECT_EQ("2.0", context.deviceContextInfo.deviceInfo.collectionVersion);
}

TEST_F(DeviceContextUTest, ShouldMarkCollectionVersionUnavailableWhenInfoJsonOmitsVersion)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteInfoJson(deviceDir, nullptr);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetInfoJson());
    EXPECT_EQ("N/A", context.deviceContextInfo.deviceInfo.collectionVersion);
}

TEST_F(DeviceContextUTest, ShouldReadLlcReadModeFromSampleJson)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteSampleJson(deviceDir, "read");
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetSampleJson());
    EXPECT_EQ("read", context.deviceContextInfo.sampleInfo.llcProfiling);
}

TEST_F(DeviceContextUTest, ShouldReadLlcWriteModeFromSampleJson)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteSampleJson(deviceDir, "write");
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetSampleJson());
    EXPECT_EQ("write", context.deviceContextInfo.sampleInfo.llcProfiling);
}

TEST_F(DeviceContextUTest, ShouldDefaultLlcModeWhenSampleJsonOmitsField)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteSampleJson(deviceDir, nullptr);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetSampleJson());
    EXPECT_TRUE(context.deviceContextInfo.sampleInfo.llcProfiling.empty());
}

TEST_F(DeviceContextUTest, TestDeviceContextEntryShouldReturn1DataInventoryWhenInfoJsonInvalid)
{
    std::unordered_map<std::type_index, RegProcessInfo> processCollection;
    ProcessControl processControl(processCollection);
    ProcessTopo processTopo(processCollection);
    EXPECT_EQ(1, DeviceContextEntry(PROF_DIR, "").size());
}

TEST_F(DeviceContextUTest, TestGetDeviceDirectoriesShouldPrintErrorLogWhenOpenDirectoryFailed)
{
    MOCKER_CPP(opendir).stubs().will(returnValue((DIR*)nullptr));
    auto subdirs = GetDeviceDirectories(PROF_DIR);
    EXPECT_TRUE(subdirs.empty());
    GlobalMockObject::verify();
}

struct dirent* Mockreaddir(DIR *dir)
{
    errno = ENOENT; // 设置errno为ENOENT
    return nullptr; // 返回nullptr
}

TEST_F(DeviceContextUTest, TestGetDeviceDirectoriesShouldPrintErrorLogWhenReadDirectoryFailed)
{
    MOCKER_CPP(readdir).stubs().will(invoke(Mockreaddir, nullptr));
    auto subdirs = GetDeviceDirectories(PROF_DIR);
    EXPECT_TRUE(subdirs.empty());
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, TestDeviceContextEntryShouldReturnEmptyVectorWhenNoProfDir)
{
    MOCKER_CPP(opendir).stubs().will(returnValue((DIR*)nullptr));
    const char *stopAt = "";
    auto processDataVec = DeviceContextEntry(PROF_DIR, stopAt);
    EXPECT_TRUE(processDataVec.empty());
    GlobalMockObject::verify();
}

}  // Domain
}  // Analysis
