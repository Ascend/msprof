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
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_QOS_PARSER_ITEM_H
#define MSPROF_ANALYSIS_QOS_PARSER_ITEM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
int QosV4ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t expandStatus);
int QosV6ParseItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *halData, uint16_t expandStatus);
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_QOS_PARSER_ITEM_H
