/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/parser/npu_mem/include/npu_mem_parser.h"

#include <limits>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/services/parser/parser_item/npu_mem_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace
{
const std::string COLLECTION_VERSION_V2 = "2.0";

bool ValidateTimeContext(const HostStartLog &hostStart, const DeviceStartInfo &startInfo)
{
    if (hostStart.clockMonotonicRaw == 0 || startInfo.collectionTimeBegin == 0 || startInfo.clockMonotonicRaw == 0)
    {
        ERROR("NPU_MEM time context is incomplete.");
        return false;
    }
    if (startInfo.collectionTimeBegin > std::numeric_limits<uint64_t>::max() / Common::NS_TO_US)
    {
        ERROR("NPU_MEM collection epoch is out of range.");
        return false;
    }
    if (startInfo.collectionTimeBegin * Common::NS_TO_US < startInfo.clockMonotonicRaw)
    {
        ERROR("NPU_MEM collection epoch precedes its monotonic timestamp.");
        return false;
    }
    return true;
}
}  // namespace

std::vector<std::string> NpuMemParser::GetFilePattern() { return filePrefix_; }

uint32_t NpuMemParser::GetTrunkSize() { return Analysis::NPU_MEM_STRUCT_SIZE; }

uint32_t NpuMemParser::GetParserItemType(const DeviceContext &deviceContext) const
{
    if (deviceContext.GetCollectionVersion() == COLLECTION_VERSION_V2)
    {
        return NPU_MEM_V2_ITEM_TYPE;
    }
    if (deviceContext.GetCollectionVersion() != "1.0")
    {
        WARN("NPU_MEM collection version % is not supported, use 1.0 parser.", deviceContext.GetCollectionVersion());
    }
    return NPU_MEM_V1_ITEM_TYPE;
}

bool NpuMemParser::ParseDataItem(uint8_t *binaryData,
                                 const std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> &parserItem,
                                 HalNpuMemData &data) const
{
    return parserItem(binaryData, Analysis::NPU_MEM_STRUCT_SIZE, Utils::ReinterpretConvert<uint8_t *>(&data), 0) ==
           ANALYSIS_OK;
}

bool NpuMemParser::ConvertTimestamp(HalNpuMemData &data, const DeviceContext &deviceContext) const
{
    HostStartLog hostStart;
    DeviceStartInfo startInfo;
    deviceContext.Getter(hostStart);
    deviceContext.Getter(startInfo);
    if (!ValidateTimeContext(hostStart, startInfo))
    {
        return false;
    }
    const auto timeParams = deviceContext.GetSyscntConversionParams();
    uint64_t timestampNs = 0;
    if (data.timestamp > std::numeric_limits<uint64_t>::max() / Common::NS_TO_US ||
        !Utils::AddWithoutOverflow(data.timestamp * Common::NS_TO_US, timeParams.hostMonotonic, timestampNs) ||
        !Utils::AddWithoutOverflow(timestampNs, startInfo.collectionTimeBegin * Common::NS_TO_US, timestampNs) ||
        timestampNs < startInfo.clockMonotonicRaw)
    {
        ERROR("NPU_MEM timestamp conversion overflow.");
        return false;
    }
    // Keep parity with InfoConfReader.get_absolute_time_by_sampling_timestamp().
    data.timestamp = timestampNs - startInfo.clockMonotonicRaw;
    return true;
}

uint32_t NpuMemParser::ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context)
{
    const auto &deviceContext = static_cast<const DeviceContext &>(context);
    const auto parserItem = ParserItemFactory::GetParseItem(NPU_MEM_PARSER, GetParserItemType(deviceContext));
    if (parserItem == nullptr)
    {
        ERROR("There is no NPU_MEM parser item.");
        return ANALYSIS_ERROR;
    }
    const size_t recordCount = binaryDataSize / Analysis::NPU_MEM_STRUCT_SIZE;
    std::vector<HalNpuMemData> parsedData;
    parsedData.reserve(recordCount);
    for (size_t index = 0; index < recordCount; ++index)
    {
        HalNpuMemData data;
        if (!ParseDataItem(binaryData.get() + index * Analysis::NPU_MEM_STRUCT_SIZE, parserItem, data) ||
            !ConvertTimestamp(data, deviceContext))
        {
            ERROR("Parse NPU_MEM data item failed, index is %.", index);
            continue;
        }
        parsedData.emplace_back(std::move(data));
    }
    const size_t successCount = parsedData.size();
    const size_t failedCount = recordCount - successCount;
    if (failedCount != 0)
    {
        if (successCount == 0)
        {
            ERROR("All NPU_MEM records failed to parse: total: %, success: %, failed: %.", recordCount, successCount,
                  failedCount);
            return ANALYSIS_ERROR;
        }
        const double failurePercent = static_cast<double>(failedCount) * 100.0 / recordCount;
        WARN("NPU_MEM records partially failed to parse: total: %, success: %, failed: %, failure percent: %.",
             recordCount, successCount, failedCount, failurePercent);
    }
    std::shared_ptr<std::vector<HalNpuMemData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalNpuMemData>, ANALYSIS_ERROR, std::move(parsedData));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(NpuMemParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(NpuMemParser, CHIP_V4_1_0, CHIP_V6_1_0);
}  // namespace Domain
}  // namespace Analysis
