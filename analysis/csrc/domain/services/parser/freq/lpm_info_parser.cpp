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

#include "analysis/csrc/domain/services/parser/freq/include/lpm_info_parser.h"

#include <functional>

#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item/lpm_info_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Infra;
using namespace Utils;

namespace
{
constexpr uint32_t DEFAULT_VOLTAGE = 900;

void AppendLpmData(std::vector<HalLpmValue>& result, const HalLpmInfoRecord& record, uint64_t devCnt,
                   uint32_t defaultValue, uint64_t& latestPreDevCnt)
{
    if (result.empty())
    {
        result.emplace_back(devCnt, defaultValue);
    }
    uint32_t tmpValue = defaultValue;
    for (uint32_t i = 0; i < record.count; ++i)
    {
        const auto& data = record.lpmDataS[i];
        if (data.sysCnt < devCnt)
        {
            if (data.sysCnt >= latestPreDevCnt)
            {
                latestPreDevCnt = data.sysCnt;
                tmpValue = data.value;
            }
            continue;
        }
        result.emplace_back(data.sysCnt, data.value);
    }
    result[0].value = tmpValue;
}
}  // namespace

LpmInfoParser::~LpmInfoParser()
{
    if (unsupportedTypeCount_ > 0)
    {
        ERROR("Unsupported LpmInfo type, count is %", unsupportedTypeCount_);
    }
}

std::vector<std::string> LpmInfoParser::GetFilePattern() { return filePrefix_; }

uint32_t LpmInfoParser::GetTrunkSize() { return sizeof(LpmInfoRawData); }

uint32_t LpmInfoParser::ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data)
{
    auto parser = ParserItemFactory::GetParseItem(LPM_INFO_PARSER, DEFAULT_LPM_INFO);
    if (parser == nullptr)
    {
        ERROR("There is no Parser function to handle LpmInfo data");
        return ANALYSIS_ERROR;
    }
    return parser(binaryData, binaryDataSize, data, 0);
}

uint32_t LpmInfoParser::ParseData(DataInventory& dataInventory, const Infra::Context& context)
{
    const auto& deviceContext = static_cast<const DeviceContext&>(context);
    DeviceInfo deviceInfo;
    DeviceStartLog deviceStart;
    deviceContext.Getter(deviceInfo);
    deviceContext.Getter(deviceStart);
    auto trunkSize = GetTrunkSize();
    auto structCount = binaryDataSize / trunkSize;
    INFO("LpmInfo structCount is: %", structCount);
    if (!Utils::Resize(halUniData_, structCount))
    {
        ERROR("Resize for LpmInfo data failed");
        return ANALYSIS_ERROR;
    }
    uint64_t validCount = 0;
    uint64_t parseFailedCount = 0;
    for (uint64_t i = 0; i < structCount; ++i)
    {
        auto* target = ReinterpretConvert<uint8_t*>(&halUniData_[validCount]);
        if (ParseDataItem(&binaryData[i * trunkSize], trunkSize, target) != ANALYSIS_OK)
        {
            ++parseFailedCount;
            continue;
        }
        ++validCount;
    }
    if (parseFailedCount > 0)
    {
        ERROR("Parse LpmInfo data failed, failed count is %, total count is %, valid count is %", parseFailedCount,
              structCount, validCount);
    }
    if (!Utils::Resize(halUniData_, validCount))
    {
        ERROR("Resize for valid LpmInfo data failed");
        return ANALYSIS_ERROR;
    }
    if (structCount > 0 && validCount == 0)
    {
        ERROR("All LpmInfo data records failed to parse");
        return ANALYSIS_ERROR;
    }
    HalLpmInfoData result;
    uint64_t latestFreqPreDevCnt = 0;
    uint64_t latestAicVoltagePreDevCnt = 0;
    uint64_t latestBusVoltagePreDevCnt = 0;
    for (const auto& record : halUniData_)
    {
        switch (record.type)
        {
            case LpmInfoType::AIC_FREQ:
                AppendLpmData(result.freqData, record, deviceStart.cntVct, deviceInfo.aicFrequency,
                              latestFreqPreDevCnt);
                break;
            case LpmInfoType::AIC_VOLTAGE:
                AppendLpmData(result.aicVoltageData, record, deviceStart.cntVct, DEFAULT_VOLTAGE,
                              latestAicVoltagePreDevCnt);
                break;
            case LpmInfoType::BUS_VOLTAGE:
                AppendLpmData(result.busVoltageData, record, deviceStart.cntVct, DEFAULT_VOLTAGE,
                              latestBusVoltagePreDevCnt);
                break;
            default:
                ++unsupportedTypeCount_;
                break;
        }
    }
    std::shared_ptr<HalLpmInfoData> data;
    MAKE_SHARED_RETURN_VALUE(data, HalLpmInfoData, ANALYSIS_ERROR, std::move(result));
    if (!dataInventory.Inject(data))
    {
        ERROR("Inject LpmInfo data into DataInventory failed");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
