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

#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/arith_metric_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/l2_cache_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_access_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_l0_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_ub_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeut_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeutext_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/resource_conflict_calculator.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"

namespace Analysis
{
namespace Domain
{
using namespace Utils;

std::unordered_map<AicMetricsEventsType, Creator> MetricCalculatorFactory::aicEvent{
    {AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<ArithMetricCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_PIPE_UTILIZATION,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<PipeUtCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<PipeUtExtCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_MEMORY, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_MEMORY_L0, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryL0Calculator>(chipId); }},
    {AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<ResourceConflictCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_MEMORY_UB, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryUBCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_L2_CACHE, [](ChipId chipId) { return MAKE_UNIQUE_PTR<L2CacheCalculator>(chipId); }},
    {AicMetricsEventsType::AIC_MEMORY_ACCESS,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryAccessCalculator>(chipId); }},
};

std::unordered_map<AivMetricsEventsType, Creator> MetricCalculatorFactory::aivEvent{
    {AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<ArithMetricCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_PIPE_UTILIZATION,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<PipeUtCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_MEMORY, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_MEMORY_L0, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryL0Calculator>(chipId); }},
    {AivMetricsEventsType::AIV_RESOURCE_CONFLICT_RATIO,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<ResourceConflictCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_MEMORY_UB, [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryUBCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_L2_CACHE, [](ChipId chipId) { return MAKE_UNIQUE_PTR<L2CacheCalculator>(chipId); }},
    {AivMetricsEventsType::AIV_MEMORY_ACCESS,
     [](ChipId chipId) { return MAKE_UNIQUE_PTR<MemoryAccessCalculator>(chipId); }},
};
}  // namespace Domain
}  // namespace Analysis
