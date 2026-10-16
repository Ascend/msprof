/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/parser/parser_item/npu_mem_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_npu_mem.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
constexpr size_t EVENT_OFFSET = sizeof(uint32_t);
constexpr size_t V1_TIMESTAMP_OFFSET = 0;
constexpr size_t V2_TIMESTAMP_OFFSET = 2U * sizeof(uint32_t);
constexpr size_t DDR_OFFSET = V2_TIMESTAMP_OFFSET + sizeof(uint64_t);
constexpr size_t HBM_OFFSET = DDR_OFFSET + sizeof(uint64_t);

int ParseNpuMemRecord(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, bool useV1Timestamp)
{
    if (binaryData == nullptr || halData == nullptr || binaryDataSize != Analysis::NPU_MEM_STRUCT_SIZE)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *data = reinterpret_cast<HalNpuMemData *>(halData);
    data->event = Utils::ReadLittleEndian<uint32_t>(binaryData + EVENT_OFFSET);
    data->timestamp = useV1Timestamp ? Utils::ReadLittleEndian<uint32_t>(binaryData + V1_TIMESTAMP_OFFSET)
                                     : Utils::ReadLittleEndian<uint64_t>(binaryData + V2_TIMESTAMP_OFFSET);
    data->ddr = Utils::ReadLittleEndian<uint64_t>(binaryData + DDR_OFFSET);
    data->hbm = Utils::ReadLittleEndian<uint64_t>(binaryData + HBM_OFFSET);
    return ANALYSIS_OK;
}
}  // namespace

int NpuMemV1ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    return ParseNpuMemRecord(binaryData, binaryDataSize, halData, true);
}

int NpuMemV2ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    return ParseNpuMemRecord(binaryData, binaryDataSize, halData, false);
}

REGISTER_PARSER_ITEM(NPU_MEM_PARSER, NPU_MEM_V1_ITEM_TYPE, NpuMemV1ParseItem);
REGISTER_PARSER_ITEM(NPU_MEM_PARSER, NPU_MEM_V2_ITEM_TYPE, NpuMemV2ParseItem);
}  // namespace Domain
}  // namespace Analysis
