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
struct adl_serializer<DeviceStartInfo>
{
    static void to_json(json& jsonData, const DeviceStartInfo& infoData)
    {
        jsonData = json{{"clockMonotonicRaw", infoData.clockMonotonicRaw}};
    }
    static void from_json(const json& jsonData, DeviceStartInfo& infoData)
    {
        uint32_t fromJsonResult = static_cast<uint32_t>(ANALYSIS_OK);
        std::string collectionTimeBeginStr = jsonData.at("collectionTimeBegin").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU64(infoData.collectionTimeBegin, collectionTimeBeginStr));
        std::string clockMonotonicRawStr = jsonData.at("clockMonotonicRaw").get<std::string>();
        fromJsonResult |= static_cast<uint32_t>(StrToU64(infoData.clockMonotonicRaw, clockMonotonicRawStr));
        if (fromJsonResult != static_cast<uint32_t>(ANALYSIS_OK))
        {
            ERROR("Get device info from json error! The input: collectionTimeBegin= %, clockMonotonicRaw= %",
                  collectionTimeBeginStr, clockMonotonicRawStr);
            throw std::runtime_error("Get info json failed");
        }
    }
};
}  // namespace nlohmann

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string DEVICE_START_INFO = "start_info";
}

bool DeviceContext::GetStartInfo()
{
    std::vector<std::string> files =
        Analysis::Utils::File::GetOriginData(deviceContextInfo.deviceFilePath, {DEVICE_START_INFO}, {"done"});
    if (files.size() != 1)
    {
        ERROR("The number of % in % is invalid.", DEVICE_START_INFO, deviceContextInfo.deviceFilePath);
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
        deviceContextInfo.startInfo = info;
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
