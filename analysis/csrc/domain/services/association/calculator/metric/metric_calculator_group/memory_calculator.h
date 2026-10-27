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
    {4.0, 4.0, 16.0, 8.0, 8.0, 8.0, 8.0, 8.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_1_1/CHIP_V6_2_0）的Memory配置
const MemoryConfig MEMORY_CONFIG_V6 = {
    {{MemoryIndex::UBReadBw, {{0x56f}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::UBWriteBw, {{0x570}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1ReadBw, {{0x707}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1WriteBw, {{0x709}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemReadBw, {{0x400}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemWriteBw, {{0x401}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 256.0, 8.0, 8.0},
    {4.0, 4.0, 16.0, 8.0, 8.0, 8.0}};

// V6新版本采集数据的Memory配置：main_mem事件地址变更为0x422/0x423，配套系数128*8
const MemoryConfig MEMORY_CONFIG_V6_NEW = {
    {{MemoryIndex::UBReadBw, {{0x56f}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::UBWriteBw, {{0x570}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1ReadBw, {{0x707}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::L1WriteBw, {{0x709}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemReadBw, {{0x422}, Calculator::CalculatorMetricByAdditionsWithFreq}},
     {MemoryIndex::MainMemWriteBw, {{0x423}, Calculator::CalculatorMetricByAdditionsWithFreq}}},
    {1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
    {256.0, 256.0, 256.0, 256.0, 128.0, 128.0},
    {4.0, 4.0, 16.0, 8.0, 8.0, 8.0}};

// 按芯片选择配置，V6按采集事件区分新老数据：0x422/0x423为新版本地址，配套新系数
const MemoryConfig& SelectMemoryConfig(ChipId chipId, const std::vector<uint32_t>& events = {})
{
    auto contains = [&events](uint32_t event)
    { return std::find(events.begin(), events.end(), event) != events.end(); };
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_1_1:
        case CHIP_V6_2_0:
            return contains(0x422) || contains(0x423) ? MEMORY_CONFIG_V6_NEW : MEMORY_CONFIG_V6;
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
        return CheckMetricEventBySubType(SelectMemoryConfig(chipId_, event).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectMemoryConfig(chipId_, allParams.events);
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

#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
