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
#include <dirent.h>

#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/topo_callback_process.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"

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
        Application::TopoNodeDefinition definition;
        definition.id = TOPO_NODE(DEVICE_PROCESSING, "DeviceContextTest.Process");
        definition.scopeKind = Application::TopoScopeKind::DEVICE;
        definition.processDependencies = TOPO_DEPS(TOPO_NODE(DEVICE_PROCESSING, "Device.Init"));
        definition.creatorFactory = [](const Application::TopoBuildContext &) -> ProcessCreator
        {
            return []() -> std::unique_ptr<Process> {
                return std::unique_ptr<Process>(
                    new Application::TopoCallbackProcess([](DataInventory &) { return true; }));
            };
        };
        Application::TopoNodeRegistry::Register(std::move(definition));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
        EXPECT_TRUE(File::CreateDir(deviceDir));
        CreateJson(deviceDir);
    }

    static void TearDownTestCase()
    {
        Application::TopoNodeRegistry::MutableDefinitions().erase(
            TOPO_NODE(DEVICE_PROCESSING, "DeviceContextTest.Process"));
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
    EXPECT_EQ("1.0", context.GetCollectionVersion());
}

TEST_F(DeviceContextUTest, ShouldReadCollectionVersionTwoFromInfoJson)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteInfoJson(deviceDir, "2.0");
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetInfoJson());
    EXPECT_EQ("2.0", context.deviceContextInfo.deviceInfo.collectionVersion);
    EXPECT_EQ("2.0", context.GetCollectionVersion());
}

TEST_F(DeviceContextUTest, ShouldMarkCollectionVersionUnavailableWhenInfoJsonOmitsVersion)
{
    const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
    WriteInfoJson(deviceDir, nullptr);
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = deviceDir;

    ASSERT_TRUE(context.GetInfoJson());
    EXPECT_EQ("N/A", context.deviceContextInfo.deviceInfo.collectionVersion);
    EXPECT_EQ("N/A", context.GetCollectionVersion());
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

TEST_F(DeviceContextUTest, TestDeviceContextEntryShouldCreateSqliteDirWhenInitSucceeds)
{
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(true));
    MOCKER_CPP(static_cast<bool (ProcessControl::*)()>(&ProcessControl::ExecuteProcess)).stubs().will(returnValue(true));
    MOCKER_CPP(&ProcessControl::GetExecuteStat).stubs().will(returnValue(ExecuteProcessStat{}));
    DeviceContextEntry(PROF_DIR, "");
    EXPECT_TRUE(File::Exist(File::PathJoin({PROF_DIR, DEVICE_DIR, "sqlite"})));
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, SingleDeviceReturnsInitializationFailure)
{
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(false));
    auto context = DeviceContext::Create();
    DataInventory inventory;
    std::string statistics;
    EXPECT_FALSE(RunSingleDevice(File::PathJoin({PROF_DIR, DEVICE_DIR}), *context, inventory, statistics));
    EXPECT_FALSE(statistics.empty());
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, SingleDeviceReturnsExecutionFailureWithoutEnumeratingDevices)
{
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(true));
    MOCKER(GetDeviceDirectories).expects(never());
    MOCKER_CPP(static_cast<bool (ProcessControl::*)()>(&ProcessControl::ExecuteProcess))
        .expects(once()).will(returnValue(false));
    MOCKER_CPP(&ProcessControl::GetExecuteStat).stubs().will(returnValue(ExecuteProcessStat{}));
    auto context = DeviceContext::Create();
    DataInventory inventory;
    std::string statistics;
    EXPECT_FALSE(RunSingleDevice(File::PathJoin({PROF_DIR, DEVICE_DIR}), *context, inventory, statistics));
    EXPECT_FALSE(statistics.empty());
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, SingleDeviceReturnsExecutionSuccessWithoutEnumeratingDevices)
{
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(true));
    MOCKER(GetDeviceDirectories).expects(never());
    MOCKER_CPP(static_cast<bool (ProcessControl::*)()>(&ProcessControl::ExecuteProcess))
        .expects(once()).will(returnValue(true));
    MOCKER_CPP(&ProcessControl::GetExecuteStat).stubs().will(returnValue(ExecuteProcessStat{}));
    auto context = DeviceContext::Create();
    DataInventory inventory;
    std::string statistics;
    EXPECT_TRUE(RunSingleDevice(File::PathJoin({PROF_DIR, DEVICE_DIR}), *context, inventory, statistics));
    EXPECT_FALSE(statistics.empty());
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, SingleDeviceUsesDirectoryScopeInStatisticsAndStopAt)
{
    struct RegistryGuard
    {
        Application::TopoNodeCollection original = Application::TopoNodeRegistry::GetDefinitions();
        ~RegistryGuard() { Application::TopoNodeRegistry::MutableDefinitions() = std::move(original); }
    } guard;
    const auto id = TOPO_NODE(DEVICE_PROCESSING, "DeviceContextTest.Process");
    const auto definition = Application::TopoNodeRegistry::GetDefinitions().at(id);
    auto& definitions = Application::TopoNodeRegistry::MutableDefinitions();
    definitions.clear();
    definitions.emplace(id, definition);
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(true));
    const std::vector<std::pair<std::string, bool>> cases = {
        {"", true},
        {"DeviceContextTest.Process", false},
        {"Analysis::Domain::DeviceContextTest.Process", false},
        {"device_0/DeviceContextTest.Process", false},
        {"device_1/DeviceContextTest.Process", true},
    };
    for (const auto& suffix : {"", "/"})
    {
        const auto devicePath = File::PathJoin({PROF_DIR, DEVICE_DIR}) + suffix;
        for (const auto& test : cases)
        {
            SCOPED_TRACE(devicePath + ", stopAt=" + test.first);
            auto context = DeviceContext::Create();
            DataInventory inventory;
            std::string statistics;
            EXPECT_EQ(RunSingleDevice(devicePath, *context, inventory, statistics, test.first.c_str()), test.second);
            EXPECT_NE(statistics.find("[device_0/DeviceContextTest.Process]"), std::string::npos);
            EXPECT_EQ(statistics.find("dfxStop:true") != std::string::npos, !test.second);
        }
    }
    GlobalMockObject::verify();
}

TEST_F(DeviceContextUTest, TestDeviceContextEntryShouldCreateSqliteDirForEachDevice)
{
    const auto device1 = File::PathJoin({PROF_DIR, "device_1"});
    EXPECT_TRUE(File::CreateDir(device1));
    MOCKER_CPP(&DeviceContext::Init).stubs().will(returnValue(true));
    MOCKER_CPP(static_cast<bool (ProcessControl::*)()>(&ProcessControl::ExecuteProcess)).stubs().will(returnValue(true));
    MOCKER_CPP(&ProcessControl::GetExecuteStat).stubs().will(returnValue(ExecuteProcessStat{}));
    EXPECT_EQ(2, DeviceContextEntry(PROF_DIR, "").size());
    EXPECT_TRUE(File::Exist(File::PathJoin({PROF_DIR, DEVICE_DIR, "sqlite"})));
    EXPECT_TRUE(File::Exist(File::PathJoin({device1, "sqlite"})));
    GlobalMockObject::verify();
    // device_1 必须在本用例结束前删掉：PROF_DIR 要到 TearDownTestCase 才整体清理，
    // 而上面的 TestDeviceContextEntryShouldReturn1DataInventoryWhenInfoJsonInvalid
    // 断言的是 DeviceContextEntry(PROF_DIR, "").size() == 1。留着 device_1 会让那条
    // 用例只能靠 gtest 的声明顺序侥幸通过，一旦 shuffle 或调整顺序就会失败。
    File::RemoveDir(device1, 0);
}

TEST_F(DeviceContextUTest, TestGetSampleJsonShouldLoadHbmProfilingEvents)
{
    const std::string sampleDir = "./device_sample";
    ASSERT_TRUE(File::CreateDir(sampleDir));
    nlohmann::json sample = {
        {"ai_core_profiling", "off"},
        {"ai_core_metrics", "PipeUtilization"},
        {"ai_core_profiling_events", ""},
        {"ai_core_profiling_mode", "task-based"},
        {"aicore_sampling_interval", 10},
        {"aiv_profiling", "off"},
        {"aiv_metrics", "PipeUtilization"},
        {"aiv_profiling_events", ""},
        {"aiv_profiling_mode", "task-based"},
        {"aiv_sampling_interval", 10},
        {"hbm_profiling_events", "read"}
    };
    FileWriter writer(File::PathJoin({sampleDir, SAMPLE_JSON}));
    writer.WriteText(sample.dump());
    writer.Close();
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = sampleDir;

    ASSERT_TRUE(context.GetSampleJson());
    SampleInfo sampleInfo;
    context.Getter(sampleInfo);
    EXPECT_EQ("read", sampleInfo.hbmProfilingEvents);
    EXPECT_TRUE(File::RemoveDir(sampleDir, 0));
}

}  // Domain
}  // Analysis
