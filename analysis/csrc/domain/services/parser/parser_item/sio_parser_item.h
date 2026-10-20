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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_SIO_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_SIO_PARSER_ITEM_H

#include <cstdint>

#include "analysis/csrc/domain/entities/hal/include/hal_log.h"

namespace Analysis
{
namespace Domain
{

constexpr uint32_t PARSER_ITEM_SIO = 0b011001;

#pragma pack(1)
// V6 结构：V6_SIO_FMT = "BBHIQQ10I"
struct SioV6RawData
{
    uint8_t funcType;
    uint8_t accIdByte;  // [7:2]: accId；[1:0]: 通用头 cnt 的高两位，不属于 accId。
    uint16_t magicNum;
    uint32_t resv1;
    uint64_t sysCnt;
    uint64_t resv2;
    uint32_t resv3;
    uint32_t reqRx;
    uint32_t rspRx;
    uint32_t snpRx;
    uint32_t datRx;
    uint32_t resv4;
    uint32_t reqTx;
    uint32_t rspTx;
    uint32_t snpTx;
    uint32_t datTx;
};

// 非 V6 结构：SIO_FMT = "HHIQHH11I"
struct SioV4RawData
{
    uint16_t funcType : 6;
    uint16_t cnt : 4;
    uint16_t accType : 6;
    uint16_t magicNum;
    uint32_t resv1;
    uint64_t sysCnt;
    uint16_t resv2;
    uint16_t accId;
    uint32_t resv3[3];
    uint32_t reqRx;
    uint32_t rspRx;
    uint32_t snpRx;
    uint32_t datRx;
    uint32_t reqTx;
    uint32_t rspTx;
    uint32_t snpTx;
    uint32_t datTx;
};
#pragma pack()

int SioParserItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);
int SioParserItemV6(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_PARSER_ITEM_SIO_PARSER_ITEM_H
