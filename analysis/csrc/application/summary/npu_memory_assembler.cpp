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
#include "npu_memory_assembler.h"

#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

NpuMemoryAssembler::NpuMemoryAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "event", "ddr(KB)", "hbm(KB)", "memory(KB)", "timestamp(us)"};
}

uint8_t NpuMemoryAssembler::AssembleData(Analysis::Infra::DataInventory &dataInventory)
{
    auto npuMemData = dataInventory.GetPtr<std::vector<NpuMemData>>();
    if (npuMemData == nullptr)
    {
        WARN("No data to export npu memory summary");
        return DATA_NOT_EXIST;
    }
    for (const auto &item : *npuMemData)
    {
        // push data to res
        res_.emplace_back(
            std::vector<std::string>{std::to_string(item.deviceId),
                                     EVENT_MAP.find(item.event) != EVENT_MAP.end() ? EVENT_MAP.at(item.event) : UNKNOWN,
                                     std::to_string(item.ddr / Analysis::Common::BYTE_SIZE),
                                     std::to_string(item.hbm / Analysis::Common::BYTE_SIZE),
                                     std::to_string(item.memory / Analysis::Common::BYTE_SIZE),
                                     DivideByPowersOfTenWithPrecision(item.timestamp) + "\t"});
    }
    if (res_.empty())
    {
        ERROR("Can't match any task data, failed to generate npu_mem_*.csv");
        return ASSEMBLE_FAILED;
    }
    WriteToFile(File::PathJoin({profPath_, Analysis::Common::OUTPUT_PATH, NPU_MEMORY_NAME}), {});
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
