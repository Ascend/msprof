/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/persistence/device/npu_mem_persistence.h"

#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_mem.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/npu_mem/include/npu_mem_parser.h"
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
std::vector<NpuMemRow> GenerateRows(const std::vector<HalNpuMemData> &data)
{
    std::vector<NpuMemRow> rows;
    rows.reserve(data.size());
    for (const auto &item : data)
    {
        rows.emplace_back(std::to_string(item.event), item.ddr, item.hbm, static_cast<double>(item.timestamp),
                          item.ddr + item.hbm);
    }
    return rows;
}
}  // namespace

uint32_t NpuMemPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto data = dataInventory.GetPtr<std::vector<HalNpuMemData>>();
    if (data == nullptr)
    {
        INFO("NPU_MEM raw data is absent, skip persistence.");
        return ANALYSIS_OK;
    }
    const auto rows = GenerateRows(*data);
    if (rows.empty())
    {
        return ANALYSIS_OK;
    }
    DBInfo dbInfo("npu_mem.db", "NpuMem");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::NpuMemDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(rows, dbInfo, dbPath))
    {
        ERROR("Persist NPU_MEM raw data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(NpuMemPersistence, true, NpuMemParser);
REGISTER_PROCESS_DEPENDENT_DATA(NpuMemPersistence, std::vector<HalNpuMemData>);
REGISTER_PROCESS_SUPPORT_CHIP(NpuMemPersistence, CHIP_V4_1_0);
}  // namespace Domain
}  // namespace Analysis
