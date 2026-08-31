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

#include "analysis/csrc/domain/services/parser/qos/include/qos_parser.h"

#include <utility>
#include <vector>

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_item/qos_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const uint16_t STARS_MAGIC_NUM = 0x6bd3;
}  // namespace

uint32_t QosParserBase::GetTrunkSize() { return Analysis::QOS_STRUCT_SIZE; }

bool QosParserBase::ParseDataItem(uint8_t *binaryData,
                                  const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                                  const Utils::SyscntConversionParams &timeParams, HalQosBwData &data) const
{
    if (parserItem(binaryData, Analysis::QOS_STRUCT_SIZE, Utils::ReinterpretConvert<uint8_t *>(&data), 0) !=
        ANALYSIS_OK)
    {
        return false;
    }
    data.timestamp = Utils::GetTimeFromSyscnt(data.timestamp, timeParams).Uint64();
    return true;
}

uint32_t QosParserBase::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const DeviceContext &deviceContext = static_cast<const DeviceContext &>(context);
    const Utils::SyscntConversionParams timeParams = GenerateSyscntConversionParams(deviceContext);
    const size_t recordCount = binaryDataSize / Analysis::QOS_STRUCT_SIZE;
    const size_t remainingBytes = binaryDataSize % Analysis::QOS_STRUCT_SIZE;
    if (remainingBytes != 0)
    {
        WARN("Ignore incomplete QoS raw record: remaining bytes is %, record size is %.", remainingBytes,
             Analysis::QOS_STRUCT_SIZE);
    }
    const uint32_t parserItemType = GetParserItemType();
    const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parserItem =
        ParserItemFactory::GetParseItem(QOS_PARSER, parserItemType);
    if (parserItem == nullptr)
    {
        ERROR("There is no QoS parser item for item type %.", parserItemType);
        return ANALYSIS_ERROR;
    }
    std::vector<HalQosBwData> parsedData;
    parsedData.reserve(recordCount);
    for (size_t index = 0; index < recordCount; ++index)
    {
        uint8_t *rawData = binaryData.get() + index * Analysis::QOS_STRUCT_SIZE;
        if (!IsDataItemValid(rawData))
        {
            continue;
        }
        HalQosBwData data;
        if (!ParseDataItem(rawData, parserItem, timeParams, data))
        {
            ERROR("Parse QoS data item failed, index is %.", index);
            continue;
        }
        parsedData.emplace_back(std::move(data));
    }
    std::shared_ptr<std::vector<HalQosBwData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalQosBwData>, ANALYSIS_ERROR, std::move(parsedData));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

std::vector<std::string> QosParser::GetFilePattern() { return filePrefix_; }

uint32_t QosParser::GetParserItemType() const { return QOS_V4_ITEM_TYPE; }

std::vector<std::string> StarsQosParser::GetFilePattern() { return filePrefix_; }

uint32_t StarsQosParser::GetParserItemType() const { return QOS_V6_ITEM_TYPE; }

bool StarsQosParser::IsDataItemValid(const uint8_t *binaryData) const
{
    const uint16_t header = Utils::ReadLittleEndian<uint16_t>(binaryData);
    const uint16_t magicNum = Utils::ReadLittleEndian<uint16_t>(binaryData + sizeof(uint16_t));
    return magicNum == STARS_MAGIC_NUM && (header & 0x3fU) == QOS_V6_ITEM_TYPE;
}

// REGISTER_PROCESS_SEQUENCE(QosParser, false);
// REGISTER_PROCESS_SUPPORT_CHIP(QosParser, CHIP_V4_1_0);
//
// namespace STARS_QOS_REGISTER
//{
// REGISTER_PROCESS_SEQUENCE(StarsQosParser, false);
// REGISTER_PROCESS_SUPPORT_CHIP(StarsQosParser, CHIP_V6_1_0, CHIP_V6_2_0);
// }  // namespace STARS_QOS_REGISTER
}  // namespace Domain
}  // namespace Analysis
