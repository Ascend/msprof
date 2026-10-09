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

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/device_context_error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;
const std::string HOST_START_LOG = "host_start.log";
const std::string DEVICE_START_LOG = "dev_start.log";

template <typename T>
bool SerializeStartConfig(T& info, std::unordered_map<std::string, std::string>& startConfig)
{
    if (StrToU64(info.clockMonotonicRaw, startConfig["clock_monotonic_raw"]) != ANALYSIS_OK)
    {
        ERROR("DeviceMonotonic to uint64_t failed, invalid str is %.", startConfig["clock_monotonic_raw"]);
        return false;
    }
    if (StrToU64(info.cntVct, startConfig["cntvct"]) != ANALYSIS_OK)
    {
        ERROR("SysCnt to uint64_t failed, invalid str is %.", startConfig["cntvct"]);
        return false;
    }
    if (typeid(info) == typeid(HostStartLog))
    {
        if (StrToU64(info.cntVctDiff, startConfig["cntvct_diff"]) != ANALYSIS_OK)
        {
            ERROR("SysCnt to uint64_t failed, invalid str is %.", startConfig["cntvct_diff"]);
            return false;
        }
    }
    return true;
}

std::unordered_map<std::string, std::string> GetConfig(std::vector<std::string>& files, std::string& filePath,
                                                       const std::string& filePattern)
{
    std::unordered_map<std::string, std::string> config;
    const int expectTokenSize = 2;
    if (files.size() != 1)
    {
        ERROR("The number of % in % is invalid.", filePattern, filePath);
        return config;
    }

    FileReader fd(files.back());
    std::vector<std::string> text;
    if (fd.ReadText(text) != ANALYSIS_OK)
    {
        ERROR("Load log text failed: '%'.", files.back());
        return config;
    }

    for (const auto& line : text)
    {
        auto tokens = Utils::Split(line, ":");
        if (tokens.size() != expectTokenSize)
        {
            continue;
        }
        config[tokens[0]] = tokens[1];
    }
    return config;
}

bool DeviceContext::GetDeviceStart()
{
    std::vector<std::string> files =
        File::GetOriginData(deviceContextInfo.deviceFilePath, {DEVICE_START_LOG}, {"done"});
    auto deviceStartConfig = GetConfig(files, deviceContextInfo.deviceFilePath, DEVICE_START_LOG);
    return SerializeStartConfig(deviceContextInfo.deviceStart, deviceStartConfig);
}

bool DeviceContext::GetHostStart()
{
    auto files = File::GetOriginData(deviceContextInfo.deviceFilePath, {HOST_START_LOG}, {"done"});
    auto hostStartConfig = GetConfig(files, deviceContextInfo.deviceFilePath, HOST_START_LOG);
    return SerializeStartConfig(deviceContextInfo.hostStartLog, hostStartConfig);
}
}  // namespace Domain
}  // namespace Analysis
