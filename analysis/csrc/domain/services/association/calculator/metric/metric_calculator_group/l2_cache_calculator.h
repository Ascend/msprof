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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct L2CacheConfig
{
    std::map<L2CacheIndex, Calculator> table;
    std::vector<double> floatBitVec;
};

// 各芯片的L2Cache配置：枚举项 -> 寄存器及计算公式
const L2CacheConfig L2_CACHE_CONFIG = {
    {{L2CacheIndex::WriteCacheHit, {{0x500}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::WriteCacheMissAllocate, {{0x502}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::R0ReadCacheHit, {{0x504}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::R0ReadCacheMissAllocate, {{0x506}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::R1ReadCacheHit, {{0x508}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::R1ReadCacheMissAllocate, {{0x50a}, Calculator::CalculatorMetricByNothing}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的L2Cache配置，寄存器0x424~0x42c
const L2CacheConfig L2_CACHE_CONFIG_V6 = {
    {{L2CacheIndex::ReadLocalL2Hit, {{0x424}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::ReadLocalL2Miss, {{0x425}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::ReadLocalL2Victim, {{0x426}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::WriteLocalL2Hit, {{0x42a}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::WriteLocalL2Miss, {{0x42b}, Calculator::CalculatorMetricByNothing}},
     {L2CacheIndex::WriteLocalL2Victim, {{0x42c}, Calculator::CalculatorMetricByNothing}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};

// 按芯片选择配置
const L2CacheConfig& SelectL2CacheConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return L2_CACHE_CONFIG_V6;
        default:
            return L2_CACHE_CONFIG;
    }
}
}  // namespace

class L2CacheCalculator : public MetricCalculator
{
   public:
    explicit L2CacheCalculator(ChipId chipId = CHIP_ID_ALL) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectL2CacheConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectL2CacheConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectL2CacheConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H
