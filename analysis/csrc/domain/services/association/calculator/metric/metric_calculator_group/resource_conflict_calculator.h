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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;

namespace
{
struct ResourceConflictConfig
{
    std::map<ResourceConflictIndex, Calculator> table;
    std::vector<double> floatBitVec;
};

// ResourceConflict配置：枚举项 -> 寄存器及计算公式
const ResourceConflictConfig RESOURCE_CONFLICT_CONFIG = {
    {{ResourceConflictIndex::VecBankGroupCfltRatio, {{0x64}, Calculator::CalculatorMetricByAdditions}},
     {ResourceConflictIndex::VecBankCfltRatio, {{0x65}, Calculator::CalculatorMetricByAdditions}},
     {ResourceConflictIndex::VecRescCfltRatio, {{0x66}, Calculator::CalculatorMetricByAdditions}}},
    {1.0, 1.0, 1.0}};

// V6芯片（CHIP_V6_1_0/CHIP_V6_2_0）的ResourceConflict配置:
// vec_bank_cflt_ratio = (r540 + r556) / task_cyc, vec_resc_cflt_ratio = r528 / r502
const ResourceConflictConfig RESOURCE_CONFLICT_CONFIG_V6 = {
    {{ResourceConflictIndex::VecBankCfltRatio, {{0x540, 0x556}, Calculator::CalculatorMetricByAdditions}},
     {ResourceConflictIndex::VecRescCfltRatio, {{0x528, 0x502}, Calculator::CalculatorMetricByDivision}}},
    {1.0, 1.0}};

// 按芯片选择配置
const ResourceConflictConfig& SelectResourceConflictConfig(ChipId chipId)
{
    switch (chipId)
    {
        case CHIP_V6_1_0:
        case CHIP_V6_2_0:
            return RESOURCE_CONFLICT_CONFIG_V6;
        default:
            return RESOURCE_CONFLICT_CONFIG;
    }
}
}  // namespace

class ResourceConflictCalculator : public MetricCalculator
{
   public:
    explicit ResourceConflictCalculator(ChipId chipId) : chipId_(chipId) {}

    std::vector<std::string> GetPmuHeader() override
    {
        return GetPmuHeaderBySubType(SelectResourceConflictConfig(chipId_).table);
    }

    bool CheckMetricEventValid(std::vector<uint32_t>& event) override
    {
        return CheckMetricEventBySubType(SelectResourceConflictConfig(chipId_).table, event);
    }

   private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        const auto& config = SelectResourceConflictConfig(chipId_);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, config.floatBitVec);
        res = CalculatePmu(pmuData, config.table, allParams);
        return res;
    }

   private:
    ChipId chipId_{CHIP_ID_ALL};
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H
