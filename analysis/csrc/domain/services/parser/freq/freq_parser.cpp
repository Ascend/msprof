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

#include "analysis/csrc/domain/services/parser/freq//include/freq_parser.h"

#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{

using namespace Infra;
using namespace Analysis::Utils;

std::vector<std::string> FreqParser::GetFilePattern() { return this->filePrefix_; }

uint32_t FreqParser::GetTrunkSize() { return Analysis::FREQ_STRUCT_SIZE; }

uint32_t FreqParser::ParseDataItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *data)
{
    std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parser =
        ParserItemFactory::GetParseItem(FREQ_PARSER, DEFAULT_FREQ_LPM);
    if (parser == nullptr)
    {
        ERROR("There is no Parser function to handle data! funcType is %", DEFAULT_FREQ_LPM);
        return ANALYSIS_ERROR;
    }
    const auto result = parser(binaryData, binaryDataSize, data, 0);
    // Frequency items return DEFAULT_CNT on success; other values carry the item error.
    return result == DEFAULT_CNT ? ANALYSIS_OK : static_cast<uint32_t>(result);
}

uint32_t FreqParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    HalFreqLpmData initData;
    DeviceInfo deviceInfo;
    DeviceStartLog deviceStart;
    std::string devicePath;
    std::vector<HalFreqLpmData> lpmDataS;
    // Parser::ProcessEntry requires a DeviceContext before invoking ParseData.
    const DeviceContext &deviceContext = static_cast<const DeviceContext &>(context);
    devicePath = deviceContext.GetDeviceFilePath();
    deviceContext.Getter(deviceStart);
    deviceContext.Getter(deviceInfo);
    initData.sysCnt = deviceStart.cntVct;
    initData.freq = deviceInfo.aicFrequency;
    lpmDataS.emplace_back(std::move(initData));
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    INFO("FreqProfileData structCount is : %", structCount);
    if (!Utils::Resize(halUniData_, structCount))
    {
        ERROR("Resize for FreqProfile data failed!");
        return ANALYSIS_ERROR;
    }
    if (structCount == 0)
    {
        lpmDataS.clear();  // structCount为0说明没变频数据，不需要落盘，直接使用配置文件频率即可
    }
    int stat{ANALYSIS_OK};
    uint64_t truncatedRecordCount = 0;
    uint32_t firstTruncatedCount = 0;
    uint64_t retainedCount = 0;
    for (uint64_t i = 0; i < structCount; i++)
    {
        auto res = this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize,
                                       ReinterpretConvert<uint8_t *>(&this->halUniData_[i]));
        if (res != ANALYSIS_OK)
        {
            stat = ANALYSIS_ERROR;
            halUniData_[i] = HalFreqData{};
            ERROR("FreqProfileData parse error in %th", i);
        }
        else
        {
            const auto *rawData = ReinterpretConvert<const FreqData *>(&this->binaryData[i * trunkSize]);
            // The item reduces the output count only when it truncates a record.
            if (rawData->count > halUniData_[i].count)
            {
                if (truncatedRecordCount == 0)
                {
                    firstTruncatedCount = rawData->count;
                    retainedCount = halUniData_[i].count;
                }
                ++truncatedRecordCount;
            }
        }
    }
    // 根据device启动时间过滤有效频率数据
    for (const auto &freqData : halUniData_)
    {
        for (uint64_t i = 0; i < freqData.count; ++i)
        {
            auto freqLpmData = freqData.freqLpmDataS[i];
            if (freqLpmData.sysCnt < deviceStart.cntVct)
            {
                lpmDataS[0].freq = freqLpmData.freq;
            }
            else
            {
                lpmDataS.emplace_back(std::move(freqLpmData));
            }
        }
    }
    if (truncatedRecordCount != 0)
    {
        WARN(
            "Freq parsing in %: % records exceeded capacity %; only the first % entries of each were processed. "
            "First count: %.",
            devicePath, truncatedRecordCount, FREQ_LPM_DATA_COUNT, retainedCount, firstTruncatedCount);
    }
    INFO("Freq.data parser is completed, begin to save data into dataInventory!");
    std::shared_ptr<std::vector<HalFreqLpmData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalFreqLpmData>, ANALYSIS_ERROR, std::move(lpmDataS));
    dataInventory.Inject(data);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(FreqParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(FreqParser, CHIP_V4_1_0, CHIP_V1_1_1);
}  // namespace Domain
}  // namespace Analysis
