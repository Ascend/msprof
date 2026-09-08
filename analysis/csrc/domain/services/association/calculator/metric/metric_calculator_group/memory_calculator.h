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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct MemoryConfig
{
    std::map<MemoryIndex, Calculator> table;
    std::vector<double> floatBitVec;
    std::vector<double> pipeSizeVec;
    std::vector<double> scalarVec;
    std::vector<double> registerScales;
};

// Memory配置：枚举项 -> 寄存器及计算公式
const MemoryConfig MEMORY_CONFIG = {
    {{MemoryIndex::UBReadBw, {{0x15}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::UBWriteBw, {{0x16}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1ReadBw, {{0x31}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1WriteBw, {{0x32}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemReadBw, {{0x12}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemWriteBw, {{0x13}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L2ReadBw, {{0xf}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L2WriteBw, {{0x10}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 128.0, 8.0, 8.0, 256.0, 256.0},
    {4.0, 4.0, 16.0, 8.0, 8.0, 8.0, 8.0, 8.0},
    {}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的Memory配置
const MemoryConfig MEMORY_CONFIG_V6 = {
    {{MemoryIndex::UBReadBw, {{0x56f, 0x571}, Calculator::CalculatorMetricByAdditionsWithFreqScales}},
     {MemoryIndex::UBWriteBw, {{0x570}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1ReadBw, {{0x707}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1WriteBw, {{0x709}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemReadBw, {{0x400}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemWriteBw, {{0x401}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 256.0, 8.0, 8.0},
    {4.0, 4.0, 16.0, 8.0, 8.0, 8.0},
    // UBReadBw各寄存器系数: r56f=1.0, r571=0.25(即128*2/(256*4))
    {1.0, 0.25}};

// 按芯片选择配置
const MemoryConfig& SelectMemoryConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return MEMORY_CONFIG_V6;
        default:
            return MEMORY_CONFIG;
    }
}
}  // namespace

class MemoryCalculator : public MetricCalculator
{
   public:
    explicit MemoryCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectMemoryConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectMemoryConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectMemoryConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.pipSize, DoublePtrType, res, config.pipeSizeVec);
        MAKE_SHARED_RETURN_VALUE(allParams.scalar, DoublePtrType, res, config.scalarVec);
        MAKE_SHARED_RETURN_VALUE(allParams.registerScales, DoublePtrType, res, config.registerScales);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
