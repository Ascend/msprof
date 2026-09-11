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

#include <set>
#include <string>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/interface/py_interface/py_init_parser.h"

using Analysis::Application::ExportManager;
using Analysis::Application::ExportMode;
using Analysis::Domain::GetDeviceDirectories;
using Analysis::Interface::WrapRunPipeline;
using Analysis::Utils::File;
using Analysis::Utils::FileWriter;
using EnvContext = Analysis::Domain::Environment::Context;
using KernelParserWorker = Analysis::Domain::KernelParserWorker;

namespace
{
constexpr int ANALYSIS_INVALID_PARAM = 101;
constexpr int PIPELINE_CANN_TRACE = 0x01;
constexpr int PIPELINE_DEVICE_DATA = 0x02;
constexpr int PIPELINE_DB = 0x04;
constexpr int PIPELINE_TIMELINE = 0x08;
constexpr int PIPELINE_SUMMARY = 0x10;
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
        MOCKER_CPP(&EnvContext::Load).stubs().will(returnValue(true));
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

TEST_F(PyInitParserUtest, TestRunPipelineReturnsInvalidParamWhenReportsJsonNotExist)
{
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_TIMELINE, nullptr, nullptr,
                              "./not_exist_reports.json"),
              ANALYSIS_INVALID_PARAM);
}

TEST_F(PyInitParserUtest, TestRunPipelineRunsTimelineWithReportsJson)
{
    MOCKER_CPP(&ExportManager::Run).stubs().will(returnValue(true));
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_TIMELINE, nullptr, nullptr,
                              REPORTS_JSON_PATH.c_str()),
              Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestRunPipelineIgnoresReportsJsonWithoutTimeline)
{
    MOCKER_CPP(&ExportManager::Run).stubs().will(returnValue(true));
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_DB, nullptr, nullptr,
                              "./not_exist_reports.json"),
              Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestRunPipelineRunsAllStages)
{
    MOCKER_CPP(&KernelParserWorker::Run).stubs().will(returnValue(Analysis::ANALYSIS_OK));
    MOCKER_CPP(&ExportManager::Run).expects(exactly(3)).will(returnValue(true));
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA | PIPELINE_DB |
                              PIPELINE_TIMELINE | PIPELINE_SUMMARY),
              Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestRunPipelineSkipsDBWhenMsprofDBExists)
{
    MOCKER_CPP(&ExportManager::HasExportedMsprofDB).stubs().will(returnValue(true));
    MOCKER_CPP(&ExportManager::Run).expects(exactly(2)).will(returnValue(true));
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY),
              Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestRunPipelineSkipsHostAndDeviceWhenSqliteIsNonEmpty)
{
    MOCKER_CPP(&ExportManager::IsPythonParseComplete).stubs().will(returnValue(true));
    MOCKER_CPP(&KernelParserWorker::Run).expects(never());
    MOCKER_CPP(&GetDeviceDirectories).expects(never());
    EXPECT_EQ(CallRunPipeline(PROF_PATH.c_str(), PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA), Analysis::ANALYSIS_OK);
}

TEST_F(PyInitParserUtest, TestIsPythonParseCompleteReturnsTrueWhenSqliteIsNonEmpty)
{
    const std::string sqlitePath = File::PathJoin({HOST_PATH, "sqlite"});
    EXPECT_TRUE(File::CreateDir(sqlitePath));
    FileWriter(File::PathJoin({sqlitePath, "op_summary.db"})).WriteText("");
    EXPECT_TRUE(ExportManager::IsPythonParseComplete(PROF_PATH));
    EXPECT_TRUE(File::RemoveDir(sqlitePath, 0));
}

TEST_F(PyInitParserUtest, TestIsPythonParseCompleteReturnsFalseWhenSqliteIsEmpty)
{
    const std::string sqlitePath = File::PathJoin({HOST_PATH, "sqlite"});
    EXPECT_TRUE(File::CreateDir(sqlitePath));
    EXPECT_FALSE(ExportManager::IsPythonParseComplete(PROF_PATH));
    EXPECT_TRUE(File::RemoveDir(sqlitePath, 0));
}

TEST_F(PyInitParserUtest, TestHasExportedMsprofDBReturnsTrueWhenMsprofDBExists)
{
    const std::string dbPath = File::PathJoin({PROF_PATH, "msprof_001.db"});
    FileWriter(dbPath).WriteText("");
    EXPECT_TRUE(ExportManager::HasExportedMsprofDB(PROF_PATH));
    EXPECT_TRUE(File::DeleteFile(dbPath));
}
