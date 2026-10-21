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

#include "analysis/csrc/domain/services/parser/parser_item/hbm_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_hbm.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
int HbmV1ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryDataSize != Analysis::HBM_STRUCT_SIZE_V1)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    const auto *record = Utils::ReinterpretConvert<const HbmV1RawData *>(binaryData);
    auto *data = Utils::ReinterpretConvert<HalHbmData *>(halData);
    data->timestamp = record->timestamp;
    data->count = record->count;
    data->eventId = record->eventId;
    data->hbmId = record->hbmId;
    return ANALYSIS_OK;
}

int HbmV2ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryDataSize != Analysis::HBM_STRUCT_SIZE_V2)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    const auto *record = Utils::ReinterpretConvert<const HbmV2RawData *>(binaryData);
    auto *data = Utils::ReinterpretConvert<HalHbmData *>(halData);
    data->eventId = record->eventId;
    data->hbmId = record->hbmId;
    data->count = record->count;
    data->timestamp = record->timestamp;
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(HBM_PARSER, HBM_V1_ITEM_TYPE, HbmV1ParseItem);
REGISTER_PARSER_ITEM(HBM_PARSER, HBM_V2_ITEM_TYPE, HbmV2ParseItem);
}  // namespace Domain
}  // namespace Analysis
