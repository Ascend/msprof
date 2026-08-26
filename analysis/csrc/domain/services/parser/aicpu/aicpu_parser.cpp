/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/services/parser/aicpu/include/aicpu_parser.h"

#include "analysis/csrc/domain/entities/hal/include/stream_expand_spec.h"
#include "analysis/csrc/domain/services/device_context/load_stream_expand_spec_data.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

namespace Analysis
{
namespace Domain
{

namespace
{
constexpr uint32_t AICPU_TRUNK_SIZE = sizeof(MsprofAdditionalInfo);
}  // namespace
using namespace Infra;
using namespace Utils;
std::vector<std::string> AicpuParser::GetFilePattern() { return filePrefix_; }

uint32_t AicpuParser::GetTrunkSize() { return AICPU_TRUNK_SIZE; }

void AicpuParser::SetDeviceAicpuStreamIdMap()
{
    for (const auto &aicpuData : aicpuData_)
    {
        if (aicpuData.type != AicpuType::KFC_HCCL_INFO) continue;
        for (const auto &info : aicpuData.KfcInfos.infos)
        {
            if (info.groupName == 0) continue;
            streamIdMap_.streamIdMap[info.taskId] = info.streamId;
        }
    }
}

uint32_t AicpuParser::ParseDataItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *data, uint16_t expandStatus)
{
    if (binaryDataSize < sizeof(MsprofAdditionalInfo))
    {
        ERROR("The binaryDataSize is small than MsprofAdditionalInfo");
        return ANALYSIS_ERROR;
    }
    auto *header = ReinterpretConvert<MsprofAdditionalInfo *>(binaryData);

    std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parser =
        ParserItemFactory::GetParseItem(GetParserType(), header->type);
    if (parser == nullptr)
    {
        ERROR("There is no Parser function to handle data! functype is %", header->type);
        return ANALYSIS_ERROR;
    }
    return parser(binaryData, binaryDataSize, data, expandStatus);
}

uint32_t AicpuParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    auto streamExpandSpecData = dataInventory.GetPtr<StreamExpandSpec>();
    uint16_t expandStatus =
        streamExpandSpecData && streamExpandSpecData->expandStatus ? streamExpandSpecData->expandStatus : 0;
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    INFO("aicpu structCount: %", structCount);
    int stat{ANALYSIS_OK};
    if (!Utils::Resize(aicpuData_, structCount))
    {
        ERROR("Resize for aicpu data failed!");
        return ANALYSIS_ERROR;
    }
    for (uint64_t i = 0; i < structCount; i++)
    {
        if (this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize,
                                ReinterpretConvert<uint8_t *>(&this->aicpuData_[i]), expandStatus) == ANALYSIS_ERROR)
        {
            ERROR("parse data failed, total of % pieces of data are parsed", i);
            stat = ANALYSIS_ERROR;
        }
    }
    if (context.GetChipID() == CHIP_V6_1_0 || context.GetChipID() == CHIP_V6_2_0)
    {
        SetDeviceAicpuStreamIdMap();
    }
    std::shared_ptr<DeviceStreamInfo> streamIdMapPtr;
    MAKE_SHARED_RETURN_VALUE(streamIdMapPtr, DeviceStreamInfo, ANALYSIS_ERROR, std::move(streamIdMap_));
    INFO("AicpuParser inject streamIdMap, size=%.", streamIdMapPtr->streamIdMap.size());
    dataInventory.Inject(streamIdMapPtr);

    std::shared_ptr<std::vector<AicpuData>> aicpuData;
    MAKE_SHARED_RETURN_VALUE(aicpuData, std::vector<AicpuData>, ANALYSIS_ERROR, std::move(aicpuData_));
    INFO("AicpuParser inject aicpuData, count=%.", aicpuData->size());
    dataInventory.Inject(aicpuData);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(AicpuParser, true, LoadStreamExpandSpec);
REGISTER_PROCESS_DEPENDENT_DATA(AicpuParser, StreamExpandSpec);
REGISTER_PROCESS_SUPPORT_CHIP(AicpuParser, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
