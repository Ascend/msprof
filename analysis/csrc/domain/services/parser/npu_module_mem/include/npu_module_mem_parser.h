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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MODULE_MEM_NPU_MODULE_MEM_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MODULE_MEM_NPU_MODULE_MEM_PARSER_H

#include <functional>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_module_mem.h"
#include "analysis/csrc/domain/services/parser/parser.h"

namespace Analysis
{
namespace Domain
{
class NpuModuleMemParser final : public Parser
{
   private:
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
    bool ParseDataItem(uint8_t *binaryData,
                       const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                       HalNpuModuleMemData &data) const;

    std::vector<std::string> filePrefix_{"npu_module_mem.data."};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MODULE_MEM_NPU_MODULE_MEM_PARSER_H
