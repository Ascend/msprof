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

#include "analysis/csrc/domain/services/parser/ub/include/ub_parser.h"

#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_item/ub_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
std::vector<std::string> UbParser::GetFilePattern() { return filePrefix_; }

uint32_t UbParser::GetTrunkSize() { return Analysis::UB_STRUCT_SIZE; }

bool UbParser::ParseDataItem(uint8_t *binaryData,
                             const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                             HalUbBwData &data) const
{
    return parserItem(binaryData, Analysis::UB_STRUCT_SIZE, Utils::ReinterpretConvert<uint8_t *>(&data), 0) ==
           ANALYSIS_OK;
}

uint32_t UbParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const DeviceContext &deviceContext = static_cast<const DeviceContext &>(context);
    DeviceInfo deviceInfo;
    deviceContext.Getter(deviceInfo);
    const size_t recordCount = binaryDataSize / Analysis::UB_STRUCT_SIZE;
    const size_t remainingBytes = binaryDataSize % Analysis::UB_STRUCT_SIZE;
    if (remainingBytes != 0)
    {
        WARN("Ignore incomplete UB raw record: remaining bytes is %, record size is %.", remainingBytes,
             Analysis::UB_STRUCT_SIZE);
    }
    const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parserItem =
        ParserItemFactory::GetParseItem(UB_PARSER, UB_ITEM_TYPE);
    if (parserItem == nullptr)
    {
        ERROR("There is no UB parser item.");
        return ANALYSIS_ERROR;
    }
    std::vector<HalUbBwData> parsedData;
    parsedData.reserve(recordCount);
    for (size_t index = 0; index < recordCount; ++index)
    {
        HalUbBwData data;
        if (!ParseDataItem(binaryData.get() + index * Analysis::UB_STRUCT_SIZE, parserItem, data))
        {
            ERROR("Parse UB data item failed, index is %.", index);
            continue;
        }
        data.deviceId = deviceInfo.deviceId;
        parsedData.emplace_back(std::move(data));
    }
    std::shared_ptr<std::vector<HalUbBwData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalUbBwData>, ANALYSIS_ERROR, std::move(parsedData));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(UbParser, false);
// REGISTER_PROCESS_SUPPORT_CHIP(UbParser, CHIP_V6_1_0, CHIP_V6_2_0);
}  // namespace Domain
}  // namespace Analysis
