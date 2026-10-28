/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/services/parser/netdev_stats/include/netdev_stats_parser.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_netdev_stats.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
uint64_t GetSliceNumber(const std::string &path)
{
    const size_t position = path.find_last_of('_');
    uint64_t result = 0;
    if (position == std::string::npos || Utils::StrToU64(result, path.substr(position + 1)) != ANALYSIS_OK)
    {
        WARN("Failed to parse NetDev stats slice number from %.", path);
        return 0;
    }
    return result;
}

bool ParseLine(const std::string &line, HalNetDevStatsData &data)
{
    std::istringstream stream(line);
    std::vector<std::string> fields;
    std::string value;
    while (stream >> value)
    {
        fields.emplace_back(value);
    }
    if (fields.size() != NETDEV_STATS_FIELD_COUNT)
    {
        return false;
    }
    for (size_t index = 0; index < fields.size(); ++index)
    {
        if (fields[index].empty() || fields[index][0] == '-' ||
            Utils::StrToU64(data.fields[index], fields[index]) != ANALYSIS_OK || data.fields[index] > INT64_MAX)
        {
            return false;
        }
    }
    return true;
}
}  // namespace

uint32_t NetDevStatsParser::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const std::string dataPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), "data"});
    auto files = Utils::File::GetOriginData(dataPath, {"netdev_stats.data."}, {"done", "complete"});
    std::stable_sort(files.begin(), files.end(), [](const std::string &left, const std::string &right)
                     { return GetSliceNumber(left) < GetSliceNumber(right); });

    std::vector<HalNetDevStatsData> parsedData;
    for (const auto &file : files)
    {
        Utils::FileReader reader(file);
        std::vector<std::string> lines;
        if (reader.ReadText(lines) != ANALYSIS_OK)
        {
            ERROR("Read NetDev stats file failed: %.", file);
            return ANALYSIS_ERROR;
        }
        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
        {
            HalNetDevStatsData data;
            if (!ParseLine(lines[lineIndex], data))
            {
                ERROR("Invalid NetDev stats record in %, line %.", file, lineIndex + 1);
                return ANALYSIS_ERROR;
            }
            parsedData.emplace_back(std::move(data));
        }
    }
    std::shared_ptr<std::vector<HalNetDevStatsData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalNetDevStatsData>, ANALYSIS_ERROR, std::move(parsedData));
    return dataInventory.Inject(data) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

// 暂不启用 NetDev C++ 解析；启用时同步恢复持久化注册及 Python parser 跳过名单。
// REGISTER_PROCESS_SEQUENCE(NetDevStatsParser, false);
// REGISTER_PROCESS_SUPPORT_CHIP(NetDevStatsParser, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0,
//                               CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3);
}  // namespace Domain
}  // namespace Analysis
