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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct PipeUtExtConfig
{
    std::map<PipeUtilizationExctIndex, Calculator> table;
    std::vector<double> floatBitVec;
};

// PipeUtExt配置：枚举项 -> 寄存器及计算公式
const PipeUtExtConfig PIPE_UT_EXT_CONFIG = {
    {{PipeUtilizationExctIndex::MacRatioExtra, {{0x416, 0x417}, Calculator::CalculatorMetricByAdditions}},
     {PipeUtilizationExctIndex::ScalarRatio, {{0x9}, Calculator::CalculatorMetricByAdditions}},
     {PipeUtilizationExctIndex::Mte1RatioExtra, {{0x302}, Calculator::CalculatorMetricByAdditions}},
     {PipeUtilizationExctIndex::Mte2Ratio, {{0xc}, Calculator::CalculatorMetricByAdditions}},
     {PipeUtilizationExctIndex::FixPipeRatio, {{0x303}, Calculator::CalculatorMetricByAdditions}},
     {PipeUtilizationExctIndex::ICacheMissRate, {{0x55, 0x54}, Calculator::CalculatorMetricByDivision}},
     {PipeUtilizationExctIndex::MacTime, {{0x416, 0x417}, Calculator::CalculatorTimeByMultiplication}},
     {PipeUtilizationExctIndex::ScalarTime, {{0x9}, Calculator::CalculatorTimeByMultiplication}},
     {PipeUtilizationExctIndex::Mte1Time, {{0x302}, Calculator::CalculatorTimeByMultiplication}},
     {PipeUtilizationExctIndex::Mte2Time, {{0xc}, Calculator::CalculatorTimeByMultiplication}},
     {PipeUtilizationExctIndex::FixPipeTime, {{0x303}, Calculator::CalculatorTimeByMultiplication}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};
}  // namespace

class PipeUtExtCalculator : public MetricCalculator
{
   public:
    explicit PipeUtExtCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override { return GetPmuHeaderBySubType(PIPE_UT_EXT_CONFIG.table); }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(PIPE_UT_EXT_CONFIG.table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, PIPE_UT_EXT_CONFIG.floatBitVec);
        res = CalculatePmu(pmuData, PIPE_UT_EXT_CONFIG.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H
