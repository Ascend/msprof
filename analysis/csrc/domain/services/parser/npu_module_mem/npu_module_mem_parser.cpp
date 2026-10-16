/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/parser/npu_module_mem/include/npu_module_mem_parser.h"

#include <string>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/services/parser/parser_item/npu_module_mem_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
std::vector<std::string> NpuModuleMemParser::GetFilePattern() { return filePrefix_; }

uint32_t NpuModuleMemParser::GetTrunkSize() { return Analysis::NPU_MODULE_MEM_STRUCT_SIZE; }

bool NpuModuleMemParser::ParseDataItem(uint8_t *binaryData,
                                       const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                                       HalNpuModuleMemData &data) const
{
    return parserItem(binaryData, Analysis::NPU_MODULE_MEM_STRUCT_SIZE, Utils::ReinterpretConvert<uint8_t *>(&data),
                      0) == ANALYSIS_OK;
}

uint32_t NpuModuleMemParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    DeviceInfo deviceInfo;
    deviceContext.Getter(deviceInfo);
    const auto parserItem = ParserItemFactory::GetParseItem(NPU_MODULE_MEM_PARSER, NPU_MODULE_MEM_ITEM_TYPE);
    if (parserItem == nullptr)
    {
        ERROR("There is no NPU_MODULE_MEM parser item.");
        return ANALYSIS_ERROR;
    }
    const size_t recordCount = binaryDataSize / Analysis::NPU_MODULE_MEM_STRUCT_SIZE;
    std::vector<HalNpuModuleMemData> parsedData;
    parsedData.reserve(recordCount);
    for (size_t index = 0; index < recordCount; ++index)
    {
        HalNpuModuleMemData data;
        if (!ParseDataItem(binaryData.get() + index * Analysis::NPU_MODULE_MEM_STRUCT_SIZE, parserItem, data))
        {
            ERROR("Parse NPU_MODULE_MEM data item failed, index is %.", index);
            continue;
        }
        data.deviceType = "NPU:" + std::to_string(deviceInfo.deviceId);
        parsedData.emplace_back(std::move(data));
    }
    std::shared_ptr<std::vector<HalNpuModuleMemData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalNpuModuleMemData>, ANALYSIS_ERROR, std::move(parsedData));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

// REGISTER_PROCESS_SEQUENCE(NpuModuleMemParser, true);
// REGISTER_PROCESS_SUPPORT_CHIP(NpuModuleMemParser, CHIP_V4_1_0);
}  // namespace Domain
}  // namespace Analysis
