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
#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_LOG_MODELING_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_LOG_MODELING_H

#include <map>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis
{

namespace Domain
{

struct MergeTaskKeyData
{
    uint32_t contextId;
    uint32_t taskId;
    uint32_t streamId;
    bool operator<(const MergeTaskKeyData& other) const
    {
        if (contextId != other.contextId) return contextId < other.contextId;
        if (taskId != other.taskId) return taskId < other.taskId;
        return streamId < other.streamId;
    }
};

class LogModeling : public Infra::Process
{
   private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

    void SplitLogGroups(std::vector<HalLogData>& logData, std::shared_ptr<std::vector<HalTrackData>>& flipTrack);
    void OutputLogCounts(const std::vector<HalLogData>& logData) const;
    void AddToDeviceTask(std::unordered_map<uint32_t, std::vector<HalLogData*>>& startTask,
                         std::unordered_map<uint32_t, std::vector<HalLogData*>>& endTask,
                         std::unordered_map<uint32_t, std::vector<HalTrackData*>>& flipGroups,
                         std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
                         std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc);

    virtual uint32_t GenGroupKey(const HalLogData& logData);
    virtual MergeTaskKeyData GenMergeTaskKey(const HalLogData& logData);

   private:
    std::unordered_map<uint32_t, std::vector<HalLogData*>> acsqStart_, acsqEnd_, fftsStart_, fftsEnd_;
    std::unordered_map<uint32_t, std::vector<HalTrackData*>> flipData_;
};

class LogModelingV6 final : public LogModeling
{
   private:
    uint32_t GenGroupKey(const HalLogData& logData) override { return logData.hd.taskId.taskId; }
};

}  // namespace Domain

}  // namespace Analysis

#endif
