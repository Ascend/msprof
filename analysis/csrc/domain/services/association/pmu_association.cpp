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
#include "analysis/csrc/domain/services/association/include/pmu_association.h"

#include <algorithm>
#include <map>

#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
void PmuAssociation::SplitPmu(std::vector<HalPmuData>& pmuData)
{
    int context_size = 0;
    int block_size = 0;
    for (auto& pmu : pmuData)
    {
        if (pmu.type == PMU)
        {
            contextPmuTask_[pmu.hd.taskId].push_back(&pmu);
            ++context_size;
        }
        else
        {
            blockPmuTask_[pmu.hd.taskId].push_back(&pmu);
            ++block_size;
        }
    }
    INFO("context pmu count is : %", context_size);
    INFO("block pmu count is : %", block_size);
}

void BlockPmuResultAccumulate(std::vector<uint64_t>& dstVec, std::vector<uint64_t>& srcVec)
{
    if (dstVec.size() != srcVec.size())
    {
        if (!Resize(dstVec, srcVec.size()))
        {
            ERROR("BlockPmu accumulate failed when resize occurs exception");
        }
    }
    std::transform(srcVec.begin(), srcVec.end(), dstVec.begin(), dstVec.begin(), std::plus<uint64_t>());
}

void SetOverrideTotalTime(HalPmuData& pmuData, const DeviceContext& context, CalculationElements& params)
{
    if (pmuData.pmu.timeList[1] < pmuData.pmu.timeList[0])
    {
        ERROR("Invalid pmu time range: start=%, end=%, taskId is %", pmuData.pmu.timeList[0], pmuData.pmu.timeList[1],
              pmuData.hd.taskId.taskId);
        return;
    }
    DeviceInfo deviceInfo;
    context.Getter(deviceInfo);
    if (!IsDoubleEqual(deviceInfo.hwtsFrequency, 0.0))
    {
        params.overrideTotalTime =
            static_cast<double>(pmuData.pmu.timeList[1] - pmuData.pmu.timeList[0]) / deviceInfo.hwtsFrequency;
    }
}

void PmuAssociation::MergeContextPmuToDeviceTask(std::vector<HalPmuData*>& pmuVec, std::vector<DeviceTask>& deviceVec,
                                                 DataInventory& dataInventory, const DeviceContext& context,
                                                 uint64_t& filterEnd)
{
    size_t pmuIndex = 0;
    size_t taskIndex = 0;
    while (pmuIndex < pmuVec.size() && taskIndex < deviceVec.size())
    {
        if (pmuVec[pmuIndex]->hd.timestamp < deviceVec[taskIndex].taskStart)
        {  // pmu的时间比task开始小，取下一个pmu
            WARN("The pmu in % is earlier than task in %, taskId is %, streamId is %, contextId is %, batchId is %",
                 pmuIndex, taskIndex, pmuVec[pmuIndex]->hd.taskId.taskId, pmuVec[pmuIndex]->hd.taskId.streamId,
                 pmuVec[pmuIndex]->hd.taskId.contextId, pmuVec[pmuIndex]->hd.taskId.batchId);
            pmuIndex++;
            continue;
        }
        else if (pmuVec[pmuIndex]->hd.timestamp > deviceVec[taskIndex].taskEnd)
        {  // pmu时间比task结束大，取下一个task
            WARN("The pmu in % is later than task in %, taskId is %, streamId is %, contextId is %, batchId is %",
                 pmuIndex, taskIndex, pmuVec[pmuIndex]->hd.taskId.taskId, pmuVec[pmuIndex]->hd.taskId.streamId,
                 pmuVec[pmuIndex]->hd.taskId.contextId, pmuVec[pmuIndex]->hd.taskId.batchId);
            taskIndex++;
            continue;
        }
        if (context.isChipV6())
        {
            CalculateContextPmuV6(*pmuVec[pmuIndex], deviceVec[taskIndex], dataInventory, context);
        }
        else
        {
            CalculateContextPmu(*pmuVec[pmuIndex], deviceVec[taskIndex], dataInventory, context);
        }
        filterEnd = std::min(filterEnd, deviceVec[taskIndex].taskEnd);
        pmuIndex++;
        // V6 mix场景：同一task可能有多个PMU（AIC + AIV），仅当下一个PMU超出当前task范围时才推进taskIndex
        if (pmuIndex < pmuVec.size() && pmuVec[pmuIndex]->hd.timestamp > deviceVec[taskIndex].taskEnd)
        {
            taskIndex++;
        }
    }
    // V6 mix任务的AIC/AIV两侧各有一条context PMU，若仅收到一侧（丢数据/时钟抖动），mix结果只含单核数据，打WARN便于定位
    if (context.isChipV6())
    {
        for (const auto& task : deviceVec)
        {
            if (task.acceleratorType != MIX_AIC && task.acceleratorType != MIX_AIV)
            {
                continue;
            }
            auto* mixInfo = dynamic_cast<PmuInfoMixAccelerator*>(task.pmuInfo.get());
            if (mixInfo == nullptr)
            {
                continue;
            }
            if (mixInfo->aicPmuResult.empty())
            {
                WARN("Mix task missing AIC context pmu, taskStart is %, taskEnd is %", task.taskStart, task.taskEnd);
            }
            if (mixInfo->aivPmuResult.empty())
            {
                WARN("Mix task missing AIV context pmu, taskStart is %, taskEnd is %", task.taskStart, task.taskEnd);
            }
        }
    }
}

void PmuAssociation::CalculateContextPmu(HalPmuData& pmuData, DeviceTask& task, DataInventory& dataInventory,
                                         const DeviceContext& context)
{
    task.acceleratorType = pmuData.pmu.acceleratorType;
    CalculationElements params;
    // 始终取 context 侧 PMU 时间（hd.timestamp），换算成 wall-clock ns 后随 pmuInfo 供下游落盘
    uint64_t contextTimeNs = GetTimeFromSyscnt(pmuData.hd.timestamp, syscntParams_).Uint64();
    if (task.acceleratorType == MIX_AIC || task.acceleratorType == MIX_AIV)
    {
        PmuInfoMixAccelerator pmuInfoMix;
        if (pmuData.pmu.acceleratorType == MIX_AIC)
        {
            pmuInfoMix.aicTotalCycles = pmuData.pmu.totalCycle;
            auto res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
            pmuInfoMix.aicPmuResult.swap(res);
            pmuInfoMix.aiCoreTime = params.totalTime;
        }
        else
        {
            pmuInfoMix.aivTotalCycles = pmuData.pmu.totalCycle;
            auto res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
            pmuInfoMix.aivPmuResult.swap(res);
            pmuInfoMix.aivTime = params.totalTime;
        }
        pmuInfoMix.mainTimestamp = pmuData.pmu.timeList[1];
        pmuInfoMix.timestamp = contextTimeNs;
        task.pmuInfo = MAKE_UNIQUE_PTR<PmuInfoMixAccelerator>(pmuInfoMix);
    }
    else
    {
        PmuInfoSingleAccelerator pmuInfoNormal;
        pmuInfoNormal.totalCycles = pmuData.pmu.totalCycle;
        std::vector<double> res;
        if (pmuData.pmu.acceleratorType == AIC)
        {
            res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
        }
        else
        {
            res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
        }
        pmuInfoNormal.totalTime = params.totalTime;
        pmuInfoNormal.pmuResult.swap(res);
        pmuInfoNormal.timestamp = contextTimeNs;
        task.pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(pmuInfoNormal);
    }
}

void PmuAssociation::CalculateContextPmuV6(HalPmuData& pmuData, DeviceTask& task, DataInventory& dataInventory,
                                           const DeviceContext& context)
{
    task.acceleratorType = pmuData.pmu.acceleratorType;
    CalculationElements params;
    // 始终取 context 侧 PMU 时间（hd.timestamp），换算成 wall-clock ns 后随 pmuInfo 供下游落盘
    uint64_t contextTimeNs = GetTimeFromSyscnt(pmuData.hd.timestamp, syscntParams_).Uint64();
    if (task.acceleratorType == MIX_AIC || task.acceleratorType == MIX_AIV)
    {
        // mix场景：同一task有两个PMU（AIC+AIV），需要合并而非覆盖
        auto* existingMix = dynamic_cast<PmuInfoMixAccelerator*>(task.pmuInfo.get());
        PmuInfoMixAccelerator pmuInfoMix;
        if (existingMix)
        {
            pmuInfoMix = *existingMix;  // 保留已有数据（另一个核的PMU结果）
        }
        // 对齐Python chip6_pmu_calculator：主核(mst)用syscnt时间差，从核用totalCycle公式（从核AIC归零）
        const bool isMst = pmuData.pmu.mst == 1;
        if (isMst)
        {
            SetOverrideTotalTime(pmuData, context, params);
        }
        if (pmuData.pmu.acceleratorType == MIX_AIC)
        {
            pmuInfoMix.aicTotalCycles = pmuData.pmu.totalCycle;
            auto res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
            pmuInfoMix.aicPmuResult.swap(res);
            if (isMst)
            {
                pmuInfoMix.aiCoreTime = params.totalTime;
            }
            else
            {
                pmuInfoMix.aivTime = params.totalTime;
            }
        }
        else
        {
            pmuInfoMix.aivTotalCycles = pmuData.pmu.totalCycle;
            auto res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
            pmuInfoMix.aivPmuResult.swap(res);
            if (isMst)
            {
                pmuInfoMix.aivTime = params.totalTime;
            }
            else
            {
                pmuInfoMix.aiCoreTime = 0;
            }
        }
        pmuInfoMix.mainTimestamp = pmuData.pmu.timeList[1];
        pmuInfoMix.timestamp = contextTimeNs;
        task.pmuInfo = MAKE_UNIQUE_PTR<PmuInfoMixAccelerator>(pmuInfoMix);
    }
    else
    {
        // 非mix场景：使用syscnt时间差计算总时间
        PmuInfoSingleAccelerator pmuInfoNormal;
        pmuInfoNormal.totalCycles = pmuData.pmu.totalCycle;
        SetOverrideTotalTime(pmuData, context, params);
        std::vector<double> res;
        if (pmuData.pmu.acceleratorType == AIC)
        {
            res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
        }
        else
        {
            res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, pmuData, task);
        }
        pmuInfoNormal.totalTime = params.totalTime;
        pmuInfoNormal.pmuResult.swap(res);
        pmuInfoNormal.timestamp = contextTimeNs;
        task.pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(pmuInfoNormal);
    }
}

size_t PmuAssociation::MergeBlockPmuToDeviceTask(std::vector<HalPmuData*>& pmuData, DeviceTask& deviceTask,
                                                 DataInventory& dataInventory, const DeviceContext& context,
                                                 uint64_t& filterEnd)
{
    if (deviceTask.pmuInfo == nullptr)
    {
        ERROR("Block Pmu has no context Pmu taskId is %, streamId is %, contextId is %", pmuData[0]->hd.taskId.taskId,
              pmuData[0]->hd.taskId.streamId, pmuData[0]->hd.taskId.contextId);
        return pmuData.size();
    }
    uint32_t mixCount = deviceTask.mixBlockNum;
    uint8_t core_type = AIC_CORE_TYPE;
    if (deviceTask.acceleratorType == MIX_AIC)
    {
        core_type = AIV_CORE_TYPE;
    }
    CalculationElements params;
    HalPmuData tempPmuData;
    tempPmuData.type = BLOCK_PMU;
    auto res = dynamic_cast<PmuInfoMixAccelerator*>(deviceTask.pmuInfo.get());
    for (auto it = pmuData.begin(); it != pmuData.end();)
    {
        auto validFlag = (*it)->hd.timestamp >= deviceTask.taskStart && (*it)->hd.timestamp <= deviceTask.taskEnd;
        if ((*it)->pmu.acceleratorType == deviceTask.acceleratorType && (*it)->pmu.coreType == core_type && validFlag)
        {
            if (res->totalBlockCount >= mixCount)
            {
                break;
            }
            tempPmuData.hd.taskId = {(*it)->hd.taskId.streamId, (*it)->hd.taskId.batchId, (*it)->hd.taskId.taskId,
                                     (*it)->hd.taskId.contextId};
            res->totalBlockCount += 1;
            tempPmuData.pmu.timeList[1] = res->mainTimestamp;
            tempPmuData.pmu.totalCycle += (*it)->pmu.totalCycle;
            tempPmuData.pmu.acceleratorType = (*it)->pmu.acceleratorType;
            tempPmuData.pmu.coreType = (*it)->pmu.coreType;
            BlockPmuResultAccumulate(tempPmuData.pmu.pmuList, ((*it)->pmu.pmuList));
            it = pmuData.erase(it);
            filterEnd = std::min(filterEnd, deviceTask.taskStart);
        }
        else
        {
            ++it;
        }
    }
    if (core_type)
    {
        auto pmuRes = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, tempPmuData, deviceTask);
        res->aivTime = params.totalTime;
        res->aivPmuResult.swap(pmuRes);
        res->aivTotalCycles = tempPmuData.pmu.totalCycle;
    }
    else
    {
        auto pmuRes = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, tempPmuData, deviceTask);
        res->aiCoreTime = params.totalTime;
        res->aicPmuResult.swap(pmuRes);
        res->aicTotalCycles = tempPmuData.pmu.totalCycle;
    }
    return pmuData.size();
}

void PmuAssociation::AssociationByPmuType(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                          DataInventory& dataInventory, const DeviceContext& context)
{
    // 以匹配上的task的end时间为基准 和pmu的timestamp进行比较
    uint64_t filterEnd = UINT64_MAX;
    for (auto& pmu : contextPmuTask_)
    {
        auto it = deviceTask.find(pmu.first);
        if (it == deviceTask.end())
        {
            if (!pmu.second.empty() && filterEnd < pmu.second[0]->hd.timestamp)
            {
                ERROR("contextPmu has no matched log taskId is %, streamId is %, contextId is %, batchId is %",
                      pmu.first.taskId, pmu.first.streamId, pmu.first.contextId, pmu.first.batchId);
            }
            continue;
        }
        std::sort(pmu.second.begin(), pmu.second.end(),
                  [](HalPmuData* ld, HalPmuData* rd) { return ld->hd.timestamp < rd->hd.timestamp; });
        MergeContextPmuToDeviceTask(pmu.second, it->second, dataInventory, context, filterEnd);
    }
    INFO("Context PMU has been calculated!");
    for (auto& pmu : blockPmuTask_)
    {
        auto it = deviceTask.find(pmu.first);
        if (it == deviceTask.end())
        {
            if (!pmu.second.empty() && filterEnd < pmu.second[0]->hd.timestamp)
            {
                ERROR("blockPmu has no matched log taskId is %, streamId is %, contextId is %, batchId is %",
                      pmu.first.taskId, pmu.first.streamId, pmu.first.contextId, pmu.first.batchId);
            }
            continue;
        }
        std::sort(it->second.begin(), it->second.end(),
                  [](DeviceTask& lData, DeviceTask& rData) { return lData.taskStart < rData.taskStart; });
        size_t pmuIndex;
        for (auto& task : it->second)
        {
            if (task.acceleratorType == AIC || task.acceleratorType == AIV)
            {
                INFO("block pmu is used only by MIX, but context pmu is not MIX! No association is required.");
                continue;
            }
            pmuIndex = MergeBlockPmuToDeviceTask(pmu.second, task, dataInventory, context, filterEnd);
            if (pmuIndex == 0)
            {
                break;
            }
        }
    }
}

uint32_t PmuAssociation::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context)
{
    auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    SampleInfo sampleInfo;
    deviceContext.Getter(sampleInfo);
    auto deviceTask = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto pmuData = dataInventory.GetPtr<std::vector<HalPmuData>>();
    if (deviceTask->empty() || !pmuData || pmuData->empty())
    {
        WARN("There is no deviceTask or PMUData, don't need to associate!");
        return ANALYSIS_OK;
    }
    auto chipId = static_cast<ChipId>(deviceContext.GetChipID());
    aicCalculator_ = MetricCalculatorFactory::GetAicCalculator(sampleInfo.aiCoreMetrics, chipId);
    aivCalculator_ = MetricCalculatorFactory::GetAivCalculator(sampleInfo.aivMetrics, chipId);
    if (aicCalculator_ == nullptr || aivCalculator_ == nullptr)
    {
        ERROR("The value of aiv_metrics or ai_core_metrics is invalid!");
        return ANALYSIS_ERROR;
    }
    bool isChipV6 = deviceContext.isChipV6();
    if (!aicCalculator_->CheckMetricEventValid(sampleInfo.aiCoreProfilingEvents) ||
        !aivCalculator_->CheckMetricEventValid(sampleInfo.aivProfilingEvents))
    {
        ERROR("The PMU event does not meet the calculation requirements, please check");
        return ANALYSIS_ERROR;
    }
    // 非V6芯片：pmuList长度与events数量严格相等；V6芯片：pmuList固定为V6_PMU_LENGTH(10)，
    // events按位置与pmuList前N个计数器配对，数量可少于pmuList，但不可超出
    if (isChipV6)
    {
        if (sampleInfo.aiCoreProfilingEvents.size() > pmuData->back().pmu.pmuList.size() ||
            sampleInfo.aivProfilingEvents.size() > pmuData->back().pmu.pmuList.size())
        {
            ERROR("The size of PMU event is larger than pmu data size, please check");
            return ANALYSIS_ERROR;
        }
    }
    else if (pmuData->back().pmu.pmuList.size() != sampleInfo.aiCoreProfilingEvents.size() ||
             pmuData->back().pmu.pmuList.size() != sampleInfo.aivProfilingEvents.size())
    {
        ERROR("The size of PMU event is not equal with pmu data size, please check");
        return ANALYSIS_ERROR;
    }
    // V6芯片：用host task表中的streamId替换PMU原始streamId
    if (isChipV6)
    {
        auto streamIdInfo = dataInventory.GetPtr<HostStreamInfo>();
        if (streamIdInfo)
        {
            for (auto& pmu : *pmuData)
            {
                auto it = streamIdInfo->streamIdMap.find(pmu.hd.taskId.taskId);
                if (it != streamIdInfo->streamIdMap.end())
                {
                    pmu.hd.taskId.streamId = it->second;
                }
            }
        }
    }
    syscntParams_ = deviceContext.GetSyscntConversionParams();
    SplitPmu(*pmuData);
    if (deviceContext.GetChipID() != CHIP_V4_1_0)
    {
        // 仅V4芯片用block pmu做mix关联；V6芯片的mix由context pmu(MIX_AIC/MIX_AIV)完成，
        blockPmuTask_.clear();
        INFO("The acceleratorType of PMU is not MIX");
    }
    else
    {
        INFO("The acceleratorType of PMU is MIX");
    }
    AssociationByPmuType(*deviceTask, dataInventory, deviceContext);
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(PmuAssociation, true, LoadHostData, AscendTaskAssociation, PmuModeling);
REGISTER_PROCESS_DEPENDENT_DATA(PmuAssociation, std::vector<HalPmuData>, std::map<TaskId, std::vector<DeviceTask>>,
                                std::vector<HalFreqLpmData>, HostStreamInfo);
REGISTER_PROCESS_SUPPORT_CHIP(PmuAssociation, CHIP_ID_ALL);
}  // namespace Domain
}  // namespace Analysis
