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

#include "analysis/csrc/interface/py_interface/py_init_parser.h"

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/include/export_mode_enum.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Interface
{
using KernelParserWorker = Analysis::Domain::KernelParserWorker;
using EnvContext = Analysis::Domain::Environment::Context;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace
{
constexpr int ANALYSIS_INVALID_PARAM = 101;
constexpr int PIPELINE_CANN_TRACE = 0x01;
constexpr int PIPELINE_DEVICE_DATA = 0x02;
constexpr int PIPELINE_DB = 0x04;
constexpr int PIPELINE_TIMELINE = 0x08;
constexpr int PIPELINE_SUMMARY = 0x10;
constexpr int PIPELINE_ALL =
    PIPELINE_CANN_TRACE | PIPELINE_DEVICE_DATA | PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY;

std::string GetPipelineFlagsString(int flags)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << flags;
    return oss.str();
}

int RunCannTrace(const std::string &cannTracePath)
{
    if (cannTracePath.empty() || !File::CheckDir(cannTracePath))
    {
        ERROR("Host CANN trace parsing path is invalid, path: %.", cannTracePath);
        return ANALYSIS_INVALID_PARAM;
    }
    INFO("Start host CANN trace parsing, path: %.", cannTracePath);
    KernelParserWorker parserWorker(cannTracePath);
    const int ret = parserWorker.Run();
    INFO("Host CANN trace parsing finished, path: %, ret: %.", cannTracePath, ret);
    return ret;
}

int RunDeviceData(const std::string &devicePath)
{
    const std::string targetDir = File::ParentPath(devicePath);
    if (!File::CheckDir(targetDir))
    {
        ERROR("Device data parsing path is invalid, path: %.", targetDir);
        return ANALYSIS_INVALID_PARAM;
    }
    INFO("Start device data parsing, path: %.", targetDir);
    DeviceContextEntry(targetDir.c_str(), "");
    INFO("Device data parsing finished, path: %.", targetDir);
    return ANALYSIS_OK;
}

int RunExport(const std::string &profPath, const std::string &jsonPath,
              const std::set<Analysis::Application::ExportMode> &exportModes)
{
    Analysis::Application::ExportManager exportManager(profPath, jsonPath);
    return exportManager.Run(exportModes) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

std::string ResolveCannTracePath(const std::string &profPath, const char *cannTracePath)
{
    if (cannTracePath != nullptr && cannTracePath[0] != '\0')
    {
        return cannTracePath;
    }
    const std::string hostPath = File::PathJoin({profPath, "host"});
    return File::CheckDir(hostPath) ? hostPath : profPath;
}

std::string ResolveDevicePath(const std::string &profPath, const char *devicePath)
{
    if (devicePath != nullptr && devicePath[0] != '\0')
    {
        return devicePath;
    }
    const std::string hostPath = File::PathJoin({profPath, "host"});
    if (File::CheckDir(hostPath))
    {
        return hostPath;
    }
    const std::vector<std::string> deviceDirs = GetDeviceDirectories(profPath);
    return deviceDirs.empty() ? "" : deviceDirs.front();
}

int RunPipeline(const char *profPath, int flags, const char *cannTracePath, const char *devicePath,
                const char *reportsJson)
{
    if (profPath == nullptr || profPath[0] == '\0' || flags == 0 || (flags & ~PIPELINE_ALL) != 0)
    {
        return ANALYSIS_INVALID_PARAM;
    }
    if ((flags & PIPELINE_TIMELINE) != 0 && reportsJson != nullptr && reportsJson[0] != '\0' &&
        !FileReader::Check(reportsJson))
    {
        return ANALYSIS_INVALID_PARAM;
    }
    const std::string profPathStr(profPath);
    if (!File::CheckDir(profPathStr))
    {
        return ANALYSIS_INVALID_PARAM;
    }
    if (Log::GetInstance().Init(File::PathJoin({profPathStr, "mindstudio_profiler_log"})) != 0)
    {
        ERROR("Init msprof log failed, path is %.", profPathStr);
    }
    if (!EnvContext::GetInstance().Load({profPathStr}))
    {
        ERROR("Context load failed, path is %.", profPathStr);
        return ANALYSIS_ERROR;
    }
    const std::string flagsStr = GetPipelineFlagsString(flags);
    INFO("Start C pipeline, prof path: %, flags: %.", profPathStr, flagsStr);
    const bool isPythonParseComplete = Analysis::Application::ExportManager::IsPythonParseComplete(profPathStr);
    const bool hasExportStage = (flags & (PIPELINE_DB | PIPELINE_TIMELINE | PIPELINE_SUMMARY)) != 0;
    const bool hasExportedMsprofDB = Analysis::Application::ExportManager::HasExportedMsprofDB(profPathStr);
    bool needDbExport = hasExportStage && !hasExportedMsprofDB;
    if (hasExportStage && hasExportedMsprofDB)
    {
        INFO("The exported msprof db already exists, skip DB export, prof path: %.", profPathStr);
    }
    if ((flags & PIPELINE_CANN_TRACE) != 0 && !isPythonParseComplete)
    {
        const int ret = RunCannTrace(ResolveCannTracePath(profPathStr, cannTracePath));
        if (ret != ANALYSIS_OK)
        {
            return ret;
        }
    }
    if ((flags & PIPELINE_CANN_TRACE) != 0 && isPythonParseComplete)
    {
        INFO("Python parse is complete, skip host data parsing.");
    }
    if ((flags & PIPELINE_DEVICE_DATA) != 0 && !isPythonParseComplete)
    {
        const int ret = RunDeviceData(ResolveDevicePath(profPathStr, devicePath));
        if (ret != ANALYSIS_OK)
        {
            return ret;
        }
    }
    if ((flags & PIPELINE_DEVICE_DATA) != 0 && isPythonParseComplete)
    {
        INFO("Python parse is complete, skip device data parsing.");
    }
    if ((flags & PIPELINE_DB) != 0 && needDbExport)
    {
        INFO("Start DB export, prof path: %.", profPathStr);
        if (RunExport(profPathStr, "", {Analysis::Application::ExportMode::DB}) != ANALYSIS_OK)
        {
            return ANALYSIS_ERROR;
        }
    }
    if ((flags & PIPELINE_DB) != 0)
    {
        needDbExport = false;
    }
    if ((flags & PIPELINE_TIMELINE) != 0)
    {
        const std::string reportsJsonPath = reportsJson == nullptr ? "" : reportsJson;
        const std::set<Analysis::Application::ExportMode> exportModes =
            needDbExport ? std::set<Analysis::Application::ExportMode>{Analysis::Application::ExportMode::TIMELINE,
                                                                       Analysis::Application::ExportMode::DB}
                         : std::set<Analysis::Application::ExportMode>{Analysis::Application::ExportMode::TIMELINE};
        INFO("Start timeline export, prof path: %, export DB: %.", profPathStr, needDbExport ? "true" : "false");
        if (RunExport(profPathStr, reportsJsonPath, exportModes) != ANALYSIS_OK)
        {
            return ANALYSIS_ERROR;
        }
        needDbExport = false;
    }
    if ((flags & PIPELINE_SUMMARY) != 0)
    {
        const std::set<Analysis::Application::ExportMode> exportModes =
            needDbExport ? std::set<Analysis::Application::ExportMode>{Analysis::Application::ExportMode::SUMMARY,
                                                                       Analysis::Application::ExportMode::DB}
                         : std::set<Analysis::Application::ExportMode>{Analysis::Application::ExportMode::SUMMARY};
        INFO("Start summary export, prof path: %, export DB: %.", profPathStr, needDbExport ? "true" : "false");
        if (RunExport(profPathStr, "", exportModes) != ANALYSIS_OK)
        {
            return ANALYSIS_ERROR;
        }
    }
    INFO("C pipeline finished, prof path: %, flags: %.", profPathStr, flagsStr);
    return ANALYSIS_OK;
}
}  // namespace

PyMethodDef g_methodTestSchedule[] = {{"dump_cann_trace", WrapDumpCANNTrace, METH_VARARGS, ""},
                                      {"dump_device_data", WrapDumpDeviceData, METH_VARARGS, ""},
                                      {"export_unified_db", WrapExportUnifiedDB, METH_VARARGS, ""},
                                      {"export_timeline", WrapExportTimeline, METH_VARARGS, ""},
                                      {"export_summary", WrapExportSummary, METH_VARARGS, ""},
                                      {"run_pipeline", WrapRunPipeline, METH_VARARGS, ""},
                                      {NULL, NULL, METH_VARARGS, ""}};

PyMethodDef *GetParserMethods() { return g_methodTestSchedule; };

PyObject *WrapDumpCANNTrace(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录下面的host目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.dump_cann_trace args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.dump_cann_trace path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "..", "mindstudio_profiler_log"}));
    KernelParserWorker parserWorker(parseFilePath);
    auto res = parserWorker.Run();
    return Py_BuildValue("i", res);
}

PyObject *WrapDumpDeviceData(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.dump_device_data args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.dump_device_data path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    const char *stopAt = "";
    DeviceContextEntry(parseFilePath, stopAt);
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapExportUnifiedDB(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF父目录或者PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_unified_db args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_unified_db path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    auto exportManager = Analysis::Application::ExportManager(parseFilePath);
    if (!exportManager.Run({Analysis::Application::ExportMode::DB}))
    {
        ERROR("UnifiedDB run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapExportTimeline(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    const char *reportJsonPath = NULL;
    if (!PyArg_ParseTuple(args, "ss", &parseFilePath, &reportJsonPath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline path is invalid!");
        return NULL;
    }
    if (*reportJsonPath != '\0' && !FileReader::Check(reportJsonPath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline reports json path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    auto exportManager = Analysis::Application::ExportManager(parseFilePath, reportJsonPath);
    if (!exportManager.Run({Analysis::Application::ExportMode::TIMELINE, Analysis::Application::ExportMode::DB}))
    {
        ERROR("Timeline run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapExportSummary(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_summary args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath))
    {
        PyErr_SetString(PyExc_TypeError, "parser.export_summary path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    auto exportManager = Analysis::Application::ExportManager(parseFilePath, "");
    if (!exportManager.Run({Analysis::Application::ExportMode::SUMMARY, Analysis::Application::ExportMode::DB}))
    {
        ERROR("Summary run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapRunPipeline(PyObject *self, PyObject *args)
{
    const char *profPath = NULL;
    const char *cannTracePath = NULL;
    const char *devicePath = NULL;
    const char *reportsJson = NULL;
    int flags = 0;
    if (!PyArg_ParseTuple(args, "zi|zzz", &profPath, &flags, &cannTracePath, &devicePath, &reportsJson))
    {
        PyErr_SetString(PyExc_TypeError, "parser.run_pipeline args parse failed!");
        return NULL;
    }
    return Py_BuildValue("i", RunPipeline(profPath, flags, cannTracePath, devicePath, reportsJson));
}
}  // namespace Interface
}  // namespace Analysis
