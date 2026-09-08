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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct MemoryL0Config
{
    std::map<MemoryL0Index, Calculator> table;
    std::vector<double> floatBitVec;
    std::vector<double> pipeSizeVec;
    std::vector<double> scalarVec;
};

// MemoryL0配置：枚举项 -> 寄存器及计算公式
const MemoryL0Config MEMORY_L0_CONFIG = {
    {{MemoryL0Index::L0aReadBw, {{0x1b}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0aWriteBw, {{0x1c}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0bReadBw, {{0x21}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0bWriteBw, {{0x22}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cReadBw, {{0x27}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cWriteBw, {{0x29}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cReadBwCube, {{0x28}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cWriteBwCube, {{0x2a}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 256.0, 256.0, 256.0, 256.0, 256.0},
    {16.0, 16.0, 16.0, 8.0, 8.0, 8.0, 32.0, 32.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的MemoryL0配置
const MemoryL0Config MEMORY_L0_CONFIG_V6 = {
    {{MemoryL0Index::L0aReadBw, {{0x304}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0aWriteBw, {{0x703}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0bReadBw, {{0x306}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0bWriteBw, {{0x705}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cReadBw, {{0x712}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cReadBwCube, {{0x30a}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryL0Index::L0cWriteBwCube, {{0x308}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 256.0, 256.0, 256.0, 256.0},
    {16.0, 16.0, 8.0, 8.0, 8.0, 8.0, 8.0}};

// 按芯片选择配置
const MemoryL0Config& SelectMemoryL0Config(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return MEMORY_L0_CONFIG_V6;
        default:
            return MEMORY_L0_CONFIG;
    }
}
}  // namespace

class MemoryL0Calculator : public MetricCalculator
{
   public:
    explicit MemoryL0Calculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectMemoryL0Config(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectMemoryL0Config(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectMemoryL0Config(chipId_);
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
#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H
