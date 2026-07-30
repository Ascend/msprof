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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_AICPU_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_AICPU_PARSER_H

#include <vector>

#include "analysis/csrc/domain/entities/hal/include/aicpu.h"
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis
{
namespace Domain
{
class AicpuParser : public Parser
{
   private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data, uint16_t expandStatus);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
    void SetDeviceAicpuStreamIdMap();
    virtual ParserType GetParserType() const { return AICPU_PARSER; }

   private:
    std::vector<AicpuData> aicpuData_;
    DeviceStreamInfo streamIdMap_;
    std::vector<std::string> filePrefix_{"aicpu.data"};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_AICPU_PARSER_H
