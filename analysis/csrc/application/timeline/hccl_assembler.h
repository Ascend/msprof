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

#ifndef ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H
#define ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Application
{
using namespace Analysis::Application;
using namespace Analysis::Utils;
const int32_t INVALID_PLANE = -1;
enum class HcclType
{
    HCCL = 0,
    MC2,
    INVALID
};

struct HcclGroup
{
    int32_t startIndex = 0;
    std::string groupName;
    HcclType type;
    std::set<int32_t> planes;
    HcclGroup() = default;
    HcclGroup(const std::string &groupName_, HcclType &&type_, const std::set<int32_t> &planes_)
        : groupName(groupName_), type(type_), planes(planes_)
    {
    }
};

class HcclOpTraceEvent : public DurationEvent
{
   public:
    HcclOpTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                     uint32_t modelId, uint32_t count, uint64_t connectionId, const std::string &dataType,
                     const std::string &algType, const std::string &relay, const std::string &retry, uint32_t rankSize)
        : DurationEvent(pid, tid, dur, ts, name),
          modelId_(modelId),
          count_(count),
          connectionId_(connectionId),
          dataType_(dataType),
          algType_(algType),
          relay_(relay),
          retry_(retry),
          rankSize_(rankSize)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t rankSize_;
    uint32_t modelId_;
    uint32_t count_;
    uint64_t connectionId_;
    std::string dataType_;
    std::string algType_;
    std::string relay_;
    std::string retry_;
};

class HcclTaskTraceEvent : public DurationEvent
{
   public:
    HcclTaskTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t src,
                       uint32_t dst, uint32_t streamId, uint32_t taskId, uint32_t contextId, uint32_t modelId,
                       uint64_t size, double esDur, double bw, const std::string notifyId, const std::string &tsType,
                       const std::string &taskType, const std::string &dataType, const std::string &linkType)
        : DurationEvent(pid, tid, dur, ts, name),
          srcRank_(src),
          dstRank_(dst),
          streamId_(streamId),
          taskId_(taskId),
          contextId_(contextId),
          modelId_(modelId),
          size_(size),
          esDur_(esDur),
          bandwidth_(bw),
          notifyId_(notifyId),
          transportType_(tsType),
          taskType(taskType),
          dataType_(dataType),
          linkType_(linkType)
    {
    }

   private:
    void ProcessArgs(JsonWriter &ostream) override;

   private:
    uint32_t srcRank_;
    uint32_t dstRank_;
    uint32_t streamId_;
    uint32_t taskId_;
    uint32_t contextId_;
    uint32_t modelId_;
    uint64_t size_;
    double esDur_;
    double bandwidth_;
    std::string notifyId_;
    std::string transportType_;
    std::string taskType;
    std::string dataType_;
    std::string linkType_;
};
class HcclAssembler : public JsonAssembler
{
   public:
    HcclAssembler();

   private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    std::string TransEnumToType(uint64_t key, const std::unordered_map<std::string, uint16_t> &enumTable);
    void GenerateMetaDataEvent(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layerInfo,
                               const std::string &profPath);
    void GenerateTMetaDataEvent(std::vector<HcclGroup> &group, int32_t &index, uint32_t formatPid);
    int32_t GetTid(const std::string groupName, const uint16_t deviceId, const HcclType &type);
    std::unordered_map<uint16_t, std::unordered_map<std::string, std::vector<HcclGroup>>> InitHcclGroup(
        std::shared_ptr<std::vector<CommunicationTaskData>> &hcclData,
        std::shared_ptr<std::vector<KfcTaskData>> &kfcData);

    template <typename T>
    void GenerateCommTaskTrace(const std::vector<T> &task, const std::string &profPath, HcclType &&type,
                               std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layerInfo)
    {
        INFO("Start GenerateCommTaskTrace");
        if (Analysis::Domain::Environment::Context::GetInstance().IsLevel0(profPath))
        {
            return;
        }
        uint32_t formatPid;
        int tid;
        std::string transport;
        std::string dataType;
        std::string linkType;
        for (auto &data : task)
        {
            // 上面已经判定过level0 此处正常不应该进入分支，但是由于lccl适配问题，导致依然存在类似场景
            if (data.planeId == INVALID_PLANE)
            {
                continue;
            }
            formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layerInfo.sortIndex);
            tid = GetTid(data.groupName, data.deviceId, type);
            if (tid == -1)
            {
                continue;
            }
            tid += (data.planeId + 1);
            transport = TransEnumToType(data.transportType, HCCL_TRANSPORT_TYPE_TABLE);
            dataType = TransEnumToType(data.dataType, HCCL_DATA_TYPE_TABLE);
            linkType = TransEnumToType(data.linkType, HCCL_LINK_TYPE_TABLE);
            std::shared_ptr<HcclTaskTraceEvent> event;
            MAKE_SHARED_RETURN_VOID(event, HcclTaskTraceEvent, formatPid, tid,
                                    static_cast<double>(data.duration) / Analysis::Common::NS_TO_US,
                                    DivideByPowersOfTenWithPrecision(data.timestamp), data.taskType, data.srcRank,
                                    data.dstRank, data.streamId, data.taskId, data.contextId, data.modelId, data.size,
                                    data.durationEstimated, data.bandwidth, data.notifyId, transport, data.taskType,
                                    dataType, linkType);
            res_.push_back(event);
        }
    }

    template <typename T>
    void GenerateConnectionTrace(const T &data, uint32_t formatPid, int tid)
    {
        auto connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
        auto traceName = HOST_TO_DEVICE + connId;
        std::shared_ptr<FlowEvent> flow;
        MAKE_SHARED_RETURN_VOID(flow, FlowEvent, formatPid, tid, DivideByPowersOfTenWithPrecision(data.timestamp),
                                HOST_TO_DEVICE, connId, traceName, FLOW_END, FLOW_BP);
        res_.push_back(flow);
    }

    template <typename T>
    void GenerateCommOpTrace(const std::vector<T> &opData, const std::string &profPath, HcclType &&type,
                             std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layerInfo)
    {
        INFO("Start GenerateCommOpTrace");
        uint32_t formatPid;
        int tid;
        std::string dataType;
        std::string retry;
        std::string relay;
        for (auto &data : opData)
        {
            formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layerInfo.sortIndex);
            tid = GetTid(data.groupName, data.deviceId, type);
            if (tid == -1)
            {
                continue;
            }
            dataType = TransEnumToType(data.dataType, HCCL_DATA_TYPE_TABLE);
            std::shared_ptr<HcclOpTraceEvent> event;
            retry = (data.retry == 1) ? "yes" : "no";
            relay = (data.relay == 1) ? "yes" : "no";
            MAKE_SHARED_RETURN_VOID(event, HcclOpTraceEvent, formatPid, tid,
                                    static_cast<double>(data.end - data.timestamp) / Analysis::Common::NS_TO_US,
                                    DivideByPowersOfTenWithPrecision(data.timestamp), data.opName, data.modelId,
                                    data.count, data.connectionId, dataType, data.algType, relay, retry, data.rankSize);
            res_.push_back(event);
            GenerateConnectionTrace(data, formatPid, tid);
        }
    }

   private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::unordered_map<uint16_t, std::unordered_map<std::string, std::vector<HcclGroup>>> groupIndex_;
};
}  // namespace Application
}  // namespace Analysis

#endif  // ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H
