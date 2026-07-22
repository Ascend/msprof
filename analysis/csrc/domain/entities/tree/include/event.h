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
#include <string>
#include <vector>

#include "analysis/csrc/infrastructure/utils/prof_common.h"

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

enum class EventType
{
    EVENT_TYPE_API = 0,
    EVENT_TYPE_EVENT,  // 两个EVENT_TYPE_EVENT可以拼出一个EVENT_TYPE_API
    EVENT_TYPE_NODE_BASIC_INFO,
    EVENT_TYPE_NODE_ATTR_INFO,
    EVENT_TYPE_TENSOR_INFO,
    EVENT_TYPE_HCCL_INFO,
    EVENT_TYPE_CONTEXT_ID,
    EVENT_TYPE_GRAPH_ID_MAP,
    EVENT_TYPE_FUSION_OP_INFO,
    EVENT_TYPE_TASK_TRACK,
    EVENT_TYPE_HCCL_OP_INFO,
    EVENT_TYPE_MEM_CPY,
    EVENT_TYPE_DUMMY,  // 虚拟类型，用于建树时标志虚拟节点
    EVENT_TYPE_INVALID
};

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
    Event(std::shared_ptr<MsprofApi> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<MsprofAdditionalInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<MsprofCompactInfo> eventPtr, const EventInfo &eventInfo);
    Event(std::shared_ptr<ConcatTensorInfo> eventPtr, const EventInfo &eventInfo);
    union
    {
        std::shared_ptr<MsprofApi> apiPtr;
        std::shared_ptr<MsprofAdditionalInfo> additionPtr;
        std::shared_ptr<MsprofCompactInfo> compactPtr;
        std::shared_ptr<ConcatTensorInfo> tensorPtr;
    };
    EventInfo info;
    int64_t id = 0;  // 全局唯一ID

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
