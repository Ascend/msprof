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

#include <dlfcn.h>
#include <sys/syscall.h>

#include <cstring>
#include <iostream>

#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
using namespace Analysis::Utils;

namespace
{
const mode_t LOG_DIR_MODE = 0750;
const size_t TIME_SIZE = 20;

LogLevel ReadLogLevel()
{
    Dl_info library{};
    if (dladdr(reinterpret_cast<const void *>(&ReadLogLevel), &library) == 0 || library.dli_fname == nullptr)
    {
        PRINT_WARN("Cannot locate the analysis log configuration. Using INFO.");
        return LogLevel::INFO;
    }
    const auto configPath =
        File::PathJoin({File::ParentPath(File::ParentPath(library.dli_fname)), "msconfig", "msprof_config.json"});
    if (!File::Exist(configPath))
    {
        return LogLevel::INFO;
    }
    nlohmann::json config;
    try
    {
        if (!FileReader::Check(configPath) || FileReader(configPath).ReadJson(config) != ANALYSIS_OK ||
            !config.is_object())
        {
            PRINT_WARN("Cannot read the analysis log configuration %. Using INFO.", configPath);
            return LogLevel::INFO;
        }
    }
    catch (const nlohmann::json::exception &)
    {
        PRINT_WARN("Cannot read the analysis log configuration %. Using INFO.", configPath);
        return LogLevel::INFO;
    }
    if (!config.contains("log"))
    {
        return LogLevel::INFO;
    }
    const auto &log = config["log"];
    if (!log.is_object())
    {
        PRINT_WARN("Invalid log.level in %. Expected an integer from 0 to 3. Using INFO.", configPath);
        return LogLevel::INFO;
    }
    const auto value = log.value("level", nlohmann::json(1));
    if (!value.is_number_integer() || value < 0 || value > 3)
    {
        PRINT_WARN("Invalid log.level in %. Expected an integer from 0 to 3. Using INFO.", configPath);
        return LogLevel::INFO;
    }
    return static_cast<LogLevel>(value.get<int>());
}
}  // namespace

int Log::Init(const std::string &logDir)
{
    // Entry points initialize the sink before starting workers.
    if (logDir.empty())
    {
        PRINT_ERROR("Log path is empty.");
        return ANALYSIS_ERROR;
    }
    if (!File::CreateDir(logDir, LOG_DIR_MODE))
    {
        PRINT_ERROR("Create log dir failed.");
        return ANALYSIS_ERROR;
    }
    const auto pid = getpid();
    std::string logName = Format("msprof_analysis_%.log", pid);
    std::string logFile = File::PathJoin({logDir, logName});
    Utils::FileWriter writer;
    writer.Open(logFile, std::ios::out | std::ios::app);
    if (!writer.IsOpen())
    {
        PRINT_ERROR("Log file open failed.");
        return ANALYSIS_ERROR;
    }
    const auto level = ReadLogLevel();
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        logWriter_.Swap(writer);
        pid_ = pid;
        level_.store(level, std::memory_order_relaxed);
    }
    return ANALYSIS_OK;
}

std::string Log::GetFileName(const std::string &path)
{
    return (strrchr(path.c_str(), '/')) ? (strrchr(path.c_str(), '/') + 1) : path;
}

std::string Log::GetTime()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    if (localtime_r(&nowTime, &localTime) == nullptr)
    {
        return "9999-99-99 99:99:99";  // When localtime is abnormal, the default value is '9999-99-99 99:99:99'
    }
    char formatTime[TIME_SIZE];
    std::strftime(formatTime, sizeof(formatTime), "%Y-%m-%d %H:%M:%S", &localTime);
    return static_cast<std::string>(formatTime);
}

void Log::LogMsg(const std::string &message, const std::string &level, const std::string &fileName,
                 const uint32_t &line)
{
    std::ostringstream oss;
    auto tid = syscall(SYS_gettid);
    oss << GetTime() << " " << level << " [" << pid_ << "]" << " [" << tid << "]";
    oss << " [" << fileName << ":" << line << "] " << message << "\n";
    bool writeFailed = false;
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        logWriter_.WriteText(oss.str());
        if (logWriter_.IsOpen() && !logWriter_.IsGood())
        {
            logWriter_.Close();
            writeFailed = true;
        }
    }
    if (writeFailed)
    {
        PRINT_WARN("Cannot write the analysis log. File logging is disabled for this task.");
    }
}

void Log::PrintMsg(const std::string &message, const std::string &level, const std::string &fileName) const
{
    std::ostringstream oss;
    oss << GetTime() << " " << level << " [" << pid_ << "] ";
    oss << fileName << ": " << message << "\n";
    // The Python worker may exit without flushing C++ streams.
    std::cout << oss.str() << std::flush;
}
}  // namespace Analysis
