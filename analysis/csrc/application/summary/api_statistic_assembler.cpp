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

#include "analysis/csrc/application/summary/api_statistic_assembler.h"

#include <algorithm>
#include <string>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Domain;
using namespace Analysis::Application;
namespace
{
struct DataKey
{
    uint16_t level;
    std::string apiName;

    bool operator<(const DataKey &other) const
    {
        if (apiName != other.apiName)
        {
            return apiName < other.apiName;
        }
        return level < other.level;
    }
};

std::string GetLevel(uint16_t level)
{
    for (const auto &node : API_LEVEL_TABLE)
    {
        if (node.second == level)
        {
            return node.first;
        }
    }
    return "";
}
}  // namespace

ApiStatisticAssembler::ApiStatisticAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "Level", "API Name", "Time(us)", "Count", "Avg(us)", "Min(us)", "Max(us)", "Variance"};
}

void ApiStatisticAssembler::GenerateApiStatistic(std::vector<ApiData> &apiData)
{
    std::map<DataKey, std::vector<double>> apiDurationMap;
    // 将具有相同的level和apiName的duration放在一起
    for (const auto &data : apiData)
    {
        double duration{0.0};
        StrToDouble(duration, DivideByPowersOfTenWithPrecision(data.end - data.timestamp));
        DataKey key{data.level, data.apiName};
        apiDurationMap[key].emplace_back(RoundToDecimalPlaces(duration));
    }

    for (const auto &it : apiDurationMap)
    {
        double sumValue{0.0};
        auto maxDurationValue = *(std::max_element(it.second.begin(), it.second.end()));
        auto minDurationValue = *(std::min_element(it.second.begin(), it.second.end()));
        for (const auto &dur : it.second)
        {
            sumValue += dur;
        }
        auto avg = RoundToDecimalPlaces(sumValue / it.second.size());
        // 计算方差
        double variance{0.0};
        for (const auto &dur : it.second)
        {
            variance += (dur - avg) * (dur - avg);
        }
        variance = RoundToDecimalPlaces(variance / it.second.size());
        std::vector<std::string> row = {"host",
                                        GetLevel(it.first.level),
                                        it.first.apiName,
                                        DoubleToStr(sumValue),
                                        std::to_string(it.second.size()),
                                        DoubleToStr(avg),
                                        DoubleToStr(minDurationValue),
                                        DoubleToStr(maxDurationValue),
                                        DoubleToStr(variance)};
        res_.emplace_back(row);
    }
}

uint8_t ApiStatisticAssembler::AssembleData(DataInventory &dataInventory)
{
    auto apiStatisticData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiStatisticData == nullptr)
    {
        WARN("Api statistic data not exist.");
        return DATA_NOT_EXIST;
    }
    GenerateApiStatistic(*apiStatisticData);
    std::string fileName = File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, API_STATISTIC_NAME});
    WriteToFile(fileName, {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
