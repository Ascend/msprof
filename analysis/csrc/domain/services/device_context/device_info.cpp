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
struct adl_serializer<DeviceInfo>
{
    static void to_json(json& jsonData, const DeviceInfo& infoData)
    {
        jsonData = json{{"platform_version", infoData.chipID}, {"devices", infoData.deviceId}};
    }
    static void from_json(const json& jsonData, DeviceInfo& infoData)
    {
        uint32_t fromJsonResult = static_cast<uint32_t>(ANALYSIS_OK);
        std::string platformVersionStr = jsonData.at("platform_version").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU32(infoData.chipID, platformVersionStr));

        std::string deviceStr = jsonData.at("devices").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU32(infoData.deviceId, deviceStr));

        jsonData.at("DeviceInfo").at(0).at("ai_core_num").get_to(infoData.aiCoreNum);

        std::string aicFrequencyStr = jsonData.at("DeviceInfo").at(0).at("aic_frequency").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU32(infoData.aicFrequency, aicFrequencyStr));

        jsonData.at("DeviceInfo").at(0).at("aiv_num").get_to(infoData.aivNum);

        std::string aivFrequencyStr = jsonData.at("DeviceInfo").at(0).at("aiv_frequency").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU32(infoData.aivFrequency, aivFrequencyStr));

        std::string hwtsFrequencyStr = jsonData.at("DeviceInfo").at(0).at("hwts_frequency").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToDouble(infoData.hwtsFrequency, hwtsFrequencyStr));
        infoData.collectionVersion = jsonData.value("version", NA);
        if (fromJsonResult != static_cast<uint32_t>(ANALYSIS_OK) || IsDoubleEqual(infoData.hwtsFrequency, 0.0))
        {
            ERROR(
                "Get device info from json error! The input: platform_version= %, devices= %, aic_frequency= %, "
                "aiv_frequency= %, hwts_frequency=%",
                platformVersionStr, deviceStr, aicFrequencyStr, aivFrequencyStr, hwtsFrequencyStr);
            throw std::runtime_error("Get info json failed");
        }
    }
};

}  // namespace nlohmann

namespace Analysis
{

namespace Domain
{

const std::string INFO_JSON = "info.json";

bool DeviceContext::GetInfoJson()
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
        deviceContextInfo.deviceInfo = info;
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
