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

#ifndef ANALYSIS_ENTITIES_EVENT_H
#define ANALYSIS_ENTITIES_EVENT_H

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"

namespace Analysis
{
namespace Domain
{

/*
 Profiling在CANN软件栈分为ACL、Model、Node、Hccl、Runtime 5层
------------------------- Pytorch -------------------------------
|-
-------------------------- PTA ----------------------------------
|-
------------------------- CANN ---------------------------------
|- ACL Level
|- Model Level [graph_id_map, fusion_op_info]
|- Node Level [node_basic_info, node_attr_info, node_tensor_info, context_id, hccl_op_info]
|- HCCL Level [hccl_info, context_id]
|- Runtime Level [task_track, mem_cpy]
-------------------------- NPU ---------------------------------

 Event类型：api, event, node_basic_info, node_attr_info, tensor_info, hccl_info,
context_id, graph_id_map, fusion_op_info, task_track, mem_cpy

 */

// 单一数据源：枚举成员与展示名必须一一对应，新增类型只需在 EVENT_TYPE_LIST 加一行
// (枚举名, 展示名)，EventType 与 EventTypeNames() 由同一清单生成，二者永不漂移。
// 展示名非机械派生（如 MEM_CPY->MemoryCopy），勿改为代码内推导。
#define EVENT_TYPE_LIST(ENTRY)                                                                                         \
    ENTRY(EVENT_TYPE_API, "Api")                                                                                       \
    ENTRY(EVENT_TYPE_EVENT, "Event") /* 两个EVENT_TYPE_EVENT可以拼出一个EVENT_TYPE_API */                      \
    ENTRY(EVENT_TYPE_NODE_BASIC_INFO, "NodeBasicInfo")                                                                 \
    ENTRY(EVENT_TYPE_NODE_ATTR_INFO, "NodeAttrInfo")                                                                   \
    ENTRY(EVENT_TYPE_TENSOR_INFO, "TensorInfo")                                                                        \
    ENTRY(EVENT_TYPE_HCCL_INFO, "HcclInfo")                                                                            \
    ENTRY(EVENT_TYPE_CONTEXT_ID, "ContextId")                                                                          \
    ENTRY(EVENT_TYPE_GRAPH_ID_MAP, "GraphIdMap") /* graph_id_map，lookup 后落盘 ModelName */                       \
    ENTRY(EVENT_TYPE_FUSION_OP_INFO, "FusionOpInfo")                                                                   \
    ENTRY(EVENT_TYPE_TASK_TRACK, "TaskTrack")                                                                          \
    ENTRY(EVENT_TYPE_HCCL_OP_INFO, "HcclOpInfo")                                                                       \
    ENTRY(EVENT_TYPE_MEM_CPY, "MemoryCopy")            /* memcpy_info，lookup 后落盘 */                            \
    ENTRY(EVENT_TYPE_RUNTIME_OP_INFO, "RuntimeOpInfo") /* capture_op_info */                                           \
    ENTRY(EVENT_TYPE_DPU_TASK_TRACK, "DpuTaskTrack")   /* dpu_track */                                                 \
    ENTRY(EVENT_TYPE_CAPTURE_STREAM_INFO, "CaptureStreamInfo")                                                         \
    ENTRY(EVENT_TYPE_MC2_COMM_INFO, "Mc2CommInfo")                                                                     \
    ENTRY(EVENT_TYPE_STATIC_OP_MEM, "StaticOpMem") /* C++ parser/dumper 已落地，入口未使能，仍走 Python */ \
    ENTRY(EVENT_TYPE_STREAM_EXPAND_SPEC, "StreamExpandSpec") /* expand_stream_spec，解析后直接落盘 */          \
    ENTRY(EVENT_TYPE_DUMMY, "Dummy")                         /* 虚拟类型，用于建树时标志虚拟节点 */    \
    ENTRY(EVENT_TYPE_INVALID, "Invalid")

enum class EventType
{
#define EVENT_TYPE_ENUM_ENTRY(enumName, displayName) enumName,
    EVENT_TYPE_LIST(EVENT_TYPE_ENUM_ENTRY)
#undef EVENT_TYPE_ENUM_ENTRY
};

// EventType 对应的展示名表，顺序与枚举一一对应
inline const std::vector<std::string> &EventTypeNames()
{
    static const std::vector<std::string> names = {
#define EVENT_TYPE_NAME_ENTRY(enumName, displayName) displayName,
        EVENT_TYPE_LIST(EVENT_TYPE_NAME_ENTRY)
#undef EVENT_TYPE_NAME_ENTRY
    };
    return names;
}

// 越界防御：type 非枚举成员时返回 "Invalid"，避免整型转枚举后数组越界
inline const std::string &EventTypeToString(EventType type)
{
    static const std::string invalidType = "Invalid";
    const size_t index = static_cast<size_t>(type);
    const auto &names = EventTypeNames();
    return (index < names.size()) ? names[index] : invalidType;
}

#undef EVENT_TYPE_LIST

inline const std::set<EventType> &TreeBuildEventTypes()
{
    static const std::set<EventType> types = {
        EventType::EVENT_TYPE_API,
        EventType::EVENT_TYPE_EVENT,
        EventType::EVENT_TYPE_NODE_BASIC_INFO,
        EventType::EVENT_TYPE_NODE_ATTR_INFO,
        EventType::EVENT_TYPE_TENSOR_INFO,
        EventType::EVENT_TYPE_HCCL_INFO,
        EventType::EVENT_TYPE_CONTEXT_ID,
        EventType::EVENT_TYPE_FUSION_OP_INFO,
        EventType::EVENT_TYPE_TASK_TRACK,
        EventType::EVENT_TYPE_HCCL_OP_INFO,
    };
    return types;
}

inline const std::set<EventType> &LookupEventTypes()
{
    static const std::set<EventType> types = {
        EventType::EVENT_TYPE_RUNTIME_OP_INFO,     EventType::EVENT_TYPE_DPU_TASK_TRACK,
        EventType::EVENT_TYPE_CAPTURE_STREAM_INFO, EventType::EVENT_TYPE_MC2_COMM_INFO,
        EventType::EVENT_TYPE_GRAPH_ID_MAP,        EventType::EVENT_TYPE_MEM_CPY,
        EventType::EVENT_TYPE_STREAM_EXPAND_SPEC,
        // EventType::EVENT_TYPE_STATIC_OP_MEM,  // C++ 入口未使能，仍走 Python；使能时打开并加入 Python 白名单
    };
    return types;
}

inline bool NeedTreeBuild(EventType type) { return TreeBuildEventTypes().count(type) > 0; }

inline bool NeedLookup(EventType type) { return LookupEventTypes().count(type) > 0; }

// Event关键信息
struct EventInfo
{
    EventInfo(EventType type, uint16_t level, uint64_t start, uint64_t end)
        : type(type), level(level), start(start), end(end)
    {
    }
    EventType type = EventType::EVENT_TYPE_INVALID;  // 类型
    uint16_t level = 0;                              // 层级
    uint64_t start = 0;                              // 开始时间
    uint64_t end = 0;                                // 结束时间 对于additional Event start == end
};

// Event为本工程对软硬件上报的各类信息(Trace)的抽象, 表示一个时间点或时间片发生的事件
struct Event
{
    Event(std::shared_ptr<ParserApi> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<ParserAdditionalInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<ParserCompactInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<ParserConcatTensorInfo> eventPtr, const EventInfo &eventInfo);
    union
    {
        std::shared_ptr<ParserApi> apiPtr;
        std::shared_ptr<ParserAdditionalInfo> additionPtr;
        std::shared_ptr<ParserCompactInfo> compactPtr;
        std::shared_ptr<ParserConcatTensorInfo> tensorPtr;
    };
    EventInfo info;
    int64_t id = 0;  // 全局唯一ID
    uint64_t key = 0;

    ~Event()
    {
        if (apiPtr)
        {
            apiPtr.~shared_ptr();
        }
        else if (additionPtr)
        {
            additionPtr.~shared_ptr();
        }
        else if (compactPtr)
        {
            compactPtr.~shared_ptr();
        }
        else if (tensorPtr)
        {
            tensorPtr.~shared_ptr();
        }
    }
};

}  // namespace Domain
}  // namespace Analysis

#endif  // ANALYSIS_ENTITIES_EVENT_H
