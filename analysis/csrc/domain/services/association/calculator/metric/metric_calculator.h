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

#ifndef ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/entities/metric/include/metric.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;
const uint64_t OFFSET = 1LL << 33;  // 2^33,pmu计算规则里带有BW的计算需要除以2^33
const uint64_t FREQ_TO_Hz = 1000000;
const uint8_t AIV_CORE_TYPE = 1;
const uint8_t AIC_CORE_TYPE = 0;
struct CalculationElements
{
    uint32_t blockNum{0};
    uint32_t coreNum{0};
    uint64_t taskCyc{0};
    uint64_t bwFreq{0};    // 这个频率为从info.json中读出来的aic_frequency
    uint64_t timeFreq{0};  // 这个频率需要考虑变频，如果有变频数据，则使用变频数据的频率，没有就是有aic_frequency
    double totalTime{0.0};
    double overrideTotalTime{0.0};  // 外部指定的totalTime，非0时覆盖PMU周期公式计算
    std::vector<uint32_t> events;
    std::vector<uint64_t> pmuList;
    std::shared_ptr<std::vector<double>> floatBit;
    std::shared_ptr<std::vector<double>> pipSize;
    std::shared_ptr<std::vector<double>> scalar;
    std::shared_ptr<std::vector<uint64_t>> cubeParams;
    std::shared_ptr<std::vector<uint64_t>> vectorParams;
    // 同一指标内各寄存器(pmuList按位置)的独立缩放系数，未设置或越界时按1.0处理，
    // 用于不同寄存器换算系数不同的场景，配合CalculatorMetricByAdditionsWithFreqScales使用
    std::shared_ptr<std::vector<double>> registerScales;
};

using DoublePtrType = std::vector<double>;
using IntPtrType = std::vector<uint64_t>;

class Calculator
{
   public:
    Calculator(std::vector<uint32_t> registers, std::function<double(CalculationElements &params, size_t index)> func)
        : registers(std::move(registers)), calculatorFunc(std::move(func))
    {
    }
    static double CalculatorMetricByNothing(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByAdditions(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByDivision(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByAdditionsWithFreq(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByAdditionsWithFreqScales(CalculationElements &allParams, size_t index);
    static double CalculatorTimeByMultiplication(CalculationElements &allParams, size_t index);
    static double CalculatorCubeFops(CalculationElements &allParams, size_t index);
    static double CalculatorVectorFops(CalculationElements &allParams, size_t index);
    static double CalculatorTotalTime(uint64_t totalCycle, uint64_t blockNum, uint64_t coreNum, uint64_t freq);
    static double CalculateMetricWithoutCycByAdd(CalculationElements &allParams, size_t index);
    static double CalculateMetricWithoutCycBySub(CalculationElements &allParams, size_t index);

   public:
    std::vector<uint32_t> registers;
    std::function<double(CalculationElements &params, size_t index)> calculatorFunc;
};

class MetricCalculator
{
   public:
    template <typename T>
    std::vector<double> CalculatePmu(HalPmuData pmuData, std::map<T, Calculator> calTable, CalculationElements &params)
    {
        auto pmuTable = GetValueMappingOffset(params, pmuData);
        if (params.overrideTotalTime > 0.0)
        {
            params.totalTime = Utils::RoundToDecimalPlaces(params.overrideTotalTime, 2);
        }
        else
        {
            params.totalTime = Calculator::CalculatorTotalTime(pmuData.pmu.totalCycle, params.blockNum, params.coreNum,
                                                               params.timeFreq);
        }
        std::vector<double> res;
        if (calTable.size() != params.floatBit->size())
        {
            ERROR("pmu calculator params init error please check!");
            return res;
        }
        size_t index = 0;
        for (auto &cal : calTable)
        {
            params.pmuList.clear();
            for (size_t regIdx = 0; regIdx < cal.second.registers.size(); ++regIdx)
            {
                auto val = pmuTable[cal.second.registers[regIdx]];
                params.pmuList.push_back(val);
            }
            res.push_back(cal.second.calculatorFunc(params, index));
            ++index;
        }
        return res;
    }

    template <typename K, typename V>
    std::vector<std::string> GetPmuHeaderBySubType(const std::map<K, V> &mapTable)
    {
        std::vector<std::string> res;
        for (auto &it : mapTable)
        {
            res.emplace_back(Metric::GetMetricHeaderString(it.first));
        }
        return res;
    }

    template <typename K, typename V>
    bool CheckMetricEventBySubType(const std::map<K, V> &mapTable, std::vector<uint32_t> &event)
    {
        bool flag = true;
        for (const auto &it : mapTable)
        {
            flag = std::all_of(it.second.registers.begin(), it.second.registers.end(), [&event](uint32_t elem)
                               { return std::find(event.begin(), event.end(), elem) != event.end(); });
            if (!flag)
            {  // 如果有元素找不到，直接退出，不再继续判断
                return flag;
            }
        }
        return flag;
    }

    std::vector<double> CalculatePmuMetric(DataInventory &dataInventory, const DeviceContext &context,
                                           CalculationElements &allParams, HalPmuData &pmuData, DeviceTask &task);
    virtual ~MetricCalculator() = default;
    virtual std::vector<std::string> GetPmuHeader() = 0;
    virtual bool CheckMetricEventValid(std::vector<uint32_t> &event) = 0;

   private:
    virtual std::vector<double> SetAllParamsAndCalculator(CalculationElements &allParams, const DeviceContext &context,
                                                          HalPmuData &pmuData) = 0;
    static std::unordered_map<uint32_t, uint64_t> GetValueMappingOffset(CalculationElements &params,
                                                                        HalPmuData &pmuData);
};
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H
