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

#ifndef MSPROF_ANALYSIS_HAL_HBM_H
#define MSPROF_ANALYSIS_HAL_HBM_H

#include <cstdint>

namespace Analysis
{
namespace Domain
{
enum HbmParserItemType : uint32_t
{
    HBM_V1_ITEM_TYPE = 0,
    HBM_V2_ITEM_TYPE = 1
};

struct HalHbmData
{
    uint64_t timestamp{};
    uint64_t count{};
    uint32_t eventId{};
    uint32_t hbmId{};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // MSPROF_ANALYSIS_HAL_HBM_H
