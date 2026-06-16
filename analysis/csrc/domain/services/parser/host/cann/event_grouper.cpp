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

#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/compact_info_parser.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"

using namespace Analysis::Utils;
using TypeData = Analysis::Domain::Host::Cann::TypeData;

namespace Analysis
{
namespace Domain
{
namespace Host
{
namespace Cann
{
namespace
{
const std::string RECORD_EVENT = "aclrtRecordEvent";
const std::string WAIT_EVENT = "aclrtStreamWaitEvent";
}  // namespace

CANNWarehouses &EventGrouper::GetGroupEvents() { return cannWarehouses_; }

std::set<uint32_t> &EventGrouper::GetThreadIdSet() { return threadIds_; }

std::vector<std::shared_ptr<Event>> &EventGrouper::GetApiTraces() { return apiTraces_; }

std::vector<std::shared_ptr<Adapter::FlipTask>> &EventGrouper::GetFlipTasks() { return flipTasks_; }

std::vector<std::shared_ptr<MsprofCompactInfo>> &EventGrouper::GetDpuTrackData() { return dpuTrackData_; }
std::unordered_map<uint64_t, uint64_t> &EventGrouper::GetDpuKernelNameMap() { return dpuKernelNameMap_; }

bool EventGrouper::Group()
{
    Utils::TimeLogger t{"Group all events"};
    const uint32_t poolSize = 8;
    ThreadPool pool(poolSize);
    pool.Start();

    // 每个Parser一个线程
    pool.AddTask(
        [this]()
        { GroupEvents<ApiEventParser, MsprofApi, &CANNWarehouse::kernelEvents>("Kernel", EventType::EVENT_TYPE_API); });
    pool.AddTask(
        [this]()
        {
            GroupEvents<GraphIdParser, MsprofAdditionalInfo, &CANNWarehouse::graphIdMapEvents>(
                "GraphIdMap", EventType::EVENT_TYPE_GRAPH_ID_MAP);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<FusionOpInfoParser, MsprofAdditionalInfo, &CANNWarehouse::fusionOpInfoEvents>(
                "FusionOpInfo", EventType::EVENT_TYPE_FUSION_OP_INFO);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<NodeBasicInfoParser, MsprofCompactInfo, &CANNWarehouse::nodeBasicInfoEvents>(
                "NodeBasicInfo", EventType::EVENT_TYPE_NODE_BASIC_INFO);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<NodeAttrInfoParser, MsprofCompactInfo, &CANNWarehouse::nodeAttrInfoEvents>(
                "NodeAttrInfo", EventType::EVENT_TYPE_NODE_ATTR_INFO);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<TensorInfoParser, ConcatTensorInfo, &CANNWarehouse::tensorInfoEvents>(
                "TensorInfo", EventType::EVENT_TYPE_TENSOR_INFO);
        });
    if (!Environment::Context::GetInstance().IsChipV6(Environment::Context::GetInstance().GetPlatformVersion()))
    {
        pool.AddTask(
            [this]()
            {
                GroupEvents<CtxIdParser, MsprofAdditionalInfo, &CANNWarehouse::contextIdEvents>(
                    "ContextId", EventType::EVENT_TYPE_CONTEXT_ID);
            });
    }
    pool.AddTask(
        [this]()
        {
            GroupEvents<HcclInfoParser, MsprofAdditionalInfo, &CANNWarehouse::hcclInfoEvents>(
                "HcclInfo", EventType::EVENT_TYPE_HCCL_INFO);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<TaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
                "TaskTrack", EventType::EVENT_TYPE_TASK_TRACK);
        });
    pool.AddTask(
        [this]()
        {
            GroupEvents<HcclOpInfoParser, MsprofCompactInfo, &CANNWarehouse::hcclOpInfoEvents>(
                "HcclOpInfo", EventType::EVENT_TYPE_HCCL_OP_INFO);
        });
    pool.AddTask(
        [this]()
        {
            // 复用模板函数 临时使用taskTrackEvents占位 实际业务不使用
            GroupEvents<DpuTaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
                "DpuTaskTrack", EventType::EVENT_TYPE_TASK_TRACK);
        });

    pool.WaitAllTasks();
    pool.Stop();
    RecordCANNWareHouses();

    return true;
}

void EventGrouper::RecordCANNWareHouses()
{
    for (auto thread : threadIds_)
    {
        if (!cannWarehouses_.Find(thread))
        {
            continue;
        }
        auto wareHouse = cannWarehouses_[thread];
        INFO(
            "After group events, Kernel: %, GraphId: %, FusionOp: %, NodeBasic: %, NodeAttr: %, Tensor: %,"
            " ContextId: %, HcclInfo: %, TaskTrack: %, HcclOpInfo: %, thread: %",
            wareHouse.kernelEvents ? wareHouse.kernelEvents->GetSize() : 0,
            wareHouse.graphIdMapEvents ? wareHouse.graphIdMapEvents->GetSize() : 0,
            wareHouse.fusionOpInfoEvents ? wareHouse.fusionOpInfoEvents->GetSize() : 0,
            wareHouse.nodeBasicInfoEvents ? wareHouse.nodeBasicInfoEvents->GetSize() : 0,
            wareHouse.nodeAttrInfoEvents ? wareHouse.nodeAttrInfoEvents->GetSize() : 0,
            wareHouse.tensorInfoEvents ? wareHouse.tensorInfoEvents->GetSize() : 0,
            wareHouse.contextIdEvents ? wareHouse.contextIdEvents->GetSize() : 0,
            wareHouse.hcclInfoEvents ? wareHouse.hcclInfoEvents->GetSize() : 0,
            wareHouse.taskTrackEvents ? wareHouse.taskTrackEvents->GetSize() : 0,
            wareHouse.hcclOpInfoEvents ? wareHouse.hcclOpInfoEvents->GetSize() : 0, thread);
    }
}

void EventGrouper::InitLastKernelTimes(const std::set<uint32_t> &threadIds)
{
    for (auto threadId : threadIds)
    {
        lastKernelTimes_[threadId] = {{MSPROF_REPORT_ACL_LEVEL, {0, 0}},
                                      {MSPROF_REPORT_MODEL_LEVEL, {0, 0}},
                                      {MSPROF_REPORT_NODE_LEVEL, {0, 0}},
                                      {MSPROF_REPORT_HCCL_NODE_LEVEL, {0, 0}}};
    }
}

bool EventGrouper::IsBuildTreeWithAcl(const std::shared_ptr<MsprofApi> &trace)
{
    if (trace->level == MSPROF_REPORT_ACL_LEVEL)
    {
        auto id = TypeData::GetInstance().Get(trace->level, trace->type);
        return id != RECORD_EVENT && id != WAIT_EVENT;
    }
    return false;
}

bool EventGrouper::isKernelApiEvent(const std::shared_ptr<MsprofApi> &trace)
{
    if (trace->level == MSPROF_REPORT_ACL_LEVEL || trace->level == MSPROF_REPORT_MODEL_LEVEL ||
        trace->level == MSPROF_REPORT_NODE_LEVEL || trace->level == MSPROF_REPORT_HCCL_NODE_LEVEL)
    {
        if (trace->beginTime == trace->endTime)
        {
            ERROR("Invalid api event, threadId = %, level = %, begin = %, end = %", trace->threadId, trace->level,
                  trace->beginTime, trace->endTime);
            return false;
        }
        if (trace->level == MSPROF_REPORT_NODE_LEVEL)
        {
            return true;
        }
        // 判断区间相交则为冗余数据
        auto lastPair = lastKernelTimes_[trace->threadId][trace->level];
        if (lastPair.second < trace->beginTime or trace->endTime < lastPair.first)
        {
            lastKernelTimes_[trace->threadId][trace->level] = {trace->beginTime, trace->endTime};
            return true;
        }
        if (IsBuildTreeWithAcl(trace))
        {
            return false;
        }
        WARN("Redundant api, level: %, time: [%, %], last valid api time: [%, %], threadId = %", trace->level,
             trace->beginTime, trace->endTime, lastKernelTimes_[trace->threadId][trace->level].first,
             lastKernelTimes_[trace->threadId][trace->level].second, trace->threadId);
        return false;
    }
    return false;
}

// 对于KernelEvents特化
template <>
void EventGrouper::GroupEvents<ApiEventParser, MsprofApi, &CANNWarehouse::kernelEvents>(const std::string &typeName,
                                                                                        EventType eventType)
{
    // 1. 解析bin
    Utils::TimeLogger t{"Group " + typeName};
    std::shared_ptr<ApiEventParser> parser;
    MAKE_SHARED_RETURN_VOID(parser, ApiEventParser, hostPath_);

    auto traces = parser->ParseData<MsprofApi>();
    SortByTimeAndLevel(traces);
    // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
    std::unordered_map<uint32_t, uint64_t> threadIdNum;
    std::set<uint32_t> threadIds;
    for (const auto &trace : traces)
    {
        threadIds.insert(trace->threadId);
    }
    InitLastKernelTimes(threadIds);
    for (const auto &trace : traces)
    {
        if (isKernelApiEvent(trace))
        {
            threadIdNum[trace->threadId] += 1;
        }
    }
    InitLastKernelTimes(threadIds);
    // 使用临时CANNWarehouses用于暂存kernel类型的数据，减小多线程竞争，提升性能
    CANNWarehouses kernelWarehouses;
    // 2. 转换生成Event，并将其添加到 CANNWarehouses
    for (const auto &trace : traces)
    {
        EventInfo info{eventType, trace->level, trace->beginTime, trace->endTime};
        std::shared_ptr<Event> event;
        MAKE_SHARED_BREAK(event, Event, trace, info);
        apiTraces_.emplace_back(event);  // 保存一份全量api

        // 只处理符合条件的trace，提升性能
        if (!isKernelApiEvent(trace))
        {
            continue;
        }

        // 新建
        if (!kernelWarehouses.FindAndInsertIfNotExist(trace->threadId))
        {
            std::shared_ptr<EventQueue> que;
            MAKE_SHARED_BREAK(que, EventQueue, trace->threadId, threadIdNum[trace->threadId]);
            kernelWarehouses[trace->threadId].kernelEvents = que;
        }
        // 插入
        kernelWarehouses[trace->threadId].kernelEvents->Push(event);
    }

    // 将暂存的kernelWarehouses更新到cannWarehouses_
    for (auto kv : threadIdNum)
    {
        cannWarehouses_[kv.first].kernelEvents = kernelWarehouses[kv.first].kernelEvents;
    }

    std::lock_guard<std::mutex> lock(tidLock_);
    threadIds_.insert(threadIds.begin(), threadIds.end());
}

// 对于taskTrackEvents特化
template <>
void EventGrouper::GroupEvents<TaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
    const std::string &typeName, EventType eventType)
{
    // 1. 解析bin
    Utils::TimeLogger t{"Group " + typeName};
    std::shared_ptr<TaskTrackParser> parser;
    MAKE_SHARED_RETURN_VOID(parser, TaskTrackParser, hostPath_);

    auto traces = parser->ParseData<MsprofCompactInfo>();
    SortByTimeAndLevel(traces);
    flipTasks_ = parser->GetData<Adapter::FlipTask>();
    dpuKernelNameMap_ = parser->GetDpuKernelNameMap();

    // 统计各个threadId元素个数，用于EventQueue分配精确的内存大小，避免大量内存浪费
    std::unordered_map<uint32_t, uint64_t> threadIdNum;
    std::set<uint32_t> threadIds;
    for (const auto &trace : traces)
    {
        threadIds.insert(trace->threadId);
        threadIdNum[trace->threadId] += 1;
    }

    for (const auto &trace : traces)
    {
        EventInfo info{eventType, trace->level, trace->timeStamp, trace->timeStamp};
        std::shared_ptr<Event> event;
        MAKE_SHARED_BREAK(event, Event, trace, info);
        // 新建
        if (!cannWarehouses_.FindAndInsertIfNotExist(trace->threadId) ||
            !(cannWarehouses_[trace->threadId].taskTrackEvents))
        {
            std::shared_ptr<EventQueue> que;
            MAKE_SHARED_BREAK(que, EventQueue, trace->threadId, threadIdNum[trace->threadId]);
            cannWarehouses_[trace->threadId].taskTrackEvents = que;
        }

        // 插入
        cannWarehouses_[trace->threadId].taskTrackEvents->Push(event);
    }
    std::lock_guard<std::mutex> lock(tidLock_);
    threadIds_.insert(threadIds.begin(), threadIds.end());
}

// dpuTaskTrack特化
template <>
void EventGrouper::GroupEvents<DpuTaskTrackParser, MsprofCompactInfo, &CANNWarehouse::taskTrackEvents>(
    const std::string &typeName, EventType eventType)
{
    Utils::TimeLogger t{"Group " + typeName};
    std::shared_ptr<DpuTaskTrackParser> parser;
    MAKE_SHARED_RETURN_VOID(parser, DpuTaskTrackParser, hostPath_);
    dpuTrackData_ = parser->ParseData<MsprofCompactInfo>();
    INFO("Parsed DPU task track data, size: %", dpuTrackData_.size());
}

template <>
void EventGrouper::SortByTimeAndLevel<MsprofApi>(std::vector<std::shared_ptr<MsprofApi>> &traces)
{
    auto comp = [](std::shared_ptr<MsprofApi> &api1, std::shared_ptr<MsprofApi> &api2)
    { return api1->beginTime < api2->beginTime || (api1->beginTime == api2->beginTime && api1->level > api2->level); };
    std::sort(traces.begin(), traces.end(), comp);
}

}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis
