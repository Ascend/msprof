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

#include "analysis/csrc/domain/services/persistence/device/ub_persistence.h"

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_ub.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/ub/include/ub_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
namespace
{
template <size_t... Index>
UbBwRow MakeUbRow(const HalUbBwData &data, Infra::IndexSequence<Index...>)
{
    return std::make_tuple(data.deviceId, data.portId, data.timestamp, data.metrics[Index]...);
}

std::vector<UbBwRow> GenerateUbRows(const std::vector<HalUbBwData> &data)
{
    std::vector<UbBwRow> rows;
    rows.reserve(data.size());
    for (const auto &item : data)
    {
        rows.emplace_back(MakeUbRow(item, Infra::MakeIndexSequence<14>{}));
    }
    return rows;
}
}  // namespace

uint32_t UbPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto data = dataInventory.GetPtr<std::vector<HalUbBwData>>();
    if (data == nullptr)
    {
        ERROR("UB raw data is null.");
        return ANALYSIS_ERROR;
    }
    const auto rows = GenerateUbRows(*data);
    if (rows.empty())
    {
        return ANALYSIS_OK;
    }
    DBInfo dbInfo("ub.db", "UBBwData");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::UbDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(rows, dbInfo, dbPath))
    {
        ERROR("Persist UB raw data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(UbPersistence, false, UbParser);
// REGISTER_PROCESS_DEPENDENT_DATA(UbPersistence, std::vector<HalUbBwData>);
// REGISTER_PROCESS_SUPPORT_CHIP(UbPersistence, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
