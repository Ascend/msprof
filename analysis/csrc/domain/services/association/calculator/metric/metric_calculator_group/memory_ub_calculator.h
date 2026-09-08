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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct MemoryUBConfig
{
    std::map<MemoryUBIndex, Calculator> table;
    std::vector<double> floatBitVec;
    std::vector<double> pipeSizeVec;
    std::vector<double> scalarVec;
};

// MemoryUB配置：枚举项 -> 寄存器及计算公式
const MemoryUBConfig MEMORY_UB_CONFIG = {
    {{MemoryUBIndex::UbReadBwVector, {{0x43}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbWriteBwVector, {{0x44}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbReadBwScalar, {{0x37}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbWriteBwScalar, {{0x38}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0},
    {128.0, 128.0, 128.0, 128.0},
    {2.0, 2.0, 1.0, 1.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的MemoryUB配置
// MemoryUB场景下ub_read_bw_mte/ub_write_bw_mte列在建表时即被移除，故此处不计算、不输出mte相关指标
const MemoryUBConfig MEMORY_UB_CONFIG_V6 = {
    {{MemoryUBIndex::UbReadBwVector, {{0x571}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbWriteBwVector, {{0x572}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbReadBwScalar, {{0x3}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::UbWriteBwScalar, {{0x5}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryUBIndex::Fixp2UbWriteBw, {{0x70c}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0},
    {128.0, 128.0, 128.0, 128.0, 256.0},
    {2.0, 2.0, 1.0, 1.0, 8.0}};

// 按芯片选择配置
const MemoryUBConfig& SelectMemoryUBConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return MEMORY_UB_CONFIG_V6;
        default:
            return MEMORY_UB_CONFIG;
    }
}
}  // namespace

class MemoryUBCalculator : public MetricCalculator
{
   public:
    explicit MemoryUBCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectMemoryUBConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectMemoryUBConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectMemoryUBConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.pipSize, DoublePtrType, res, config.pipeSizeVec);
        MAKE_SHARED_RETURN_VALUE(allParams.scalar, DoublePtrType, res, config.scalarVec);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H
