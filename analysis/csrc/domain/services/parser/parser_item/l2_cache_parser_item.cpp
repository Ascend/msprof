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

#include "analysis/csrc/domain/services/parser/parser_item/l2_cache_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
int L2CacheParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryData == nullptr || halData == nullptr || binaryDataSize != sizeof(HalL2CacheRaw))
    {
        ERROR("binaryDataSize is not equal to the size of HalL2CacheRaw size.");
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *rawData = Utils::ReinterpretConvert<HalL2CacheRaw *>(binaryData);
    auto *data = Utils::ReinterpretConvert<HalL2CacheRaw *>(halData);
    *data = *rawData;
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(L2_CACHE_PARSER, L2_CACHE_ITEM_TYPE, L2CacheParseItem);
}  // namespace Domain
}  // namespace Analysis
