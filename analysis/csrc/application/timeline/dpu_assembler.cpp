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

#include "analysis/csrc/application/timeline/dpu_assembler.h"

#include <algorithm>
#include <set>
#include <unordered_map>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

DPUAssembler::DPUAssembler() : JsonAssembler(PROCESS_DPU, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void DPUTrackTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Thread Id"] << threadId_;
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
    ostream["Task Type"] << taskType_;
}

void DPUHcclTraceEvent::ProcessArgs(JsonWriter &ostream)
{
    ostream["Thread Id"] << threadId_;
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
    ostream["OP Type"] << opType_;
    ostream["AI CPU Device Id"] << npuDeviceId_;
    ostream["AI CPU Task Id"] << aicpuTaskId_;
    ostream["Group Name"] << groupName_ + "(" + groupNameId_ + ")";
    ostream["Plane Id"] << planeId_;
    ostream["Notify Id"] << notifyId_;
    ostream["Duration Estimated(us)"] << durationEstimated_;
    ostream["Rank Size"] << rankSize_;
    ostream["Src Rank"] << localRank_;
    ostream["Dst Rank"] << remoteRank_;
    ostream["Transport Type"] << transportType_;
    ostream["Size(Byte)"] << dataSize_;
    ostream["Bandwidth(GB/s)"] << bandwidth_;
    ostream["Data Type"] << dataType_;
    ostream["Link Type"] << linkType_;
    ostream["Rdma Type"] << rdmaType_;
}

void DPUAssembler::GenerateMetaData(const std::vector<DPUData> &dpuData, uint32_t pid)
{
    std::map<uint16_t, std::set<uint16_t>> deviceStreamMap;
    for (const auto &data : dpuData)
    {
        deviceStreamMap[data.dpuDeviceId].insert(data.streamId);
    }

    auto layer = GetLayerInfo(PROCESS_DPU);

    for (const auto &devicePair : deviceStreamMap)
    {
        auto formatPid = JsonAssembler::GetFormatPid(pid, layer.sortIndex, devicePair.first);

        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                layer.component);
        res_.push_back(processName);

        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                layer.label + " " + std::to_string(devicePair.first));
        res_.push_back(processLabel);

        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, formatPid, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                layer.sortIndex);
        res_.push_back(processIndex);

        for (const auto &streamId : devicePair.second)
        {
            std::string streamName = "Stream " + std::to_string(streamId);
            std::shared_ptr<MetaDataNameEvent> threadName;
            MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, formatPid, static_cast<int>(streamId),
                                    META_DATA_THREAD_NAME, streamName);
            res_.push_back(threadName);

            std::shared_ptr<MetaDataIndexEvent> threadIndex;
            MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, formatPid, static_cast<int>(streamId),
                                    META_DATA_THREAD_INDEX, streamId);
            res_.push_back(threadIndex);
        }
    }
}

void DPUAssembler::GenerateDPUTrace(const std::vector<DPUData> &dpuData, uint32_t pid)
{
    auto layer = GetLayerInfo(PROCESS_DPU);
    for (const auto &data : dpuData)
    {
        auto formatPid = JsonAssembler::GetFormatPid(pid, layer.sortIndex, data.dpuDeviceId);
        double dur = static_cast<double>(data.endTime - data.timestamp) / NS_TO_US;
        std::string ts = DivideByPowersOfTenWithPrecision(data.timestamp);
        int tid = static_cast<int>(data.streamId);

        if (!data.isHccl)
        {
            std::shared_ptr<DPUTrackTraceEvent> event;
            MAKE_SHARED_RETURN_VOID(event, DPUTrackTraceEvent, formatPid, tid, dur, ts, data.opName, data.threadId,
                                    data.streamId, data.taskId, data.taskType);
            res_.push_back(event);
        }
        else
        {
            double bandwidth = 0;
            if (!Utils::IsDoubleEqual(dur, 0.0) && data.dataSize <= INT64_MAX)
            {
                constexpr double COMMUNICATION_B_TO_GB = 1.0 / (1000 * 1000 * 1000);
                bandwidth = data.dataSize * COMMUNICATION_B_TO_GB / (dur / MICRO_SECOND);
            }

            std::shared_ptr<DPUHcclTraceEvent> event;
            MAKE_SHARED_RETURN_VOID(event, DPUHcclTraceEvent, formatPid, tid, dur, ts, data.opName, data.threadId,
                                    data.streamId, data.taskId, data.opType, data.npuDeviceId, data.aicpuTaskId,
                                    data.groupName, data.groupNameId, data.planeId, data.notifyId, data.rankSize,
                                    data.durationEstimated, data.localRank, data.remoteRank, data.transportType,
                                    data.dataSize, bandwidth, data.dataType, data.linkType, data.rdmaType);
            res_.push_back(event);
        }
    }
}

uint8_t DPUAssembler::AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath)
{
    INFO("Start DPU Assembler");
    auto dpuData = dataInventory.GetPtr<std::vector<DPUData>>();
    if (dpuData == nullptr)
    {
        WARN("Can't get dpuData from dataInventory");
        return DATA_NOT_EXIST;
    }

    auto pid = Context::GetInstance().GetPidFromInfoJson(HOST_ID, profPath);
    GenerateMetaData(*dpuData, pid);
    GenerateDPUTrace(*dpuData, pid);

    if (res_.empty())
    {
        ERROR("Can't Generate any DPU process data");
        return ASSEMBLE_FAILED;
    }

    for (const auto &node : res_)
    {
        node->DumpJson(ostream);
    }
    ostream << ",";
    INFO("End DPU Assembler");
    return ASSEMBLE_SUCCESS;
}
}  // namespace Application
}  // namespace Analysis
