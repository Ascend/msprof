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

#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "securec.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

int FreqLpmParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus)
{
    if (binaryDataSize != sizeof(FreqData))
    {
        ERROR("The TrunkSize of Freq is not equal with the FreqData struct");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *binData = ReinterpretConvert<FreqData *>(binaryData);
    auto *targetData = ReinterpretConvert<HalFreqData *>(halUniData);
    uint32_t count = binData->count;
    if (count > FREQ_LPM_DATA_COUNT)
    {
        // Intentionally retain the entries within capacity instead of rejecting the whole record.
        // Continue parsing subsequent records; FreqParser::ParseData summarizes truncation in one WARN.
        count = FREQ_LPM_DATA_COUNT;
    }

    targetData->count = count;
    for (uint32_t i = 0; i < count; ++i)
    {
        targetData->freqLpmDataS[i].sysCnt = binData->lpmDataS[i].sysCnt;
        targetData->freqLpmDataS[i].freq = binData->lpmDataS[i].freq;
    }
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(FREQ_PARSER, DEFAULT_FREQ_LPM, FreqLpmParseItem);
}  // namespace Domain
}  // namespace Analysis
