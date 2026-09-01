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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/persistence/device/low_power_persistence.h"

#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Application;
using namespace Infra;
using namespace Utils;

namespace
{
constexpr size_t LOW_POWER_BASE_COLUMN_COUNT = 2;
constexpr size_t LOW_POWER_COLUMN_COUNT = LOW_POWER_BASE_COLUMN_COUNT + LOW_POWER_SAMPLE_COUNT;

template <size_t... Index>
auto GenerateRow(const HalLowPowerData& data, double timestamp,
                 IndexSequence<Index...>) -> decltype(std::make_tuple(timestamp, static_cast<uint32_t>(data.dieId),
                                                                      static_cast<uint32_t>(data.sampleData[Index])...))
{
    return std::make_tuple(timestamp, static_cast<uint32_t>(data.dieId),
                           static_cast<uint32_t>(data.sampleData[Index])...);
}

using LowPowerRow = decltype(GenerateRow(std::declval<const HalLowPowerData&>(), std::declval<double>(),
                                         MakeIndexSequence<LOW_POWER_SAMPLE_COUNT>{}));
using ProcessedData = std::vector<LowPowerRow>;

static_assert(std::tuple_size<LowPowerRow>::value == LOW_POWER_COLUMN_COUNT,
              "The LowPower row size must match the expected database column count");

LowPowerRow GenerateRow(const HalLowPowerData& data, double timestamp)
{
    return GenerateRow(data, timestamp, MakeIndexSequence<LOW_POWER_SAMPLE_COUNT>{});
}

bool GenerateLowPowerData(const std::vector<HalSocProfileData>& source, const DeviceContext& context,
                          ProcessedData& result)
{
    if (!Reserve(result, source.size()))
    {
        ERROR("Reserve for LowPower persistence data failed");
        return false;
    }
    const auto params = GenerateSyscntConversionParams(context);
    for (const auto& item : source)
    {
        if (item.type != SOC_PROFILE_LOW_POWER)
        {
            continue;
        }
        const auto& lowPower = item.lowPower;
        result.emplace_back(GenerateRow(lowPower, GetTimeFromSyscnt(lowPower.sysCnt, params).Double()));
    }
    return true;
}
}  // namespace

uint32_t LowPowerPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    auto socProfileData = dataInventory.GetPtr<std::vector<HalSocProfileData>>();
    if (socProfileData == nullptr)
    {
        ERROR("There is no LowPower data to persist");
        return ANALYSIS_ERROR;
    }
    if (socProfileData->empty())
    {
        INFO("LowPower data is empty, no persistence is required");
        return ANALYSIS_OK;
    }

    ProcessedData data;
    if (!GenerateLowPowerData(*socProfileData, deviceContext, data))
    {
        return ANALYSIS_ERROR;
    }
    if (data.empty())
    {
        INFO("No LowPower data remains after filtering, no persistence is required");
        return ANALYSIS_OK;
    }

    DBInfo lowPowerDB("lowpower.db", "LowPower");
    MAKE_SHARED0_RETURN_VALUE(lowPowerDB.database, LowPowerDB, ANALYSIS_ERROR);
    const auto columns = lowPowerDB.database->GetTableCols(lowPowerDB.tableName);
    if (columns.size() != LOW_POWER_COLUMN_COUNT)
    {
        ERROR("The LowPower table column count is invalid, actual: %, expected: %", columns.size(),
              LOW_POWER_COLUMN_COUNT);
        return ANALYSIS_ERROR;
    }
    std::string dbPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, lowPowerDB.dbName});
    MAKE_SHARED_RETURN_VALUE(lowPowerDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(data, lowPowerDB, dbPath))
    {
        ERROR("Save LowPower data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
