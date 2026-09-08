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

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_CHIP6_PMU_PARSER_ITEM_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_CHIP6_PMU_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{

#define PARSER_ITEM_V6_CONTEXT_PMU 0b101010
// V6的Block PMU记录标签('101001')
#define PARSER_ITEM_V6_BLOCK_PMU 0b101001
#define V6_PMU_LENGTH 10

#pragma pack(1)
struct Chip6Pmu
{
    uint16_t funcType : 6;  // 第1个，16位数据的低6位
    uint16_t cnt : 4;
    uint16_t resv1 : 6;  // resv字段为解析完整大小结构占位需要，实际未使用
    uint16_t resv2;
    uint32_t taskId;                  // 第5个，32位唯一id
    uint64_t totalCycle;              // 第6个，64位数据
    uint8_t ctxType;                  // 第7个，8位数据
    uint8_t flags;                    // 第8个，8位数据 bit0:mst bit1:mix bit2:ov_flag
    uint16_t resv8;                   // 第9个，16位数据
    uint8_t coreType;                 // 第10个，8位数据最低1位
    uint8_t coreId;                   // 第11个，8位数据
    uint16_t resv11;                  // 第12个，16位数据
    uint16_t subBlockId;              // 第13个，16位数据
    uint16_t blockId;                 // 第14个，16位数据
    uint32_t resv14;                  // 第15个，32位数据
    uint64_t pmuList[V6_PMU_LENGTH];  // 第16-25个，64位数据
    uint64_t startTime;               // 第26个，64位数据
    uint64_t endTime;                 // 第27个，64位数据
};
#pragma pack()

int Chip6PmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);
int Chip6BlockPmuParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halUniData, uint16_t expandStatus);

}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_CHIP6_PMU_PARSER_ITEM_H
