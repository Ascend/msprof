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

#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"

#include "analysis/csrc/domain/entities/hal/include/stream_expand_spec.h"
#include "analysis/csrc/domain/services/device_context/load_stream_expand_spec_data.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/step_trace_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_block_num_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_flip_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_memcpy_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/task_type_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Utils;

#pragma pack(1)
struct TsTrackHeader
{
    uint8_t resv;
    uint8_t funcType;
};
#pragma pack()

std::vector<std::string> TsTrackParser::GetFilePattern() { return filePrefix_; }

uint32_t TsTrackParser::GetTrunkSize() { return TS_TRACK_STRUCT_SIZE; }

uint32_t TsTrackParser::ParseDataItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *data,
                                      ParserType parserType, uint16_t expandStatus)
{
    if (binaryDataSize < sizeof(TsTrackHeader))
    {
        ERROR("The binaryDataSize is small than ParseDataItem");
        return ANALYSIS_ERROR;
    }
    auto *header = ReinterpretConvert<TsTrackHeader *>(binaryData);
    std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parser =
        ParserItemFactory::GetParseItem(parserType, header->funcType);
    if (parser != nullptr)
    {
        auto result = parser(binaryData, binaryDataSize, data, expandStatus);
        if (result == PARSER_ERROR_SIZE_MISMATCH)
        {
            ERROR("Parse ts track item failed, functype is %", header->funcType);
            return ANALYSIS_ERROR;
        }
        return ANALYSIS_OK;
    }
    ERROR("Missing parser for supported ts track data, functype is %", header->funcType);
    return ANALYSIS_ERROR;
}

uint32_t TsTrackParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    int stat{ANALYSIS_OK};
    auto streamExpandSpec = dataInventory.GetPtr<StreamExpandSpec>();
    uint16_t expandStatus =
        streamExpandSpec != nullptr && streamExpandSpec->expandStatus ? streamExpandSpec->expandStatus : 0;
    // ParserType parserType =
    //     context.GetChipID() == CHIP_V6_1_0 || context.GetChipID() == CHIP_V6_2_0 ? TRACK_PARSER_V6 : TRACK_PARSER;
    ParserType parserType = TRACK_PARSER;  // 后续放开上面注释，删除本行即可恢复对CHIP_V6的支持
    INFO("TsTrack structCount: %", structCount);

    halUniData_.clear();
    if (!Utils::Reserve(halUniData_, structCount))
    {
        ERROR("Reserve for TsTrack data failed!");
        return ANALYSIS_ERROR;
    }

    for (uint64_t i = 0; i < structCount; i++)
    {
        HalTrackData parsedData{};
        auto *dataPoint = ReinterpretConvert<uint8_t *>(&parsedData);
        auto result =
            this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize, dataPoint, parserType, expandStatus);
        if (result == ANALYSIS_ERROR)
        {
            ERROR("parse ts track data failed, total of % pieces of data are parsed", i);
            stat = ANALYSIS_ERROR;
            continue;
        }
        halUniData_.emplace_back(std::move(parsedData));
    }
    std::shared_ptr<std::vector<HalTrackData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalTrackData>, ANALYSIS_ERROR, std::move(halUniData_));
    dataInventory.Inject(data);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(TsTrackParser, true, LoadStreamExpandSpec);
REGISTER_PROCESS_DEPENDENT_DATA(TsTrackParser, StreamExpandSpec);
REGISTER_PROCESS_SUPPORT_CHIP(TsTrackParser, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
