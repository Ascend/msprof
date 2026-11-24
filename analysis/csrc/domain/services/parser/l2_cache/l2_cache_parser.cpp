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

#include "analysis/csrc/domain/services/parser/l2_cache/include/l2_cache_parser.h"

#include <vector>

#include "analysis/csrc/domain/entities/hal/include/hal_l2_cache.h"
#include "analysis/csrc/domain/services/parser/parser_item/l2_cache_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
std::vector<std::string> L2CacheParser::GetFilePattern() { return filePrefix_; }

uint32_t L2CacheParser::GetTrunkSize() { return L2_CACHE_RECORD_SIZE; }

// Keep the full file prefix; ParseData ignores a trailing incomplete record.
uint64_t L2CacheParser::GetReadOffset(uint64_t fileSize, uint32_t trunkSize) const
{
    (void)fileSize;
    (void)trunkSize;
    return 0;
}

uint32_t L2CacheParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    (void)context;
    if (binaryDataSize > 0 && binaryData == nullptr)
    {
        ERROR("l2 cache binary data is null, data size: %.", binaryDataSize);
        return ANALYSIS_ERROR;
    }
    const auto trunkSize = GetTrunkSize();
    if (trunkSize == 0)
    {
        ERROR("l2 cache trunk size is 0");
        return ANALYSIS_ERROR;
    }
    if (binaryDataSize == 0)
    {
        return ANALYSIS_OK;
    }
    if (binaryDataSize < trunkSize)
    {
        ERROR("Invalid l2 cache data size: %, record size: %.", binaryDataSize, trunkSize);
        return ANALYSIS_ERROR;
    }
    const size_t recordCount = binaryDataSize / trunkSize;
    const size_t remainingBytes = binaryDataSize % trunkSize;
    if (remainingBytes != 0)
    {
        WARN("l2 cache trailing bytes ignored, total size: %, record size: %, remaining bytes: %.", binaryDataSize,
             trunkSize, remainingBytes);
    }
    const auto parserItem = ParserItemFactory::GetParseItem(L2_CACHE_PARSER, L2_CACHE_ITEM_TYPE);
    if (parserItem == nullptr)
    {
        ERROR("There is no L2 cache parser item.");
        return ANALYSIS_ERROR;
    }
    std::vector<HalL2CacheRaw> parsedData;
    parsedData.reserve(recordCount);
    for (size_t index = 0; index < recordCount; ++index)
    {
        HalL2CacheRaw data{};
        if (parserItem(binaryData.get() + index * trunkSize, trunkSize, Utils::ReinterpretConvert<uint8_t *>(&data),
                       0) != ANALYSIS_OK)
        {
            ERROR("Parse L2 cache data item failed, index is %.", index);
            return ANALYSIS_ERROR;
        }
        parsedData.emplace_back(std::move(data));
    }
    if (parsedData.empty())
    {
        return ANALYSIS_OK;
    }
    std::shared_ptr<std::vector<HalL2CacheRaw>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalL2CacheRaw>, ANALYSIS_ERROR, std::move(parsedData));
    return dataInventory.Inject(data) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

// REGISTER_PROCESS_SEQUENCE(L2CacheParser, true);
// REGISTER_PROCESS_DEPENDENT_DATA(L2CacheParser);
// REGISTER_PROCESS_SUPPORT_CHIP(L2CacheParser, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0, CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0, CHIP_V6_1_1);
}  // namespace Domain
}  // namespace Analysis
