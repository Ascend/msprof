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

#ifndef ANALYSIS_DOMAIN_BLOCK_DETAIL_DATA_H
#define ANALYSIS_DOMAIN_BLOCK_DETAIL_DATA_H

#include <string>

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis
{
namespace Domain
{

struct BlockDetailData : public BasicData
{
    uint16_t deviceId = UINT16_MAX;
    uint16_t coreId = UINT16_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t subtaskId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint64_t coreType = UINT64_MAX;
    uint64_t fakeEndTime = 0;
    double duration = 0.0;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_BLOCK_DETAIL_DATA_H
