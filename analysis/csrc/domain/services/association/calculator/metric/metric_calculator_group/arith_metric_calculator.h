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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H

#include <algorithm>

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct ArithMetricConfig
{
    std::map<ArithMetricIndex, Calculator> table;
    std::vector<double> floatBitVec;
    std::vector<uint64_t> vectorParams;
    std::vector<uint64_t> cubeParams;
};

// ArithMetric配置：枚举项 -> 寄存器及计算公式
const ArithMetricConfig ARITH_METRIC_CONFIG = {
    {{ArithMetricIndex::MacFp16Ratio, {{0x49}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::MacInt8Ratio, {{0x4a}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecFp32Ratio, {{0x4b}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecFp16Ratio, {{0x4c, 0x4d}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecInt32Ratio, {{0x4e}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecMiscRatio, {{0x4f}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::CubeFops, {{0x49, 0x4a}, Calculator::CalculatorCubeFops}},
     {ArithMetricIndex::VectorFops, {{0x4c, 0x4d, 0x4b, 0x4e, 0x4f}, Calculator::CalculatorVectorFops}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {128, 64, 64, 64, 32},
    {8192, 16384}};

// V3系列芯片（CHIP_V3_1_0/CHIP_V3_2_0/CHIP_V3_3_0）的ArithMetric配置，寄存器同默认，仅vectorParams不同
const ArithMetricConfig ARITH_METRIC_CONFIG_CHIP3 = {
    {{ArithMetricIndex::MacFp16Ratio, {{0x49}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::MacInt8Ratio, {{0x4a}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecFp32Ratio, {{0x4b}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecFp16Ratio, {{0x4c, 0x4d}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecInt32Ratio, {{0x4e}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::VecMiscRatio, {{0x4f}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::CubeFops, {{0x49, 0x4a}, Calculator::CalculatorCubeFops}},
     {ArithMetricIndex::VectorFops, {{0x4c, 0x4d, 0x4b, 0x4e, 0x4f}, Calculator::CalculatorVectorFops}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {128, 64, 64, 16, 16},
    {8192, 16384}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的ArithMetric配置:
// event IDs: 0x323=mac_fp16_ratio, 0x324=mac_int8_ratio
// cube_fops = r323*16*16*16*2 + r324*16*16*32*2
const ArithMetricConfig ARITH_METRIC_CONFIG_V6 = {
    {{ArithMetricIndex::MacFp16Ratio, {{0x323}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::MacInt8Ratio, {{0x324}, Calculator::CalculatorMetricByAdditions}},
     {ArithMetricIndex::CubeFops, {{0x323, 0x324}, Calculator::CalculatorCubeFops}}},
    {1.0, 1.0, 1.0},
    {128, 64, 64, 64, 32},
    {8192, 16384}};

// 按芯片选择配置
const ArithMetricConfig& SelectArithMetricConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return ARITH_METRIC_CONFIG_V6;
        case CHIP_V3_1_0:
        case CHIP_V3_2_0:
        case CHIP_V3_3_0:
            return ARITH_METRIC_CONFIG_CHIP3;
        default:
            return ARITH_METRIC_CONFIG;
    }
}
}  // namespace

class ArithMetricCalculator : public MetricCalculator
{
   public:
    explicit ArithMetricCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectArithMetricConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectArithMetricConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectArithMetricConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.cubeParams, IntPtrType, res, config.cubeParams);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.vectorParams, IntPtrType, res, config.vectorParams);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H
