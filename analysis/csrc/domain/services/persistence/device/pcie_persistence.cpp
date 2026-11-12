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

#include "analysis/csrc/domain/services/persistence/device/pcie_persistence.h"

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_llc_pcie.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
using PcieRows = std::vector<std::tuple<uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
// Unique stage names allow concurrent parses of the same profile without sharing temporary rows.
std::string MakePcieStageTableName()
{
    static std::atomic<uint64_t> sequence{0};
    const auto tick = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return "PcieOriginalDataStage_" + std::to_string(getpid()) + "_" + std::to_string(tick) + "_" +
           std::to_string(sequence.fetch_add(1));
}

bool SavePcieData(const std::string &dbPath, const PcieRows &rows)
{
    // Stage PCIe rows before replacing the single table in a transaction, preserving old rows on failure.
    // LlcPersistence replaces three related tables by dropping and recreating them instead.
    Infra::PCIeDB database;
    Infra::DBRunner dbRunner(dbPath);
    const std::string stageTable = MakePcieStageTableName();
    const auto columns = database.GetTableCols("PcieOriginalData");
    std::vector<std::string> columnNames;
    columnNames.reserve(columns.size());
    for (const auto &column : columns)
    {
        columnNames.emplace_back(column.name);
    }
    const std::string columnList = Utils::Join(columnNames, ", ");
    if (!dbRunner.CreateTable("PcieOriginalData", columns) || !dbRunner.CreateTable(stageTable, columns))
    {
        return false;
    }
    if (!dbRunner.InsertData(stageTable, rows))
    {
        if (!dbRunner.DropTable(stageTable))
        {
            ERROR("Failed to clean up PCIe stage table % in %", stageTable, dbPath);
        }
        return false;
    }

    const std::string replaceSql =
        "BEGIN IMMEDIATE; DELETE FROM PcieOriginalData; "
        "INSERT INTO PcieOriginalData (" +
        columnList + ") SELECT " + columnList + " FROM " + stageTable + "; DROP TABLE " + stageTable + "; COMMIT;";
    if (!dbRunner.UpdateData(replaceSql))
    {
        // UpdateData closes its connection on failure, rolling back the open transaction.
        // A separate DBRunner call cannot issue ROLLBACK on that connection.
        if (!dbRunner.DropTable(stageTable))
        {
            ERROR("Failed to clean up PCIe stage table % in %", stageTable, dbPath);
        }
        return false;
    }
    return true;
}

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
    const std::string dbPath = Utils::File::PathJoin({deviceContext->GetDeviceFilePath(), "sqlite", "pcie.db"});
    const auto rows = GeneratePcieRows(*records);
    if (rows.size() != records->size())
    {
        ERROR("Generate PcieOriginalData failed.");
        return ANALYSIS_ERROR;
    }
    if (!SavePcieData(dbPath, rows))
    {
        ERROR("Save PCIe data failed: %", dbPath);
        return ANALYSIS_ERROR;
    }
    INFO("Process PCIe data done: %", dbPath);
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
