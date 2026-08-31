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

#include "analysis/csrc/domain/services/parser/parser_item/qos_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_qos.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"

namespace Analysis
{
namespace Domain
{
namespace
{
#pragma pack(1)
struct QosRawRecord
{
    uint32_t reserved[2];
    uint64_t reservedTimestamp;
    uint64_t syscnt;
    uint32_t metrics[10];
};

struct StarsQosRawRecord
{
    uint16_t header;
    uint16_t magicNum;
    uint32_t reserved;
    uint64_t syscnt;
    uint64_t reservedTimestamp;
    uint32_t metrics[10];
};
#pragma pack()

template <typename ValueType>
ValueType ReadRawField(const ValueType *field)
{
    return Utils::ReadLittleEndian<ValueType>(reinterpret_cast<const uint8_t *>(field));
}

template <typename RawRecord>
int ParseQosRecord(const RawRecord *record, uint8_t *halData, int32_t dieId)
{
    auto *data = reinterpret_cast<HalQosBwData *>(halData);
    // The parser converts this syscnt value to a timestamp after item dispatch.
    data->timestamp = ReadRawField(&record->syscnt);
    data->dieId = dieId;
    for (size_t index = 0; index < data->metrics.size(); ++index)
    {
        data->metrics[index] = ReadRawField(&record->metrics[index]);
    }
    return ANALYSIS_OK;
}
}  // namespace

int QosV4ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryDataSize != Analysis::QOS_STRUCT_SIZE)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    const auto *record = reinterpret_cast<const QosRawRecord *>(binaryData);
    return ParseQosRecord(record, halData, INVALID_VALUE);
}

int QosV6ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryDataSize != Analysis::QOS_STRUCT_SIZE)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    const auto *record = reinterpret_cast<const StarsQosRawRecord *>(binaryData);
    const int32_t dieId = static_cast<int32_t>(ReadRawField(&record->header) >> 10U);
    return ParseQosRecord(record, halData, dieId);
}

REGISTER_PARSER_ITEM(QOS_PARSER, QOS_V4_ITEM_TYPE, QosV4ParseItem);
REGISTER_PARSER_ITEM(QOS_PARSER, QOS_V6_ITEM_TYPE, QosV6ParseItem);
}  // namespace Domain
}  // namespace Analysis
