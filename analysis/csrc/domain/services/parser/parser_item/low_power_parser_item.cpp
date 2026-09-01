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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/parser/parser_item/low_power_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_soc_profile.h"
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

static_assert(sizeof(LowPowerData) == STARS_SOC_PROFILE_STRUCT_SIZE, "The size of LowPowerData must be 64 bytes");

int LowPowerParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != STARS_SOC_PROFILE_STRUCT_SIZE)
    {
        ERROR("The LowPower record size is invalid, actual: %, expected: %", binaryDataSize,
              STARS_SOC_PROFILE_STRUCT_SIZE);
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* source = ReinterpretConvert<LowPowerData*>(binaryData);
    auto* target = ReinterpretConvert<HalSocProfileData*>(halUniData);
    target->type = SOC_PROFILE_LOW_POWER;
    target->lowPower.dieId = source->dieId;
    target->lowPower.sysCnt = source->sysCnt;
    for (uint32_t i = 0; i < LOW_POWER_SAMPLE_COUNT; ++i)
    {
        target->lowPower.sampleData[i] = source->sampleData[i ^ 1U];
    }
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, PARSER_ITEM_LOW_POWER, LowPowerParserItem);

}  // namespace Domain
}  // namespace Analysis
