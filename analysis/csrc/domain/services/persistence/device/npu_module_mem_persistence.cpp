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

#include "analysis/csrc/domain/services/persistence/device/npu_module_mem_persistence.h"

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_module_mem.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/npu_module_mem/include/npu_module_mem_parser.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
namespace
{
std::vector<NpuModuleMemRow> GenerateRows(const std::vector<HalNpuModuleMemData> &data)
{
    std::vector<NpuModuleMemRow> rows;
    rows.reserve(data.size());
    for (const auto &item : data)
    {
        const int64_t totalSize = item.totalSize > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())
                                      ? -1
                                      : static_cast<int64_t>(item.totalSize);
        rows.emplace_back(item.moduleId, item.syscnt, totalSize, item.deviceType);
    }
    return rows;
}
}  // namespace

uint32_t NpuModuleMemPersistence::ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto data = dataInventory.GetPtr<std::vector<HalNpuModuleMemData>>();
    if (data == nullptr)
    {
        INFO("NPU_MODULE_MEM raw data is absent, skip persistence.");
        return ANALYSIS_OK;
    }
    const auto rows = GenerateRows(*data);
    if (rows.empty())
    {
        return ANALYSIS_OK;
    }
    DBInfo dbInfo("npu_module_mem.db", "NpuModuleMem");
    MAKE_SHARED0_RETURN_VALUE(dbInfo.database, Infra::NpuModuleMemDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, dbInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(dbInfo.dbRunner, Infra::DBRunner, ANALYSIS_ERROR, dbPath);
    if (!SaveData(rows, dbInfo, dbPath))
    {
        ERROR("Persist NPU_MODULE_MEM raw data failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
