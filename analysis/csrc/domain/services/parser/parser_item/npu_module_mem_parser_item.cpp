/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/parser/parser_item/npu_module_mem_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_npu_module_mem.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/binary_utils.h"

namespace Analysis
{
namespace Domain
{
int NpuModuleMemParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t)
{
    if (binaryData == nullptr || halData == nullptr || binaryDataSize != Analysis::NPU_MODULE_MEM_STRUCT_SIZE)
    {
        return PARSER_ERROR_SIZE_MISMATCH;
    }
    auto *data = reinterpret_cast<HalNpuModuleMemData *>(halData);
    data->moduleId = Utils::ReadLittleEndian<uint32_t>(binaryData);
    data->syscnt = Utils::ReadLittleEndian<uint64_t>(binaryData + 2U * sizeof(uint32_t));
    data->totalSize = Utils::ReadLittleEndian<uint64_t>(binaryData + 2U * sizeof(uint32_t) + sizeof(uint64_t));
    return ANALYSIS_OK;
}

REGISTER_PARSER_ITEM(NPU_MODULE_MEM_PARSER, NPU_MODULE_MEM_ITEM_TYPE, NpuModuleMemParseItem);
}  // namespace Domain
}  // namespace Analysis
