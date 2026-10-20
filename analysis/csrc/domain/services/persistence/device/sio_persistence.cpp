/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/services/persistence/device/sio_persistence.h"

#include <tuple>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
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
template <size_t... Index>
auto GenerateRow(const HalSioData& data, double timestamp,
                 IndexSequence<Index...>) -> decltype(std::make_tuple(static_cast<uint32_t>(data.accId),
                                                                      static_cast<uint32_t>(data.bandwidth[Index])...,
                                                                      timestamp))
{
    return std::make_tuple(static_cast<uint32_t>(data.accId), static_cast<uint32_t>(data.bandwidth[Index])...,
                           timestamp);
}

using SioRow = decltype(GenerateRow(std::declval<const HalSioData&>(), std::declval<double>(),
                                    MakeIndexSequence<SIO_BANDWIDTH_COUNT>{}));
using ProcessedData = std::vector<SioRow>;

SioRow GenerateRow(const HalSioData& data, double timestamp)
{
    return GenerateRow(data, timestamp, MakeIndexSequence<SIO_BANDWIDTH_COUNT>{});
}

bool GenerateSioData(const std::vector<HalLogData>& source, const DeviceContext& context, ProcessedData& result)
{
    if (!Reserve(result, source.size()))
    {
        ERROR("Reserve for Sio persistence data failed");
        return false;
    }
    const auto params = context.GetSyscntConversionParams();
    for (const auto& item : source)
    {
        if (item.type != SOC_PROFILE_SIO)
        {
            continue;
        }
        const auto& sio = item.sio;
        result.emplace_back(GenerateRow(sio, GetTimeFromSyscnt(sio.sysCnt, params).Double()));
    }
    return true;
}
}  // namespace

uint32_t SioPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    auto socProfileData = dataInventory.GetPtr<HalSocProfileData>();
    INFO("start c model sio data process");
    if (socProfileData == nullptr)
    {
        ERROR("There is no Sio data to persist");
        return ANALYSIS_ERROR;
    }
    if (socProfileData->items.empty())
    {
        INFO("Sio data is empty, no persistence is required");
        return ANALYSIS_OK;
    }

    ProcessedData data;
    if (!GenerateSioData(socProfileData->items, deviceContext, data))
    {
        return ANALYSIS_ERROR;
    }
    if (data.empty())
    {
        INFO("No Sio data remains after filtering, no persistence is required");
        return ANALYSIS_OK;
    }

    DBInfo sioDB("sio.db", "Sio");
    MAKE_SHARED0_RETURN_VALUE(sioDB.database, SioDB, ANALYSIS_ERROR);
    std::string dbPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, sioDB.dbName});
    MAKE_SHARED_RETURN_VALUE(sioDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(data, sioDB, dbPath))
    {
        ERROR("Save Sio data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(SioPersistence, false, StarsSocProfileParser);
REGISTER_PROCESS_DEPENDENT_DATA(SioPersistence, HalSocProfileData);
REGISTER_PROCESS_SUPPORT_CHIP(SioPersistence, CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V6_1_0);

}  // namespace Domain
}  // namespace Analysis
