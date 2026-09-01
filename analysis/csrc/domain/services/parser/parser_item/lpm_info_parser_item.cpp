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

#include "analysis/csrc/domain/services/parser/parser_item/lpm_info_parser_item.h"

#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

namespace
{
bool IsValidLpmInfoType(uint32_t type) { return type <= static_cast<uint32_t>(LpmInfoType::BUS_VOLTAGE); }
}  // namespace

int LpmInfoParseItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != sizeof(LpmInfoRawData))
    {
        ERROR("The TrunkSize of LpmInfo is not equal with the LpmInfoRawData struct");
        return ANALYSIS_ERROR;
    }
    auto* binData = ReinterpretConvert<LpmInfoRawData*>(binaryData);
    if (binData->count > LPM_INFO_DATA_COUNT)
    {
        ERROR("The count of LpmInfo data exceeds the maximum, count is %", binData->count);
        return ANALYSIS_ERROR;
    }
    if (!IsValidLpmInfoType(binData->type))
    {
        ERROR("The type of LpmInfo data is invalid, type is %", binData->type);
        return ANALYSIS_ERROR;
    }

    auto* targetData = ReinterpretConvert<HalLpmInfoRecord*>(halUniData);
    targetData->count = binData->count;
    targetData->type = static_cast<LpmInfoType>(binData->type);
    for (uint32_t i = 0; i < binData->count; ++i)
    {
        targetData->lpmDataS[i].sysCnt = binData->lpmDataS[i].sysCnt;
        targetData->lpmDataS[i].value = binData->lpmDataS[i].value;
    }
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(LPM_INFO_PARSER, DEFAULT_LPM_INFO, LpmInfoParseItem);
}  // namespace Domain
}  // namespace Analysis
