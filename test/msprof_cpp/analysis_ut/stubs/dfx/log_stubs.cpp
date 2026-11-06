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
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

#include <mutex>
#include <utility>

namespace Analysis {
namespace {
std::mutex g_logMutex;
bool g_isLogCaptureEnabled = false;
std::vector<TestLogMessage> g_logMessages;
bool g_isPrintCaptureEnabled = false;
std::vector<TestLogMessage> g_printMessages;
}  // namespace

void StartTestLogCapture()
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    g_logMessages.clear();
    g_isLogCaptureEnabled = true;
}

std::vector<TestLogMessage> StopTestLogCapture()
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    g_isLogCaptureEnabled = false;
    return std::move(g_logMessages);
}

std::string Analysis::Log::GetFileName(std::string const&) {return {};}
void StartTestPrintCapture()
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    g_printMessages.clear();
    g_isPrintCaptureEnabled = true;
}

std::vector<TestLogMessage> StopTestPrintCapture()
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    g_isPrintCaptureEnabled = false;
    return std::move(g_printMessages);
}

void Log::PrintMsg(const std::string& message, const std::string& level, const std::string& fileName) const
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_isPrintCaptureEnabled)
    {
        g_printMessages.push_back({message, level, fileName, 0});
    }
}
void Log::LogMsg(const std::string& message, const std::string &level,
                 const std::string &fileName, const uint32_t &line)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_isLogCaptureEnabled)
    {
        g_logMessages.push_back({message, level, fileName, line});
    }
}
int Log::Init(const std::string &logDir)
{
    return 0;
}
} // namespace Analysis
