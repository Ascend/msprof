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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/services/parser/log/include/stars_soc_profile_parser.h"

#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
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
#pragma pack(1)
struct StarsSocProfileHeader
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t resv : 6;
    uint16_t magicNum;
};
#pragma pack()
}  // namespace

std::vector<std::string> StarsSocProfileParser::GetFilePattern() { return filePrefix_; }

uint32_t StarsSocProfileParser::GetTrunkSize() { return STARS_SOC_PROFILE_STRUCT_SIZE; }

uint32_t StarsSocProfileParser::ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data)
{
    if (binaryDataSize < sizeof(StarsSocProfileHeader))
    {
        ERROR("The binaryDataSize is small than StarsSocProfileHeader");
        return ANALYSIS_ERROR;
    }
    auto* header = ReinterpretConvert<StarsSocProfileHeader*>(binaryData);
    if (header->magicNum != STARS_MAGIC_NUM)
    {
        ++invalidCnt_;
        return ANALYSIS_OK;
    }

    auto parser = ParserItemFactory::GetParseItem(GetParserType(), header->funcType);
    if (parser == nullptr)
    {
        WARN("There is no Parser function to handle SocProfile data! functype is %", header->funcType);
        return ANALYSIS_OK;
    }
    const int currentCnt = parser(binaryData, binaryDataSize, data, 0);
    if (currentCnt != DEFAULT_CNT)
    {
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

uint32_t StarsSocProfileParser::ParseData(DataInventory& dataInventory, const Infra::Context& context)
{
    (void)context;
    const uint32_t trunkSize = GetTrunkSize();
    const uint64_t structCount = binaryDataSize / trunkSize;
    INFO("StarsSocProfile structCount: %", structCount);
    uint64_t parseFailedCnt = 0;
    if (!Reserve(halUniData_, structCount))
    {
        ERROR("Reserve for StarsSocProfile data failed");
        return ANALYSIS_ERROR;
    }

    for (uint64_t i = 0; i < structCount; ++i)
    {
        auto* record = &binaryData[i * trunkSize];
        HalSocProfileData item;
        if (ParseDataItem(record, trunkSize, ReinterpretConvert<uint8_t*>(&item)) == ANALYSIS_ERROR)
        {
            ++parseFailedCnt;
            continue;
        }
        if (item.type != SOC_PROFILE_INVALID)
        {
            halUniData_.emplace_back(std::move(item));
        }
    }
    if (invalidCnt_ != 0)
    {
        ERROR("% chunks have invalid magic number, expected %", invalidCnt_, STARS_MAGIC_NUM);
    }
    if (parseFailedCnt != 0)
    {
        ERROR("% chunks failed to parse StarsSocProfile data", parseFailedCnt);
    }

    std::shared_ptr<std::vector<HalSocProfileData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalSocProfileData>, ANALYSIS_ERROR, std::move(halUniData_));
    if (!dataInventory.Inject(data))
    {
        ERROR("Inject StarsSocProfile data into DataInventory failed");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

}  // namespace Domain
}  // namespace Analysis
