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
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "test/msprof_cpp/analysis_ut/stubs/dfx/log_stubs.h"

#include <mutex>
#include <utility>

namespace Analysis {
namespace {
std::mutex g_logMutex;
bool g_isLogCaptureEnabled = false;
std::vector<TestLogMessage> g_logMessages;
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
void Analysis::Log::PrintMsg(std::string const&, std::string const&, std::string const&) const {}
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
