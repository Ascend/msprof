/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/pipeline/unified_pipeline.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/interface/py_interface/py_init_parser.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using Analysis::Application::ExportManager;
using Analysis::Application::ExportMode;
using Analysis::Application::PIPELINE_CANN_TRACE;
using Analysis::Application::PIPELINE_DEVICE_DATA;
using Analysis::Application::PIPELINE_DB;
using Analysis::Application::PIPELINE_TIMELINE;
using Analysis::Application::PIPELINE_SUMMARY;
using Analysis::Domain::GetDeviceDirectories;
using Analysis::Interface::WrapRunPipeline;
using Analysis::Utils::File;
using Analysis::Utils::FileWriter;
using EnvContext = Analysis::Domain::Environment::Context;
using KernelParserWorker = Analysis::Domain::KernelParserWorker;

namespace
{
constexpr int ANALYSIS_INVALID_PARAM = 101;
Analysis::Application::PipelineRequest capturedRequest;
std::vector<std::string> capturedErrors;
void CaptureErrorLog(Analysis::Log *, const std::string &message, const std::string &level, const std::string &,
                     const uint32_t &)
{
    if (level == "[ERROR]")
    {
        capturedErrors.push_back(message);
    }
}

int CaptureUnifiedRequest(const Analysis::Application::PipelineRequest& request)
{
    capturedRequest = request;
    return Analysis::ANALYSIS_OK;
}
std::string capturedExportPath;
std::unordered_set<Analysis::Application::TopoNodeId, Analysis::Application::TopoNodeIdHash> capturedExportNodes;
bool CaptureExportGraph(const Analysis::Application::TopoGraphBuilder &builder,
                        Analysis::Infra::RuntimeProcessCollection &, const std::string &path)
{
    capturedExportPath = path;
    capturedExportNodes.clear();
    for (const auto &node : builder.GetRuntimeNodeKeys())
    {
        capturedExportNodes.insert(node.first);
    }
    return true;
}
bool HasExportStage(Analysis::Application::TopoNodeStage stage)
{
    return std::any_of(capturedExportNodes.begin(), capturedExportNodes.end(),
                       [stage](const Analysis::Application::TopoNodeId &node) { return node.stage == stage; });
}
const std::string BASE_PATH = "./py_init_parser_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string HOST_PATH = File::PathJoin({PROF_PATH, "host"});
const std::string REPORTS_JSON_PATH = File::PathJoin({BASE_PATH, "reports.json"});
PyObject *NewStringOrNone(const char *value)
{
    if (value == nullptr) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return PyUnicode_FromString(value);
}

int CallRunPipeline(const char *profPath, int flags, const char *cannTracePath = nullptr,
                    const char *devicePath = nullptr, const char *reportsJson = nullptr)
{
    PyObject *args = PyTuple_New(5);
    PyTuple_SetItem(args, 0, NewStringOrNone(profPath));
    PyTuple_SetItem(args, 1, PyLong_FromLong(flags));
    PyTuple_SetItem(args, 2, NewStringOrNone(cannTracePath));
    PyTuple_SetItem(args, 3, NewStringOrNone(devicePath));
    PyTuple_SetItem(args, 4, NewStringOrNone(reportsJson));
    PyObject *result = WrapRunPipeline(nullptr, args);
    Py_DECREF(args);
    if (result == nullptr) {
        PyErr_Clear();
        return Analysis::ANALYSIS_ERROR;
    }
    const int ret = static_cast<int>(PyLong_AsLong(result));
    Py_DECREF(result);
    return ret;
}
}  // namespace

class PyInitParserUtest : public testing::Test
{
   protected:
    static void SetUpTestCase()
    {
        Py_Initialize();
        File::CreateDir(BASE_PATH);
        File::CreateDir(PROF_PATH);
        File::CreateDir(HOST_PATH);
        FileWriter reportsJson(REPORTS_JSON_PATH);
        reportsJson.WriteText("{}");
    }

    static void TearDownTestCase()
    {
        GlobalMockObject::verify();
        File::RemoveDir(BASE_PATH, 0);
        Py_Finalize();
    }

    void SetUp() override
    {
        GlobalMockObject::verify();
        capturedRequest = Analysis::Application::PipelineRequest{};
        capturedErrors.clear();
        capturedExportPath.clear();
        capturedExportNodes.clear();
        FileWriter(REPORTS_JSON_PATH).WriteText("{}");
        MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
        MOCKER(&Analysis::Application::ExecuteTopoGraph).stubs().will(invoke(CaptureExportGraph));
        MOCKER(&Analysis::Application::RunUnifiedPipeline).stubs().will(invoke(CaptureUnifiedRequest));
    }

    void TearDown() override { GlobalMockObject::verify(); }
};

TEST_F(PyInitParserUtest, TestRunPipelineReturnsInvalidParamWhenProfPathIsEmpty)
{
    EXPECT_EQ(CallRunPipeline("", PIPELINE_DEVICE_DATA), ANALYSIS_INVALID_PARAM);
}

TEST_F(PyInitParserUtest, TestRunPipelineReturnsInvalidParamWhenFlagsIsZero)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), 0), ANALYSIS_INVALID_PARAM);
}

TEST_F(PyInitParserUtest, TestRunPipelineRejectsRemovedPlanningFlag)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_DEVICE_DATA | 0x20), ANALYSIS_INVALID_PARAM);
}

TEST_F(PyInitParserUtest, TestRunPipelineReturnsInvalidParamWhenReportsJsonNotExist)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_TIMELINE, nullptr, nullptr,
                              "./not_exist_reports.json"),
              ANALYSIS_INVALID_PARAM);
}

TEST_F(PyInitParserUtest, TestRunPipelineRunsTimelineWithReportsJson)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_TIMELINE, nullptr, nullptr,
                              REPORTS_JSON_PATH.c_str()), Analysis::ANALYSIS_OK);
    EXPECT_EQ(capturedRequest.reportsJson, REPORTS_JSON_PATH);
    EXPECT_EQ(capturedRequest.flags, PIPELINE_TIMELINE);
}

TEST_F(PyInitParserUtest, TestRunPipelineIgnoresReportsJsonWithoutTimeline)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_DB, nullptr, nullptr,
                              "./not_exist_reports.json"), Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestRunPipelineDelegatesAllStagesToOnePlanner)
{
    const int flags = PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA | PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY;
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), flags), Analysis::ANALYSIS_OK);
    EXPECT_EQ(capturedRequest.flags, flags);
    EXPECT_EQ(capturedRequest.profPath, PROF_PATH);
    EXPECT_EQ(capturedRequest.hostPath, HOST_PATH);
}

TEST_F(PyInitParserUtest, TestRunPipelinePreservesExplicitPaths)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_DEVICE_DATA, "custom_host", "custom_device"), Analysis::ANALYSIS_OK);
    EXPECT_EQ(capturedRequest.hostPath, "custom_host");
    EXPECT_EQ(capturedRequest.devicePath, "custom_device");
}

TEST_F(PyInitParserUtest, TestRunPipelineWithoutHostDoesNotLogErrors)
{
    const auto profPath = File::PathJoin({BASE_PATH, "PROF_device_only"});
    const auto devicePath = File::PathJoin({profPath, "device_0"});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_FALSE(File::Exist(File::PathJoin({profPath, "host"})));
    // Avoid reopening the singleton logger while checking directory errors.
    MOCKER_CPP(&Analysis::Log::Init).stubs().will(returnValue(Analysis::ANALYSIS_OK));
    MOCKER_CPP(&Analysis::Log::LogMsg).stubs().will(invoke(CaptureErrorLog));

    for (const char *explicitDevicePath : {static_cast<const char *>(nullptr), devicePath.c_str()})
    {
        EXPECT_EQ(CallRunPipeline(profPath.c_str(), PIPELINE_DEVICE_DATA, nullptr, explicitDevicePath),
                  Analysis::ANALYSIS_OK);
        EXPECT_EQ(capturedRequest.hostPath, profPath);
        EXPECT_EQ(capturedRequest.devicePath, devicePath);
        EXPECT_EQ(capturedRequest.flags, PIPELINE_DEVICE_DATA);
    }
    EXPECT_TRUE(capturedErrors.empty()) << testing::PrintToString(capturedErrors);
}

TEST_F(PyInitParserUtest, LegacyExportWithoutHostDoesNotLogErrors)
{
    const auto profPath = File::PathJoin({BASE_PATH, "PROF_device_only_export"});
    const auto devicePath = File::PathJoin({profPath, "device_0"});
    ASSERT_TRUE(File::CreateDir(profPath));
    ASSERT_TRUE(File::CreateDir(devicePath));
    ASSERT_TRUE(File::CreateDir(File::PathJoin({devicePath, "data"})));
    FileWriter(File::PathJoin({devicePath, "info.json"})).WriteText("{\"platform_version\":\"5\"}");
    FileWriter(REPORTS_JSON_PATH).WriteText("{\"summary_process\":{\"step_trace\":true}}");
    ASSERT_FALSE(File::Exist(File::PathJoin({profPath, "host"})));
    MOCKER_CPP(&Analysis::Log::LogMsg).stubs().will(invoke(CaptureErrorLog));

    ExportManager manager(profPath, REPORTS_JSON_PATH);
    EXPECT_TRUE(manager.Run({ExportMode::SUMMARY}));
    EXPECT_EQ(capturedExportPath, profPath);
    EXPECT_FALSE(HasExportStage(Analysis::Application::TopoNodeStage::HOST_PROCESSING));
    EXPECT_TRUE(HasExportStage(Analysis::Application::TopoNodeStage::DEVICE_PROCESSING));
    EXPECT_TRUE(HasExportStage(Analysis::Application::TopoNodeStage::SUMMARY_GENERATION));
    EXPECT_TRUE(capturedErrors.empty()) << testing::PrintToString(capturedErrors);
}

TEST_F(PyInitParserUtest, TestRunPipelinePropagatesUnifiedFailure)
{
    MOCKER(&Analysis::Application::RunUnifiedPipeline).reset();
    MOCKER(&Analysis::Application::RunUnifiedPipeline).stubs().will(returnValue(Analysis::ANALYSIS_ERROR));
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_CANN_TRACE), Analysis::ANALYSIS_ERROR);
}

TEST_F(PyInitParserUtest, LegacyExportInterfacesUseExportManagerAndBuildCompleteGraph)
{
    using namespace Analysis::Interface;
    const std::vector<std::pair<PyCFunction, int>> exports{{WrapExportUnifiedDB, PIPELINE_DB},
                                                           {WrapExportTimeline, PIPELINE_TIMELINE | PIPELINE_DB},
                                                           {WrapExportSummary, PIPELINE_SUMMARY | PIPELINE_DB}};
    for (const auto &entry : exports)
    {
        PyObject *args = entry.first == WrapExportTimeline
                             ? Py_BuildValue("(ss)", PROF_PATH.c_str(), REPORTS_JSON_PATH.c_str())
                             : Py_BuildValue("(s)", PROF_PATH.c_str());
        PyObject *result = entry.first(nullptr, args);
        Py_DECREF(args);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(PyLong_AsLong(result), Analysis::ANALYSIS_OK);
        Py_DECREF(result);
        EXPECT_EQ(capturedExportPath, PROF_PATH);
        EXPECT_EQ(HasExportStage(Analysis::Application::TopoNodeStage::DATABASE_PERSISTENCE),
                  (entry.second & PIPELINE_DB) != 0);
        EXPECT_EQ(HasExportStage(Analysis::Application::TopoNodeStage::TIMELINE_EXPORT),
                  (entry.second & PIPELINE_TIMELINE) != 0);
        EXPECT_EQ(HasExportStage(Analysis::Application::TopoNodeStage::SUMMARY_GENERATION),
                  (entry.second & PIPELINE_SUMMARY) != 0);
        EXPECT_TRUE(HasExportStage(Analysis::Application::TopoNodeStage::HOST_PROCESSING));
    }
}

TEST_F(PyInitParserUtest, ExportManagerRunPreservesRequestedModesAndReportPath)
{
    FileWriter(REPORTS_JSON_PATH)
        .WriteText("{\"json_process\":{\"ascend\":true},\"summary_process\":{\"step_trace\":true}}");
    ExportManager manager(PROF_PATH, REPORTS_JSON_PATH);
    const std::vector<std::pair<std::set<ExportMode>, int>> selections{
        {{ExportMode::DB}, PIPELINE_DB},
        {{ExportMode::TIMELINE}, PIPELINE_TIMELINE},
        {{ExportMode::SUMMARY}, PIPELINE_SUMMARY},
        {{ExportMode::DB, ExportMode::TIMELINE, ExportMode::SUMMARY},
         PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY}};
    for (const auto &selection : selections)
    {
        ASSERT_TRUE(manager.Run(selection.first));
        using Analysis::Application::TopoNodeStage;
        EXPECT_EQ(capturedExportPath, PROF_PATH);
        EXPECT_EQ(HasExportStage(TopoNodeStage::DATABASE_PERSISTENCE), (selection.second & PIPELINE_DB) != 0);
        EXPECT_EQ(HasExportStage(TopoNodeStage::TIMELINE_EXPORT), (selection.second & PIPELINE_TIMELINE) != 0);
        EXPECT_EQ(HasExportStage(TopoNodeStage::SUMMARY_GENERATION), (selection.second & PIPELINE_SUMMARY) != 0);
        if ((selection.second & PIPELINE_TIMELINE) != 0)
        {
            EXPECT_EQ(capturedExportNodes.count({TopoNodeStage::TIMELINE_EXPORT, "Task Scheduler", "export"}), 1U);
            EXPECT_EQ(capturedExportNodes.count({TopoNodeStage::TIMELINE_EXPORT, "NPU MEM", "export"}), 0U);
        }
        if ((selection.second & PIPELINE_SUMMARY) != 0)
        {
            EXPECT_EQ(capturedExportNodes.count({TopoNodeStage::SUMMARY_GENERATION, "STEP_TRACE", "export"}), 1U);
            EXPECT_EQ(capturedExportNodes.count({TopoNodeStage::SUMMARY_GENERATION, "NPU_MEM", "export"}), 0U);
        }
    }
}

TEST_F(PyInitParserUtest, ExportManagerRunRejectsInvalidModesBeforeDispatch)
{
    MOCKER(&Analysis::Application::ExecuteTopoGraph).reset();
    MOCKER(&Analysis::Application::ExecuteTopoGraph).expects(never());
    ExportManager manager(PROF_PATH);
    EXPECT_FALSE(manager.Run({}));
    EXPECT_FALSE(manager.Run({static_cast<ExportMode>(99)}));
    EXPECT_FALSE(manager.Run({ExportMode::DB, static_cast<ExportMode>(99)}));
}

TEST_F(PyInitParserUtest, ExportManagerRunPropagatesGraphExecutionFailure)
{
    MOCKER(&Analysis::Application::ExecuteTopoGraph).reset();
    MOCKER(&Analysis::Application::ExecuteTopoGraph).expects(once()).will(returnValue(false));
    ExportManager manager(PROF_PATH);
    EXPECT_FALSE(manager.Run({ExportMode::DB}));
}

TEST_F(PyInitParserUtest, TestHasExportedMsprofDBReturnsTrueWhenMsprofDBExists)
{
    const std::string dbPath = File::PathJoin({PROF_PATH, "msprof_001.db"});
    FileWriter(dbPath).WriteText("");
    EXPECT_TRUE(ExportManager::HasExportedMsprofDB(PROF_PATH));
    EXPECT_TRUE(File::DeleteFile(dbPath));
}
