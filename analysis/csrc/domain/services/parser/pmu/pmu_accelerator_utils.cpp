/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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

#include "analysis/csrc/domain/services/parser/pmu/pmu_accelerator_utils.h"

namespace Analysis
{
namespace Domain
{
AcceleratorType GetAcceleratorTypeByType(uint16_t subTaskType, uint16_t fftsType)
{
    if (fftsType == FFTS_PLUS)
    {
        if (subTaskType == TASK_AIC)
        {
            return AcceleratorType::MIX_AIC;
        }
        else if (subTaskType == TASK_AIV)
        {
            return AcceleratorType::MIX_AIV;
        }
    }
    if (fftsType == NORMAL_TASK || (fftsType == FFTS_PLUS && subTaskType == NORMAL_TASK))
    {
        return AcceleratorType::AIC;
    }
    return AcceleratorType::AIV;
}
}  // namespace Domain
}  // namespace Analysis
