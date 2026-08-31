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

#include "analysis/csrc/domain/services/parser/parser_item/ub_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_ub.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
#pragma pack(1)
struct UbRawRecord
{
    uint16_t magicNum;
    uint16_t reserved;
    uint16_t portId;
    uint16_t reservedPort;
    uint64_t timestamp;
    uint64_t metrics[14];
};
#pragma pack()

template <typename ValueType>
ValueType ReadRawField(const ValueType *field)
{
    return Utils::ReadLittleEndian<ValueType>(reinterpret_cast<const uint8_t *>(field));
}
}  // namespace

int UbParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryDataSize != Analysis::UB_STRUCT_SIZE)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    const auto *record = reinterpret_cast<const UbRawRecord *>(binaryData);
    auto *data = reinterpret_cast<HalUbBwData *>(halData);
    data->portId = ReadRawField(&record->portId);
    data->timestamp = ReadRawField(&record->timestamp);
    for (size_t index = 0; index < data->metrics.size(); ++index)
    {
        data->metrics[index] = ReadRawField(&record->metrics[index]);
    }
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(UB_PARSER, UB_ITEM_TYPE, UbParseItem);
}  // namespace Domain
}  // namespace Analysis
