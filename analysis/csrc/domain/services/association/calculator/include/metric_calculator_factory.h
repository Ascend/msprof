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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H

#include <functional>

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis
{
namespace Domain
{
using Creator = std::function<std::unique_ptr<MetricCalculator>(ChipId)>;

class MetricCalculatorFactory
{
   public:
    // 芯片信息由调用方传入：DeviceContext::Instance()为thread_local，计算器在worker线程创建时无法获取正确芯片信息
    static std::unique_ptr<MetricCalculator> GetAicCalculator(AicMetricsEventsType type, ChipId chipId = CHIP_ID_ALL)
    {
        auto it = aicEvent.find(type);
        if (it != aicEvent.end())
        {
            return it->second(chipId);
        }
        return nullptr;
    }

    static std::unique_ptr<MetricCalculator> GetAivCalculator(AivMetricsEventsType type, ChipId chipId = CHIP_ID_ALL)
    {
        auto it = aivEvent.find(type);
        if (it != aivEvent.end())
        {
            return it->second(chipId);
        }
        return nullptr;
    }

   private:
    static std::unordered_map<AicMetricsEventsType, Creator> aicEvent;
    static std::unordered_map<AivMetricsEventsType, Creator> aivEvent;
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H
