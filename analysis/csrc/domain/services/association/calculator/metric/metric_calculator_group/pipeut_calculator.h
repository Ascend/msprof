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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct PipeUtConfig
{
    std::map<PipeLineUtIndex, Calculator> table;
    std::vector<double> floatBitVec;
};

// PipeUt配置：枚举项 -> 寄存器及计算公式
const PipeUtConfig PIPE_UT_CONFIG = {
    {{PipeLineUtIndex::VecRatio, {{0x8}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::VecTime, {{0x8}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::MacRatio, {{0xa}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::MacTime, {{0xa}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::ScalarRatio, {{0x9}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::ScalarTime, {{0x9}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte1Ratio, {{0xb}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte1Time, {{0xb}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte2Ratio, {{0xc}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte2Time, {{0xc}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte3Ratio, {{0xd}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte3Time, {{0xd}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::ICacheMissRate, {{0x55, 0x54}, Calculator::CalculatorMetricByDivision}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的PipeUt配置:
// 0x501=vec_ratio, 0x301=mac_ratio, 0x1=scalar_ratio,
// 0x701=mte1_ratio, 0x202=mte2_ratio, 0x203=mte3_ratio,
// 0x714=fixpipe_ratio, 0x35/0x34=icache_miss_rate
const PipeUtConfig PIPE_UT_CONFIG_V6 = {
    {{PipeLineUtIndex::VecRatio, {{0x501}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::VecTime, {{0x501}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::MacRatio, {{0x301}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::MacTime, {{0x301}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::ScalarRatio, {{0x1}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::ScalarTime, {{0x1}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte1Ratio, {{0x701}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte1Time, {{0x701}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte2Ratio, {{0x202}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte2Time, {{0x202}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::Mte3Ratio, {{0x203}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::Mte3Time, {{0x203}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::FixPipeRatio, {{0x714}, Calculator::CalculatorMetricByAdditions}},
     {PipeLineUtIndex::FixPipeTime, {{0x714}, Calculator::CalculatorTimeByMultiplication}},
     {PipeLineUtIndex::ICacheMissRate, {{0x35, 0x34}, Calculator::CalculatorMetricByDivision}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};

// 按芯片选择配置
const PipeUtConfig& SelectPipeUtConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return PIPE_UT_CONFIG_V6;
        default:
            return PIPE_UT_CONFIG;
    }
}
}  // namespace

class PipeUtCalculator : public MetricCalculator
{
   public:
    explicit PipeUtCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectPipeUtConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectPipeUtConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectPipeUtConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H
