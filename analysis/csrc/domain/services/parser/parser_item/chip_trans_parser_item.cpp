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

#include "analysis/csrc/domain/services/parser/parser_item/chip_trans_parser_item.h"

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;

namespace
{
#pragma pack(1)
// V6 结构：CHIP_TRANS_V6_FMT = "2HI3Q8L"
struct ChipTransV6RawData
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t dieId : 6;
    uint16_t magicNum;
    uint32_t resv1;
    uint64_t sysCnt;
    uint64_t writeBandwidth;
    uint64_t readBandwidth;
    uint32_t resv[8];
};

// 非 V6 结构：CHIP_TRANS_FMT = "2HLQ2H3L2Q4L"
struct ChipTransV4RawData
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t accType : 6;
    uint16_t magicNum;
    uint32_t resv1;
    uint64_t sysCnt;
    uint16_t resv2;
    uint16_t eventIdField;
    uint32_t resv3[3];
    uint64_t value1;
    uint64_t value2;
    uint32_t resv4[4];
};
#pragma pack()

// 非 V6 芯片 acc_type（首 uint16 的高 6 位）区分 PA 链路与 PCIE 链路。
constexpr uint16_t ACC_TYPE_PA = 0b111111;    // 63
constexpr uint16_t ACC_TYPE_PCIE = 0b100010;  // 34
constexpr uint16_t EVENT_ID_MASK = 0x3FU;     // event_id 取低 6 位
}  // namespace

// 非 V6 结构：CHIP_TRANS_FMT = "2HLQ2H3L2Q4L"
int ChipTransParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != sizeof(ChipTransV4RawData))
    {
        ERROR("The ChipTrans record size is invalid, actual: %, expected: %", binaryDataSize,
              sizeof(ChipTransV4RawData));
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* target = ReinterpretConvert<HalLogData*>(halUniData);
    auto* source = ReinterpretConvert<ChipTransV4RawData*>(binaryData);
    target->type = SOC_PROFILE_CHIP_TRANS;
    target->chipTrans.sysCnt = source->sysCnt;
    target->chipTrans.id = source->eventIdField & EVENT_ID_MASK;
    target->chipTrans.value1 = source->value1;
    target->chipTrans.value2 = source->value2;
    if (source->accType == ACC_TYPE_PA)
    {
        target->chipTrans.transType = CHIP_TRANS_PA;
    }
    else if (source->accType == ACC_TYPE_PCIE)
    {
        target->chipTrans.transType = CHIP_TRANS_PCIE;
    }
    else
    {
        target->chipTrans.transType = CHIP_TRANS_INVALID;
    }
    return DEFAULT_CNT;
}

// V6 结构：CHIP_TRANS_V6_FMT = "2HI3Q8L"
int ChipTransParserItemV6(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus)
{
    (void)expandStatus;
    if (binaryData == nullptr || halUniData == nullptr || binaryDataSize != sizeof(ChipTransV6RawData))
    {
        ERROR("The ChipTrans record size is invalid, actual: %, expected: %", binaryDataSize,
              sizeof(ChipTransV6RawData));
        return PARSER_ERROR_SIZE_MISMATCH;
    }

    auto* target = ReinterpretConvert<HalLogData*>(halUniData);
    auto* source = ReinterpretConvert<ChipTransV6RawData*>(binaryData);
    target->type = SOC_PROFILE_CHIP_TRANS;
    target->chipTrans.transType = CHIP_TRANS_V6;
    target->chipTrans.sysCnt = source->sysCnt;
    target->chipTrans.id = source->dieId;
    target->chipTrans.value1 = source->writeBandwidth;
    target->chipTrans.value2 = source->readBandwidth;
    return DEFAULT_CNT;
}

REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER, PARSER_ITEM_CHIP_TRANS, ChipTransParserItem);
REGISTER_PARSER_ITEM(SOC_PROFILE_PARSER_V6, PARSER_ITEM_CHIP_TRANS, ChipTransParserItemV6);

}  // namespace Domain
}  // namespace Analysis
