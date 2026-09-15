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

#include "analysis/csrc/domain/services/persistence/device/pcie_persistence.h"

#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/llc/include/llc_pcie_cpp_enable.h"
#include "analysis/csrc/domain/services/parser/pcie/include/pcie_parser.h"
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
using PcieRows = std::vector<std::tuple<uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;

PcieRows GeneratePcieRows(const std::vector<HalPcieData> &records)
{
    PcieRows rows;
    if (!Utils::Reserve(rows, records.size()))
    {
        return {};
    }
    for (const auto &record : records)
    {
        rows.emplace_back(record.timestamp, record.deviceId, record.txPBandwidthMin, record.txPBandwidthMax,
                          record.txPBandwidthAvg, record.txNpBandwidthMin, record.txNpBandwidthMax,
                          record.txNpBandwidthAvg, record.txCplBandwidthMin, record.txCplBandwidthMax,
                          record.txCplBandwidthAvg, record.txNpLatencyMin, record.txNpLatencyMax, record.txNpLatencyAvg,
                          record.rxPBandwidthMin, record.rxPBandwidthMax, record.rxPBandwidthAvg,
                          record.rxNpBandwidthMin, record.rxNpBandwidthMax, record.rxNpBandwidthAvg,
                          record.rxCplBandwidthMin, record.rxCplBandwidthMax, record.rxCplBandwidthAvg);
    }
    return rows;
}
}  // namespace

uint32_t PciePersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    auto records = dataInventory.GetPtr<std::vector<HalPcieData>>();
    if (records == nullptr)
    {
        ERROR("PCIe data is null.");
        return ANALYSIS_ERROR;
    }
    if (records->empty())
    {
        INFO("There is no PCIe data, don't need to persistence.");
        return ANALYSIS_OK;
    }

    const auto *deviceContext = dynamic_cast<const DeviceContext *>(&context);
    if (deviceContext == nullptr)
    {
        ERROR("PCIe persistence requires DeviceContext.");
        return ANALYSIS_ERROR;
    }
    DBInfo pcieDB("pcie.db", "PcieOriginalData");
    MAKE_SHARED0_RETURN_VALUE(pcieDB.database, Infra::PCIeDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext->GetDeviceFilePath(), "sqlite", pcieDB.dbName});
    MAKE_SHARED_RETURN_VALUE(pcieDB.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    const auto rows = GeneratePcieRows(*records);
    if (rows.size() != records->size())
    {
        ERROR("Generate PcieOriginalData failed.");
        return ANALYSIS_ERROR;
    }
    if (!SaveData(rows, pcieDB, dbPath))
    {
        ERROR("Save PCIe data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    INFO("Process PCIe data done: %", dbPath);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(PciePersistence, true, PcieParser);
// REGISTER_PROCESS_DEPENDENT_DATA(PciePersistence, std::vector<HalPcieData>);
// REGISTER_PROCESS_SUPPORT_CHIP(PciePersistence, CHIP_V1_1_0, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0,
//                               CHIP_V4_1_0, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
