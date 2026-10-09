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
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "device_context.h"
#include "device_context_error_code.h"
#include "nlohmann/json.hpp"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace nlohmann
{

template <>
struct adl_serializer<CpuInfo>
{
    static void to_json(json& jsonData, const CpuInfo& infoData) { jsonData = json{{"Frequency", infoData.frequency}}; }
    static void from_json(const json& jsonData, CpuInfo& infoData)
    {
        std::string frequencyStr = jsonData.at("CPU").at(0).at("Frequency").get<std::string>();
        if (!frequencyStr.empty())
        {
            if (StrToDouble(infoData.frequency, frequencyStr) != ANALYSIS_OK)
            {
                ERROR("Get frequency failed, the input string is '%'.", frequencyStr);
            }
        }
    }
};
}  // namespace nlohmann

namespace Analysis
{
namespace Domain
{

const std::string INFO_JSON = "info.json";

bool DeviceContext::GetCpuInfo()
{
    std::vector<std::string> files =
        Analysis::Utils::File::GetOriginData(deviceContextInfo.deviceFilePath, {INFO_JSON}, {"done"});
    if (files.size() != 1)
    {
        ERROR("The number of % in % is invalid.", INFO_JSON, deviceContextInfo.deviceFilePath);
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
        deviceContextInfo.cpuInfo = info;
    }
    catch (const std::exception& e)
    {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }
    return true;
}

}  // namespace Domain

}  // namespace Analysis
