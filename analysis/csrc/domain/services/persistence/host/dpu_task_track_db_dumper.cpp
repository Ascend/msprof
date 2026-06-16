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

#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"

namespace Analysis
{
namespace Domain
{

using HashData = Analysis::Domain::Host::Cann::HashData;

namespace
{
const uint16_t DEVICE_ID_MASK = 0xFFF;
const std::string NA = "N/A";
const std::string AI_CPU = "AI_CPU";
}  // namespace

DpuTaskTrackDBDumper::DpuTaskTrackDBDumper(const std::string &hostFilePath)
    : BaseDumper<DpuTaskTrackDBDumper>(hostFilePath, "DPUTaskTrack")
{
    MAKE_SHARED0_NO_OPERATION(database_, DPUDB);
}

DpuTaskTrackData DpuTaskTrackDBDumper::GenerateData(const DpuTrackInfos &dpuTrackInfos)
{
    DpuTaskTrackData data;
    if (!Utils::Reserve(data, dpuTrackInfos.size()))
    {
        return data;
    }
    for (const auto &info : dpuTrackInfos)
    {
        uint32_t dpuDeviceId = info->data.dpuTrack.deviceId & DEVICE_ID_MASK;
        uint32_t threadId = info->threadId;
        uint64_t startTime = info->data.dpuTrack.startTime;
        uint64_t endTime = info->timeStamp;
        std::string taskType = AI_CPU;  // 先写死 之后统一全局常量
        uint32_t streamId = info->data.dpuTrack.streamId;
        uint32_t taskId = info->data.dpuTrack.taskId;
        uint64_t kernelKey = (static_cast<uint64_t>(info->data.dpuTrack.deviceId) << 32) | taskId;
        auto kernelIt = kernelNameMap_.find(kernelKey);
        std::string kernelName =
            (kernelIt != kernelNameMap_.end()) ? HashData::GetInstance().Get(kernelIt->second) : NA;

        data.emplace_back(dpuDeviceId, threadId, startTime, endTime, taskType, streamId, taskId, kernelName);
    }
    return data;
}

}  // namespace Domain
}  // namespace Analysis
