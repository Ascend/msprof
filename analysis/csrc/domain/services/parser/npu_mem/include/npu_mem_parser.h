/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MEM_NPU_MEM_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MEM_NPU_MEM_PARSER_H

#include <functional>

#include "analysis/csrc/domain/entities/hal/include/hal_npu_mem.h"
#include "analysis/csrc/domain/services/parser/parser.h"

namespace Analysis
{
namespace Domain
{
class NpuMemParser final : public Parser
{
   private:
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
    uint32_t GetParserItemType(const DeviceContext &deviceContext) const;
    bool ParseDataItem(uint8_t *binaryData,
                       const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                       HalNpuMemData &data) const;
    bool ConvertTimestamp(HalNpuMemData &data, const DeviceContext &deviceContext) const;

    std::vector<std::string> filePrefix_{"npu_mem.data.", "npu_mem.app."};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_NPU_MEM_NPU_MEM_PARSER_H
