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

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

#include <algorithm>

#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Infra;
double Calculator::CalculatorTotalTime(uint64_t totalCycle, uint64_t blockNum, uint64_t coreNum, uint64_t freq)
{
    if (!blockNum || !coreNum || !freq)
    {
        return 0.0;
    }
    auto res = static_cast<double>(totalCycle) * FREQ_TO_Hz / freq / blockNum * ((blockNum + coreNum - 1) / coreNum);
    return res;
}

double Calculator::CalculatorMetricByAdditions(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0)
    {
        for (size_t i = 0; i < allParams.pmuList.size(); ++i)
        {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
        }
    }
    return res;
}

double Calculator::CalculatorMetricByDivision(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc == 0)
    {
        return res;
    }
    for (size_t i = 0; i < allParams.pmuList.size(); ++i)
    {
        if (allParams.pmuList[i] == 0)
        {
            return 0.0;
        }
        if (i == 0)
        {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
        }
        else
        {
            if (!Utils::IsDoubleEqual((*allParams.floatBit)[index], 0.0))
            {
                res /= ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
            }
        }
    }
    return res;
}

double Calculator::CalculatorMetricByNothing(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    for (const auto& value : allParams.pmuList)
    {
        res += value;
    }
    return res;
}

double Calculator::CalculatorMetricByAdditionsWithFreq(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0 && allParams.bwFreq != 0)
    {
        double freq = static_cast<double>(allParams.bwFreq);
        for (size_t i = 0; i < allParams.pmuList.size(); ++i)
        {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.pipSize)[index] *
                    (*allParams.scalar)[index] / (allParams.taskCyc / freq) / OFFSET);
        }
    }
    return res;
}

double Calculator::CalculatorMetricByAdditionsWithFreqScales(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0 && allParams.bwFreq != 0)
    {
        double freq = static_cast<double>(allParams.bwFreq);
        for (size_t i = 0; i < allParams.pmuList.size(); ++i)
        {
            double scale = 1.0;
            if (allParams.registerScales != nullptr && i < allParams.registerScales->size())
            {
                scale = (*allParams.registerScales)[i];
            }
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * scale * (*allParams.pipSize)[index] *
                    (*allParams.scalar)[index] / (allParams.taskCyc / freq) / OFFSET);
        }
    }
    return res;
}

double Calculator::CalculatorTimeByMultiplication(CalculationElements& allParams, size_t index)
{
    return allParams.totalTime * CalculatorMetricByAdditions(allParams, index);
}

double Calculator::CalculatorCubeFops(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0)
    {
        if (allParams.pmuList.size() > allParams.cubeParams->size())
        {
            ERROR("The cube params count is small than the pmu count, can't calculator!");
            return res;
        }
        for (size_t i = 0; i < allParams.pmuList.size(); ++i)
        {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.cubeParams)[i]);
        }
    }
    return res;
}

double Calculator::CalculatorVectorFops(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0)
    {
        if (allParams.pmuList.size() > allParams.vectorParams->size())
        {
            ERROR("The vector params count is small than the pmu count, can't calculator!");
            return res;
        }
        for (size_t i = 0; i < allParams.pmuList.size(); ++i)
        {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.vectorParams)[i]);
        }
    }
    return res;
}

double Calculator::CalculateMetricWithoutCycByAdd(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    for (size_t i = 0; i < allParams.pmuList.size(); ++i)
    {
        res += (*allParams.floatBit)[index] * static_cast<double>(allParams.pmuList[i]);
    }
    return res;
}

double Calculator::CalculateMetricWithoutCycBySub(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    double tmpRes;
    for (size_t i = 0; i < allParams.pmuList.size(); ++i)
    {
        tmpRes = (*allParams.floatBit)[index] * static_cast<double>(allParams.pmuList[i]);
        if (i == 0)
        {
            res += tmpRes;
        }
        else
        {
            res -= tmpRes;
        }
    }
    return res;
}

std::unordered_map<uint32_t, uint64_t> MetricCalculator::GetValueMappingOffset(CalculationElements& params,
                                                                               HalPmuData& pmuData)
{
    std::unordered_map<uint32_t, uint64_t> res;
    if (params.events.size() > pmuData.pmu.pmuList.size())
    {
        ERROR("PMU length is not equal with pmu events, please check");
        return res;
    }
    // 只配对较短列表的长度
    size_t pairCount = std::min(params.events.size(), pmuData.pmu.pmuList.size());
    for (size_t i = 0; i < pairCount; ++i)
    {
        res.insert({params.events[i], pmuData.pmu.pmuList[i]});
    }
    return res;
}

void SetBlockNum(DeviceTask& task, CalculationElements& allParams, HalPmuData& pmu)
{
    if (pmu.type == PMU)
    {
        allParams.blockNum = task.blockNum;
    }
    else
    {
        allParams.blockNum = task.mixBlockNum;
    }
}

uint64_t GetFreq(std::vector<HalFreqLpmData>& freqData, uint64_t freq, HalPmuData& pmu)
{
    std::sort(freqData.begin(), freqData.end(),
              [](HalFreqLpmData& lData, HalFreqLpmData& rData) { return lData.sysCnt < rData.sysCnt; });
    bool hasZeroFreq =
        std::any_of(freqData.begin(), freqData.end(), [](HalFreqLpmData& data) { return data.freq == 0; });
    if (hasZeroFreq)
    {
        return freq;
    }
    auto res = freq;
    for (auto& data : freqData)
    {
        if (pmu.pmu.timeList[1] < data.sysCnt)
        {
            break;
        }
        res = data.freq * FREQ_TO_Hz;
    }
    return res != 0 ? res : freq;
}

std::vector<double> MetricCalculator::CalculatePmuMetric(DataInventory& dataInventory, const DeviceContext& context,
                                                         CalculationElements& params, HalPmuData& pmu, DeviceTask& task)
{
    DeviceInfo deviceInfo{};
    context.Getter(deviceInfo);
    SampleInfo info;
    context.Getter(info);
    uint64_t freq = deviceInfo.aicFrequency * FREQ_TO_Hz;
    params.bwFreq = freq;
    // chip4的PMU计算需要判断解析的频率信息
    if (context.GetChipID() == CHIP_V4_1_0)
    {
        auto freqData = dataInventory.GetPtr<std::vector<HalFreqLpmData>>();
        if (freqData != nullptr)
        {
            freq = GetFreq(*freqData, freq, pmu);
        }
    }
    params.timeFreq = freq;
    params.taskCyc = pmu.pmu.totalCycle;
    if (!Utils::Resize(params.events, info.aiCoreProfilingEvents.size()))
    {
        ERROR("pmu events resize failed!");
        return {};
    }
    if (pmu.pmu.acceleratorType == MIX_AIC || pmu.pmu.acceleratorType == AIC)
    {
        if (pmu.type == PMU)
        {
            std::copy(info.aiCoreProfilingEvents.begin(), info.aiCoreProfilingEvents.end(), params.events.begin());
        }
        else
        {
            std::copy(info.aivProfilingEvents.begin(), info.aivProfilingEvents.end(), params.events.begin());
        }
        params.coreNum = pmu.type == PMU ? deviceInfo.aiCoreNum : deviceInfo.aivNum;
    }
    else
    {
        if (pmu.type == PMU)
        {
            std::copy(info.aivProfilingEvents.begin(), info.aivProfilingEvents.end(), params.events.begin());
        }
        else
        {
            std::copy(info.aiCoreProfilingEvents.begin(), info.aiCoreProfilingEvents.end(), params.events.begin());
        }
        params.coreNum = pmu.type == PMU ? deviceInfo.aivNum : deviceInfo.aiCoreNum;
    }
    SetBlockNum(task, params, pmu);
    return SetAllParamsAndCalculator(params, context, pmu);
}
}  // namespace Domain
}  // namespace Analysis
