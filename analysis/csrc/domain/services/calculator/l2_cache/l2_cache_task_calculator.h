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

#ifndef ANALYSIS_DOMAIN_SERVICES_CALCULATOR_L2_CACHE_L2_CACHE_TASK_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_CALCULATOR_L2_CACHE_L2_CACHE_TASK_CALCULATOR_H

#include <cstdint>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{
namespace Domain
{
constexpr int L2_CACHE_DECIMAL_ACCURACY = 6;

struct L2CacheSummaryRow
{
    uint32_t taskType{0};
    uint32_t streamId{0};
    uint32_t taskId{0};
    double hitRate{0.0};
    double victimRate{0.0};
};

class L2CacheTaskCalculator : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_CALCULATOR_L2_CACHE_L2_CACHE_TASK_CALCULATOR_H
