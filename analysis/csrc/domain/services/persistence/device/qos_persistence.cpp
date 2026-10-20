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

#include "analysis/csrc/domain/services/persistence/device/qos_persistence.h"

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"
#include "analysis/csrc/domain/services/parser/qos/include/qos_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
template <size_t... Index>
QosBwRow MakeQosRow(double timestamp, const HalQosBwData &data, Infra::IndexSequence<Index...>)
{
    return std::make_tuple(timestamp, data.dieId, data.metrics[Index]...);
}

std::vector<QosBwRow> GenerateQosRows(const std::vector<HalQosBwData> &data)
{
    std::vector<QosBwRow> rows;
    rows.reserve(data.size());
    for (const auto &item : data)
    {
        rows.emplace_back(MakeQosRow(item.timestamp, item, Infra::MakeIndexSequence<10>{}));
    }
    return rows;
}

std::vector<QosBwRow> GenerateProfileQosRows(const std::vector<HalLogData> &items,
                                             const Utils::SyscntConversionParams &params)
{
    std::vector<QosBwRow> rows;
    rows.reserve(items.size());
    for (const auto &item : items)
    {
        if (item.type != SOC_PROFILE_QOS)
        {
            continue;
        }
        const double timestamp = Utils::GetTimeFromSyscnt(static_cast<uint64_t>(item.qos.timestamp), params).Double();
        rows.emplace_back(MakeQosRow(timestamp, item.qos, Infra::MakeIndexSequence<10>{}));
    }
    return rows;
}
}  // namespace

uint32_t QosPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto v4Data = dataInventory.GetPtr<std::vector<HalQosBwData>>();
    const auto socProfile = dataInventory.GetPtr<HalSocProfileData>();
    INFO("start c model qos data process");
    if (v4Data == nullptr && socProfile == nullptr)
    {
        ERROR("QoS raw data is null.");
        return ANALYSIS_ERROR;
    }

    std::vector<QosBwRow> rows;
    if (v4Data != nullptr)
    {
        auto v4Rows = GenerateQosRows(*v4Data);
        rows.insert(rows.end(), v4Rows.begin(), v4Rows.end());
    }
    if (socProfile != nullptr)
    {
        const auto params = deviceContext.GetSyscntConversionParams();
        auto profileRows = GenerateProfileQosRows(socProfile->items, params);
        rows.insert(rows.end(), profileRows.begin(), profileRows.end());
    }
    if (rows.empty())
    {
        return ANALYSIS_OK;
    }

    DBInfo dbInfo("qos.db", "QosBwData");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::QosDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(rows, dbInfo, dbPath))
    {
        ERROR("Persist QoS raw data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(QosPersistence, false, QosParser, StarsSocProfileParser);
REGISTER_PROCESS_DEPENDENT_DATA(QosPersistence, std::vector<HalQosBwData>, HalSocProfileData);
REGISTER_PROCESS_SUPPORT_CHIP(QosPersistence, CHIP_V4_1_0, CHIP_V6_1_0);
}  // namespace Domain
}  // namespace Analysis
