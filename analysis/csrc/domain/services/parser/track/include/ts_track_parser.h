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

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H

#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"

namespace Analysis
{
namespace Domain
{
class TsTrackParser : public Parser
{
   private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data, ParserType parserType,
                           uint16_t expandStatus);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

   private:
    std::vector<HalTrackData> halUniData_;
    std::vector<std::string> filePrefix_{"ts_track."};
};
}  // namespace Domain
}  // namespace Analysis
#endif  // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H
