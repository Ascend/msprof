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

#ifndef ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H

#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"

namespace Analysis
{
namespace Application
{
class PCIeAssembler : public JsonAssembler
{
   public:
    PCIeAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, const std::string &profPath) override;
    // 仅txNopostLatency单位转为us,其余为带宽 单位转为MB/s
    std::unordered_map<uint16_t, uint32_t> GeneratePCIeTrace(std::vector<PCIeData> &pcieData, uint32_t sortIndex,
                                                             const std::string &profPath);

   private:
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H
