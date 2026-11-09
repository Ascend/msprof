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

#include "analysis/csrc/interface/py_interface/py_init_parser.h"

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/pipeline/unified_pipeline.h"
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
using ExportManager = Analysis::Application::ExportManager;
using ExportMode = Analysis::Application::ExportMode;
using Analysis::Application::IsValidPipelineFlags;
using Analysis::Application::PIPELINE_ALL;
using Analysis::Application::PIPELINE_SUMMARY;
using Analysis::Application::PIPELINE_TIMELINE;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace
{
constexpr int ANALYSIS_INVALID_PARAM = 101;

std::string GetPipelineFlagsString(int flags)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << flags;
    return oss.str();
}

std::string ResolveCannTracePath(const std::string &profPath, const char *cannTracePath)
{
    if (cannTracePath != nullptr && cannTracePath[0] != '\0')
    {
        return cannTracePath;
    }
    const std::string hostPath = File::PathJoin({profPath, "host"});
    return File::CheckDir(hostPath, false) ? hostPath : profPath;
}

std::string ResolveDevicePath(const std::string &profPath, const char *devicePath)
{
    if (devicePath != nullptr && devicePath[0] != '\0')
    {
        return devicePath;
    }
    const std::string hostPath = File::PathJoin({profPath, "host"});
    if (File::CheckDir(hostPath, false))
    {
        return hostPath;
    }
    const std::vector<std::string> deviceDirs = GetDeviceDirectories(profPath);
    return deviceDirs.empty() ? "" : deviceDirs.front();
}

int RunPipeline(const char *profPath, int flags, const char *cannTracePath, const char *devicePath,
                const char *reportsJson)
{
    if (profPath == nullptr || profPath[0] == '\0')
    {
        ERROR("Cannot run unified pipeline: profile path is null or empty, flags: %.", flags);
        return ANALYSIS_INVALID_PARAM;
    }
    if (!IsValidPipelineFlags(flags))
    {
        ERROR("Cannot run unified pipeline: invalid flags %, supported mask: %, profile path: %.", flags, PIPELINE_ALL,
              profPath);
        return ANALYSIS_INVALID_PARAM;
    }
    if ((flags & (PIPELINE_TIMELINE | PIPELINE_SUMMARY)) != 0 && reportsJson != nullptr && reportsJson[0] != '\0' &&
        !FileReader::Check(reportsJson))
    {
        PRINT_ERROR("Cannot run unified pipeline: invalid reports file %, profile path: %.", reportsJson, profPath);
        return ANALYSIS_INVALID_PARAM;
    }
    const std::string profPathStr(profPath);
    if (!File::CheckDir(profPathStr))
    {
        PRINT_ERROR("Cannot run unified pipeline: invalid profile directory %.", profPathStr);
        return ANALYSIS_INVALID_PARAM;
    }
    if (Log::GetInstance().Init(File::PathJoin({profPathStr, "mindstudio_profiler_log"})) != 0)
    {
        ERROR("Init msprof log failed, path is %.", profPathStr);
    }
    Analysis::Application::PipelineRequest request;
    request.profPath = profPathStr;
    request.flags = flags;
    request.hostPath = ResolveCannTracePath(profPathStr, cannTracePath);
    request.devicePath = ResolveDevicePath(profPathStr, devicePath);
    request.reportsJson = reportsJson == nullptr ? "" : reportsJson;
    INFO("Start unified C pipeline, path: %, flags: %.", profPathStr, GetPipelineFlagsString(flags));
    return Analysis::Application::RunUnifiedPipeline(request);
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
    ExportManager manager(parseFilePath);
    return Py_BuildValue("i", manager.Run({ExportMode::DB}) ? ANALYSIS_OK : ANALYSIS_ERROR);
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
    ExportManager manager(parseFilePath, reportJsonPath);
    return Py_BuildValue("i", manager.Run({ExportMode::TIMELINE, ExportMode::DB}) ? ANALYSIS_OK : ANALYSIS_ERROR);
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
    ExportManager manager(parseFilePath);
    return Py_BuildValue("i", manager.Run({ExportMode::SUMMARY, ExportMode::DB}) ? ANALYSIS_OK : ANALYSIS_ERROR);
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
