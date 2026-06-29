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
#include "npu_module_mem_assembler.h"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

namespace Analysis
{
namespace Application
{

using namespace Analysis::Domain;
using namespace Analysis::Utils;

// index
const std::vector<std::string> NPU_MODULE_MEM_HEADERS = {"Device_id", "Component", "Timestamp(us)",
                                                         "Total Reserved(KB)", "Device"};

NpuModuleMemAssembler::NpuModuleMemAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    // reserve the module map
    for (const auto &pair : MODULE_NAME_TABLE)
    {
        moduleMap_[pair.second] = pair.first;
    }
}

uint8_t NpuModuleMemAssembler::AssembleData(DataInventory &dataInventory)
{
    auto npuModuleMemData = dataInventory.GetPtr<std::vector<NpuModuleMemData>>();
    // set headers
    headers_ = NPU_MODULE_MEM_HEADERS;
    if (npuModuleMemData == nullptr)
    {
        WARN("No data to export npu module memory summary");
        return DATA_NOT_EXIST;
    }
    // sort
    std::sort(npuModuleMemData->begin(), npuModuleMemData->end(),
              [](const NpuModuleMemData &a, const NpuModuleMemData &b)
              {
                  if (a.moduleId == b.moduleId)
                  {
                      return a.timestamp < b.timestamp;
                  }
                  return a.moduleId < b.moduleId;
              });
    for (auto &item : *npuModuleMemData)
    {
        // push data to res
        res_.emplace_back(std::vector<std::string>{
            std::to_string(item.deviceId),
            moduleMap_.find(item.moduleId) != moduleMap_.end() ? moduleMap_.at(item.moduleId) : UNKNOWN,
            DivideByPowersOfTenWithPrecision(item.timestamp) + "\t",
            std::to_string(item.totalReserved / Analysis::Common::BYTE_SIZE), item.deviceType});
    }
    if (res_.empty())
    {
        ERROR("Can't match any task data, failed to generate npu_module_mem_*.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, NPU_MODULE_MEMORY_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
