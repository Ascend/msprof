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
#include <iostream>
#include <sstream>

#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "device_context.h"
#include "device_context_error_code.h"
#include "nlohmann/json.hpp"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace Analysis
{

namespace Domain
{
namespace
{
const std::string SAMPLE_JSON = "sample.json";
const char EVENT_SEPARATOR = ',';
}  // namespace

// 分割字符串，提取十六进制数
void HexStrToInt(std::string& jsonStr, std::vector<uint32_t>& intValues)
{
    std::stringstream ss(jsonStr);
    int index = 0;
    while (!ss.eof())
    {
        char comma;
        uint32_t value;
        ss >> std::hex >> value;
        if (ss.fail())
        {
            ERROR("The MetricEventsStr error, please check!");
            break;
        }
        if (ss.peek() == EVENT_SEPARATOR)
        {
            ss >> comma;
        }
        if (index < DEFAULT_PMU_LENGTH)
        {
            intValues[index] = value;
        }
        else
        {
            intValues.push_back(value);
        }
        ++index;
    }
}

AivMetricsEventsType GetAivMetricsEventsTypeFromStr(const std::string& aivMetrics)
{
    auto it = aivMetricsMap.find(aivMetrics);
    if (it != aivMetricsMap.end())
    {
        return it->second;
    }
    else
    {
        // 如果找不到对应的枚举值，返回AicMetricsUnknown
        return AivMetricsEventsType::AIV_METRICS_UNKNOWN;
    }
}

AicMetricsEventsType GetAicMetricsEventsTypeFromStr(const std::string& aicMetrics)
{
    auto it = aicMetricsMap.find(aicMetrics);
    if (it != aicMetricsMap.end())
    {
        return it->second;
    }
    else
    {
        // 如果找不到对应的枚举值，返回AicMetricsUnknown
        return AicMetricsEventsType::AIC_METRICS_UNKNOWN;
    }
}

ProfilingMode GetProfilingModeFromStr(const std::string& profilingStr)
{
    auto it = profilingMap.find(profilingStr);
    if (it != profilingMap.end())
    {
        return it->second;
    }
    else
    {
        // 如果找不到对应的枚举值，PROFILING_UNKNOWN
        return ProfilingMode::PROFILING_UNKNOWN;
    }
}

}  // namespace Domain

}  // namespace Analysis

namespace nlohmann
{

template <>
struct adl_serializer<SampleInfo>
{
    static void to_json(json& jsonData, const SampleInfo& infoData)
    {
        jsonData = json{{"ai_core_profiling", infoData.aiCoreProfiling}, {"ai_core_metrics", infoData.aiCoreMetrics}};
    }
    static void from_json(const json& jsonData, SampleInfo& infoData)
    {
        std::string aiCoreProfilingStr = jsonData.at("ai_core_profiling").get<std::string>();
        infoData.aiCoreProfiling = (aiCoreProfilingStr == "on");

        std::string aiCoreMetricsStr = jsonData.at("ai_core_metrics").get<std::string>();
        infoData.aiCoreMetrics = GetAicMetricsEventsTypeFromStr(aiCoreMetricsStr);

        jsonData.at("ai_core_profiling_events").get_to(infoData.aiCoreProfilingEventsStr);
        if (infoData.aiCoreProfilingEventsStr != "")
        {
            HexStrToInt(infoData.aiCoreProfilingEventsStr, infoData.aiCoreProfilingEvents);
        }

        std::string profilingStr = jsonData.at("ai_core_profiling_mode").get<std::string>();
        infoData.aiCoreProfilingMode = GetProfilingModeFromStr(profilingStr);

        jsonData.at("aicore_sampling_interval").get_to(infoData.aiCoreSamplingInterval);

        std::string aivProfilingStr = jsonData.at("aiv_profiling").get<std::string>();
        infoData.aivProfiling = (aivProfilingStr == "on");

        std::string aivMetricsStr = jsonData.at("aiv_metrics").get<std::string>();
        infoData.aivMetrics = GetAivMetricsEventsTypeFromStr(aivMetricsStr);

        jsonData.at("aiv_profiling_events").get_to(infoData.aivProfilingEventsStr);
        if (infoData.aivProfilingEventsStr != "")
        {
            HexStrToInt(infoData.aivProfilingEventsStr, infoData.aivProfilingEvents);
        }

        std::string aivProfilingMode = jsonData.at("aiv_profiling_mode").get<std::string>();
        infoData.aivProfilingMode = GetProfilingModeFromStr(aivProfilingMode);

        jsonData.at("aiv_sampling_interval").get_to(infoData.aivSamplingInterval);
        std::string dynamicStr = jsonData.value("dynamic", "off");
        infoData.dynamic = (dynamicStr == "on");

        std::string profLevel = jsonData.value("profLevel", "l0");
        std::string profLevelHisi = jsonData.value("prof_level", "level0");
        infoData.isLevel0 = (profLevel == "l0" && profLevelHisi == "level0");  // 同时为level0时说明是level0
    }
};

}  // namespace nlohmann

bool DeviceContext::GetSampleJson()
{
    std::vector<std::string> files = File::GetOriginData(deviceContextInfo.deviceFilePath, {SAMPLE_JSON}, {"done"});
    if (files.size() != 1)
    {
        ERROR("The number of % in % is invalid.", SAMPLE_JSON, deviceContextInfo.deviceFilePath);
        return false;
    }

    FileReader fd(files.back());
    nlohmann::json info;
    if (fd.ReadJson(info) != ANALYSIS_OK)
    {
        ERROR("Load json context failed: '%'.", files.back());
        return false;
    }

    try
    {
        deviceContextInfo.sampleInfo = info;
    }
    catch (const std::exception& e)
    {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }

    return true;
}
