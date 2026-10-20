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

#include "analysis/csrc/domain/services/persistence/device/inter_soc_persistence.h"

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
// InterSoc 表列顺序：l2_buffer_bw_level, mata_bw_level, sys_time
using InterSocRow = std::tuple<uint32_t, uint32_t, double>;
using ProcessedData = std::vector<InterSocRow>;

bool GenerateInterSocData(const std::vector<HalLogData>& source, const DeviceContext& context, ProcessedData& result)
{
    if (!Reserve(result, source.size()))
    {
        ERROR("Reserve for InterSoc persistence data failed");
        return false;
    }
    const auto params = context.GetSyscntConversionParams();
    for (const auto& item : source)
    {
        if (item.type != SOC_PROFILE_INTER_SOC)
        {
            continue;
        }
        const auto& interSoc = item.interSoc;
        result.emplace_back(interSoc.l2BufferBwLevel, interSoc.mataBwLevel,
                            GetTimeFromSyscnt(interSoc.sysCnt, params).Double());
    }
    return true;
}
}  // namespace

uint32_t InterSocPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    auto socProfileData = dataInventory.GetPtr<HalSocProfileData>();
    INFO("start c model inter soc data process");
    if (socProfileData == nullptr)
    {
        ERROR("There is no InterSoc data to persist");
        return ANALYSIS_ERROR;
    }
    if (socProfileData->items.empty())
    {
        INFO("InterSoc data is empty, no persistence is required");
        return ANALYSIS_OK;
    }

    ProcessedData data;
    if (!GenerateInterSocData(socProfileData->items, deviceContext, data))
    {
        return ANALYSIS_ERROR;
    }
    if (data.empty())
    {
        INFO("No InterSoc data remains after filtering, no persistence is required");
        return ANALYSIS_OK;
    }

    DBInfo interSocDB("soc_profiler.db", "InterSoc");
    MAKE_SHARED0_RETURN_VALUE(interSocDB.database, SocProfilerDB, ANALYSIS_ERROR);
    std::string dbPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, interSocDB.dbName});
    MAKE_SHARED_RETURN_VALUE(interSocDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(data, interSocDB, dbPath))
    {
        ERROR("Save InterSoc data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(InterSocPersistence, false, StarsSocProfileParser);
REGISTER_PROCESS_DEPENDENT_DATA(InterSocPersistence, HalSocProfileData);
REGISTER_PROCESS_SUPPORT_CHIP(InterSocPersistence, CHIP_V4_1_0);

}  // namespace Domain
}  // namespace Analysis
