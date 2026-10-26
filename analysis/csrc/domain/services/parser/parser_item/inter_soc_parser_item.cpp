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

#include "analysis/csrc/domain/services/parser/parser_item/inter_soc_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

int InterSocParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != STARS_SOC_PROFILE_STRUCT_SIZE)
    {
        ERROR("The InterSoc record size is invalid, actual: %, expected: %", binaryDataSize,
              STARS_SOC_PROFILE_STRUCT_SIZE);
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* target = ReinterpretConvert<HalLogData*>(halUniData);
    auto* source = ReinterpretConvert<InterSocRawData*>(binaryData);
    target->type = SOC_PROFILE_INTER_SOC;
    target->interSoc.sysCnt = source->sysCnt;
    target->interSoc.mataBwLevel = source->bwLevel & 0x7U;                // 低 3 位
    target->interSoc.l2BufferBwLevel = (source->bwLevel & 0x7F8U) >> 3U;  // 第 3~10 位
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, PARSER_ITEM_INTER_SOC, InterSocParserItem);
REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER_V6, PARSER_ITEM_INTER_SOC, InterSocParserItem);

}  // namespace Domain
}  // namespace Analysis
