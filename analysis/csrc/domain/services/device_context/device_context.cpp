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
#include "device_context.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "device_context_error_code.h"
#include "nlohmann/json.hpp"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Infra;

namespace Analysis
{

namespace Domain
{
DeviceContext &DeviceContext::Instance()
{
    thread_local DeviceContext ins;
    return ins;
}

bool DeviceContext::Init(const std::string &devicePath)
{
    if (!this->isInitialized_)
    {
        this->deviceContextInfo.deviceFilePath = devicePath;
        std::vector<std::function<bool()>> funcList = {
            [this]() { return this->GetInfoJson(); },    [this]() { return this->GetCpuInfo(); },
            [this]() { return this->GetSampleJson(); },  [this]() { return this->GetHostStart(); },
            [this]() { return this->GetDeviceStart(); }, [this]() { return this->GetStartInfo(); }};
        auto ret = std::all_of(funcList.begin(), funcList.end(), [](std::function<bool()> func) { return func(); });
        this->isInitialized_ = ret;  // 标记已初始化
        return ret;
    }
    return true;
}

std::vector<std::string> GetDeviceDirectories(const std::string &path)
{
    std::vector<std::string> subdirs;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr)
    {
        ERROR("Error opening directory: %, errorCode: %, errorInfo: %", path, errno, strerror(errno));
        return subdirs;
    }
    struct dirent *entry;
    std::string subdirPath;
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string subdirName = entry->d_name;
        if (subdirName == "." || subdirName == "..")
        {
            continue;
        }
        subdirPath = File::PathJoin({path, subdirName});
        struct stat fileStat;
        if (lstat(subdirPath.c_str(), &fileStat) == -1)
        {
            ERROR("The % file lstat failed. The Error code is %", subdirName, strerror(errno));
            continue;
        }
        if (S_ISDIR(fileStat.st_mode) && subdirName.find("device") == 0)
        {
            subdirs.push_back(subdirPath);
        }
    }
    closedir(dir);
    return subdirs;
}

std::vector<DataInventory> DeviceContextEntry(const char *targetDir, const char *stopAt)
{
    Utils::TimeLogger t{"DeviceContextEntry "};
    std::vector<std::string> subdirs = GetDeviceDirectories(targetDir);
    std::vector<DataInventory> processDataVec(subdirs.size());
    std::vector<std::string> processStats(subdirs.size());
    if (subdirs.empty())
    {
        WARN("No valid device directory, the file name should start with 'device'.");
        return processDataVec;
    }

    std::function<void()> func;
    // 单个 device 失败只会写进它自己的 processStat，返回的 processDataVec 里那一项仍是
    // 默认构造的空 DataInventory，调用方无法从 vector 长度区分成功与失败（两个调用点
    // 还都丢弃了返回值）。这里额外累计一个失败数，在最后汇总打印，便于快速定位。
    std::atomic<size_t> failedNum{0};

    ThreadPool tp(subdirs.size());
    size_t i = 0;
    for (const auto &subdir : subdirs)
    {
        auto &processStat = processStats[i];
        auto &processData = processDataVec[i];
        ++i;
        func = [subdir, &processStat, &processData, stopAt, &failedNum]
        {
            DeviceContext &context = DeviceContext::Instance();
            if (!context.Init(subdir))
            {
                processStat = "Init failed, exit!";
                ++failedNum;
                return;
            }
            std::string sqlitePath = File::PathJoin({subdir, Common::SQLITE});
            if (!File::CreateDir(sqlitePath))
            {
                ERROR("Create device sqlite dir failed, path is %.", sqlitePath);
                processStat = "Create sqlite dir failed, exit!";
                ++failedNum;
                return;
            }
            if (stopAt != nullptr)
            {
                context.SetStopAt(stopAt);
            }

            auto regInfo = ProcessRegister::CopyProcessInfo();
            ProcessControl processControl(regInfo);

            bool ret = processControl.ExecuteProcess(processData, context);

            auto stat = processControl.GetExecuteStat();
            RecordProcessStat(stat, subdir, processStat);
        };
        tp.AddTask(func);
    }
    tp.Start();
    tp.WaitAllTasks();
    tp.Stop();

    for (const auto &stat : processStats)
    {
        INFO("stat info: %", stat);
    }
    if (failedNum.load() > 0)
    {
        ERROR("% of % device dir(s) failed, see stat info above", failedNum.load(), subdirs.size());
    }
    return processDataVec;
}
}  // namespace Domain
}  // namespace Analysis
