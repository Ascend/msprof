/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include "analysis/csrc/domain/services/modeling/npu_op_mem_calculator.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <utility>

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace
{

struct OperatorKey
{
    OperatorKey(uint64_t operatorIdValue, uint64_t addrValue, uint32_t deviceIdValue)
        : operatorId(operatorIdValue), addr(addrValue), deviceId(deviceIdValue)
    {
    }

    uint64_t operatorId;
    uint64_t addr;
    uint32_t deviceId;

    bool operator==(const OperatorKey &other) const
    {
        return operatorId == other.operatorId && addr == other.addr && deviceId == other.deviceId;
    }
};

struct OperatorKeyHash
{
    size_t operator()(const OperatorKey &key) const
    {
        size_t seed = std::hash<uint64_t>()(key.operatorId);
        seed ^= std::hash<uint64_t>()(key.addr) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
        seed ^= std::hash<uint32_t>()(key.deviceId) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
        return seed;
    }
};

struct AllocatedData
{
    AllocatedData(size_t index, uint64_t order) : rawDataIndex(index), insertionOrder(order) {}

    size_t rawDataIndex;
    uint64_t insertionOrder;
};

std::string GetDeviceType(const ParserMemoryInfo &memoryInfo) { return "NPU:" + std::to_string(memoryInfo.deviceId); }

int64_t CalculateDuration(uint64_t allocationTime, uint64_t releaseTime)
{
    // Records are sorted by timestamp before association, and a release is matched only after its allocation.
    // Therefore, releaseTime is guaranteed to be greater than or equal to allocationTime here.
    const uint64_t duration = releaseTime - allocationTime;
    if (duration > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
    {
        return std::numeric_limits<int64_t>::max();
    }
    return static_cast<int64_t>(duration);
}

NpuOpMemLifecycleData CreateLifecycle(const ParserAdditionalInfo &allocation,
                                      const std::unordered_map<uint64_t, std::string> &hashData)
{
    const auto &memoryInfo = allocation.memoryInfo;
    NpuOpMemLifecycleData lifecycle;
    lifecycle.operatorId = memoryInfo.nodeId;
    lifecycle.size = memoryInfo.size;
    lifecycle.allocationTime = allocation.timeStamp;
    lifecycle.allocationTotalAllocated = memoryInfo.totalAllocateMemory;
    lifecycle.allocationTotalReserved = memoryInfo.totalReserveMemory;
    lifecycle.deviceType = GetDeviceType(memoryInfo);
    auto hashIter = hashData.find(memoryInfo.nodeId);
    if (hashIter != hashData.end())
    {
        lifecycle.name = hashIter->second;
    }
    return lifecycle;
}

void FillReleaseData(NpuOpMemLifecycleData &lifecycle, const ParserAdditionalInfo &release)
{
    lifecycle.releaseTime = release.timeStamp;
    lifecycle.releaseTotalAllocated = release.memoryInfo.totalAllocateMemory;
    lifecycle.releaseTotalReserved = release.memoryInfo.totalReserveMemory;
}

}  // namespace

bool NpuOpMemCalculator::Calculate(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &rawData,
                                   const std::unordered_map<uint64_t, std::string> &hashData,
                                   NpuOpMemCalculationResult &result) const
{
    result = NpuOpMemCalculationResult{};
    if (!Utils::Reserve(result.memoryRecords, rawData.size()) || !Utils::Reserve(result.operatorMemory, rawData.size()))
    {
        ERROR("NpuOpMemCalculator: Reserve result data failed.");
        return false;
    }
    for (const auto &item : rawData)
    {
        if (!item)
        {
            ERROR("NpuOpMemCalculator: Raw data is null.");
            return false;
        }
        NpuOpMemRecordData record;
        record.timestamp = item->timeStamp;
        record.totalReserveMemory = item->memoryInfo.totalReserveMemory;
        record.totalAllocateMemory = item->memoryInfo.totalAllocateMemory;
        record.deviceType = GetDeviceType(item->memoryInfo);
        result.memoryRecords.emplace_back(std::move(record));
    }

    std::vector<size_t> associationData;
    if (!Utils::Reserve(associationData, rawData.size()))
    {
        ERROR("NpuOpMemCalculator: Reserve association data failed.");
        return false;
    }
    for (size_t i = 0; i < rawData.size(); ++i)
    {
        associationData.emplace_back(i);
    }
    std::stable_sort(associationData.begin(), associationData.end(), [&rawData](size_t left, size_t right)
                     { return rawData[left]->timeStamp < rawData[right]->timeStamp; });

    std::unordered_map<OperatorKey, size_t, OperatorKeyHash> allocatedData;
    std::vector<AllocatedData> allocatedStates;
    if (!Utils::Reserve(allocatedStates, rawData.size()))
    {
        ERROR("NpuOpMemCalculator: Reserve allocated states failed.");
        return false;
    }
    uint64_t insertionOrder = 0;
    for (const auto rawDataIndex : associationData)
    {
        const auto &item = *rawData[rawDataIndex];
        const auto &memoryInfo = item.memoryInfo;
        const OperatorKey key{memoryInfo.nodeId, memoryInfo.addr, memoryInfo.deviceId};
        if (memoryInfo.size > 0)
        {
            auto iter = allocatedData.find(key);
            if (iter == allocatedData.end())
            {
                const auto allocatedStateIndex = allocatedStates.size();
                allocatedStates.emplace_back(rawDataIndex, insertionOrder++);
                allocatedData.emplace(key, allocatedStateIndex);
            }
            else
            {
                allocatedStates[iter->second].rawDataIndex = rawDataIndex;
            }
        }
        else if (memoryInfo.size < 0)
        {
            auto iter = allocatedData.find(key);
            if (iter == allocatedData.end())
            {
                continue;
            }
            auto lifecycle = CreateLifecycle(*rawData[allocatedStates[iter->second].rawDataIndex], hashData);
            FillReleaseData(lifecycle, item);
            lifecycle.duration = CalculateDuration(lifecycle.allocationTime, lifecycle.releaseTime);
            result.operatorMemory.emplace_back(std::move(lifecycle));
            allocatedData.erase(iter);
        }
    }

    std::vector<size_t> remainingData;
    if (!Utils::Reserve(remainingData, allocatedData.size()))
    {
        ERROR("NpuOpMemCalculator: Reserve remaining data failed.");
        return false;
    }
    for (const auto &item : allocatedData)
    {
        remainingData.emplace_back(item.second);
    }
    std::sort(remainingData.begin(), remainingData.end(), [&allocatedStates](size_t left, size_t right)
              { return allocatedStates[left].insertionOrder < allocatedStates[right].insertionOrder; });
    for (const auto allocatedStateIndex : remainingData)
    {
        result.operatorMemory.emplace_back(
            CreateLifecycle(*rawData[allocatedStates[allocatedStateIndex].rawDataIndex], hashData));
    }
    return true;
}

}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
