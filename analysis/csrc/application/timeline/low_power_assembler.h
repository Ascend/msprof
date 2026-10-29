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
#ifndef ANALYSIS_APPLICATION_LOW_POWER_ASSEMBLER_H
#define ANALYSIS_APPLICATION_LOW_POWER_ASSEMBLER_H

#include <set>
#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/low_power_data.h"

namespace Analysis
{
namespace Application
{
class LowPowerAssembler : public JsonAssembler
{
   public:
    LowPowerAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, const std::string &profPath) override;
    std::unordered_map<uint16_t, uint32_t> GenerateFreqTrace(std::vector<LowPowerData> &freqData, uint32_t sortIndex,
                                                             const std::string &profPath);

   private:
};
}  // namespace Application
}  // namespace Analysis
#endif  // ANALYSIS_APPLICATION_LOW_POWER_ASSEMBLER_H
