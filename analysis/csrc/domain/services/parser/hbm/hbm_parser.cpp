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

#include "analysis/csrc/domain/services/parser/hbm/include/hbm_parser.h"

#include <memory>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
uint32_t HbmParser::Prepare(const DeviceContext &deviceContext)
{
    const std::string &version = deviceContext.GetCollectionVersion();
    if (version == "2.0")
    {
        recordSize_ = Analysis::HBM_STRUCT_SIZE_V2;
        parserItemType_ = HBM_V2_ITEM_TYPE;
        return ANALYSIS_OK;
    }
    if (version != "1.0")
    {
        WARN("Unsupported HBM data version %, use version 1.0 format.", version);
    }
    recordSize_ = Analysis::HBM_STRUCT_SIZE_V1;
    parserItemType_ = HBM_V1_ITEM_TYPE;
    return ANALYSIS_OK;
}

std::vector<std::string> HbmParser::GetFilePattern() { return filePrefix_; }

uint32_t HbmParser::GetTrunkSize() { return recordSize_; }

uint32_t HbmParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &)
{
    const auto parserItem = ParserItemFactory::GetParseItem(HBM_PARSER, parserItemType_);
    if (parserItem == nullptr)
    {
        ERROR("There is no HBM parser item for item type %.", parserItemType_);
        return ANALYSIS_ERROR;
    }
    const size_t recordCount = binaryDataSize / recordSize_;
    std::vector<HalHbmData> parsedData;
    if (!Utils::Reserve(parsedData, recordCount))
    {
        return ANALYSIS_ERROR;
    }
    for (size_t index = 0; index < recordCount; ++index)
    {
        HalHbmData data;
        uint8_t *rawData = binaryData.get() + index * recordSize_;
        if (parserItem(rawData, recordSize_, reinterpret_cast<uint8_t *>(&data), 0) != ANALYSIS_OK)
        {
            ERROR("Parse HBM data item failed, index is %.", index);
            return ANALYSIS_ERROR;
        }
        parsedData.emplace_back(std::move(data));
    }
    std::shared_ptr<std::vector<HalHbmData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalHbmData>, ANALYSIS_ERROR, std::move(parsedData));
    return dataInventory.Inject(data) ? ANALYSIS_OK : ANALYSIS_ERROR;
}

// 暂不启用 HBM C++ 解析；启用时同步恢复持久化注册及 Python parser 跳过名单。
// REGISTER_PROCESS_SEQUENCE(HbmParser, false);
// REGISTER_PROCESS_SUPPORT_CHIP(HbmParser, CHIP_V1_1_0, CHIP_V2_1_0, CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0,
// CHIP_V4_1_0,
//                               CHIP_V1_1_1, CHIP_V1_1_2, CHIP_V1_1_3, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
