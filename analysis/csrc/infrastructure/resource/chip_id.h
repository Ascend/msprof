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
#ifndef ANALYSIS_INFRASTRUCTURE_RESOURCE_CHIP_ID_H
#define ANALYSIS_INFRASTRUCTURE_RESOURCE_CHIP_ID_H

#include <cstdint>

namespace Analysis
{

enum ChipId : uint32_t
{
    CHIP_V1_1_0 = 0,
    CHIP_V2_1_0 = 1,
    CHIP_V3_1_0 = 2,
    CHIP_V3_2_0 = 3,
    CHIP_V3_3_0 = 4,
    CHIP_V4_1_0 = 5,

    CHIP_V1_1_1 = 7,
    CHIP_V1_1_2 = 8,
    CHIP_V5_1_0 = 9,

    CHIP_V1_1_3 = 11,
    CHIP_V6_1_0 = 15,
    CHIP_V6_2_0 = 16,

    CHIP_ID_ALL = UINT32_MAX  // 所有芯片均有此流程
};

}

#endif
