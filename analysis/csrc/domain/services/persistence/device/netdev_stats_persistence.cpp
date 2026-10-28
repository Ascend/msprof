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

#include "analysis/csrc/domain/services/persistence/device/netdev_stats_persistence.h"

#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_netdev_stats.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/netdev_stats/include/netdev_stats_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/file.h"

namespace Analysis
{
namespace Domain
{
namespace
{
using NetDevStatsRow =
    std::tuple<uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
               uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;

NetDevStatsRow MakeRow(uint32_t deviceId, const HalNetDevStatsData &data)
{
    return std::make_tuple(deviceId, data.fields[0], data.fields[1], data.fields[2], data.fields[3], data.fields[4],
                           data.fields[5], data.fields[6], data.fields[7], data.fields[8], data.fields[9],
                           data.fields[10], data.fields[11], data.fields[12], data.fields[13], data.fields[14],
                           data.fields[15]);
}
}  // namespace

uint32_t NetDevStatsPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto data = dataInventory.GetPtr<std::vector<HalNetDevStatsData>>();
    if (data == nullptr)
    {
        ERROR("NetDev stats raw data is null.");
        return ANALYSIS_ERROR;
    }
    if (data->empty())
    {
        return ANALYSIS_OK;
    }
    DeviceInfo deviceInfo;
    deviceContext.Getter(deviceInfo);
    std::vector<NetDevStatsRow> rows;
    rows.reserve(data->size());
    for (const auto &item : *data)
    {
        rows.emplace_back(MakeRow(deviceInfo.deviceId, item));
    }

    DBInfo dbInfo("netdev_stats.db", "NetDevStatsOriginalData");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::NetDevStatsDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(rows, dbInfo, dbPath))
    {
        ERROR("Persist NetDev stats original data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

// 暂不启用 NetDev C++ 持久化，避免与 Python 重复写入。
// REGISTER_PROCESS_SEQUENCE(NetDevStatsPersistence, false, NetDevStatsParser);
// REGISTER_PROCESS_DEPENDENT_DATA(NetDevStatsPersistence, std::vector<HalNetDevStatsData>);
// REGISTER_PROCESS_SUPPORT_CHIP(NetDevStatsPersistence, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0,
// CHIP_V3_3_0,
//                               CHIP_V4_1_0, CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3);
}  // namespace Domain
}  // namespace Analysis
