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

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H

#include <map>
#include <memory>
#include <vector>

#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/event_queue.h"
#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "analysis/csrc/domain/services/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{
namespace Domain
{
namespace Cann
{

using EventQueue = Analysis::Domain::EventQueue;
using TreeNode = Analysis::Domain::TreeNode;
using Event = Analysis::Domain::Event;
using EventType = Analysis::Domain::EventType;
using EventInfo = Analysis::Domain::EventInfo;
using CANNWarehouse = Analysis::Domain::Host::Cann::CANNWarehouse;
using ThreadPool = Analysis::Utils::ThreadPool;

using EventQueuePair = std::pair<std::shared_ptr<EventQueue>, std::shared_ptr<EventQueue>>;

// 一个TreeBuilder对象负责将一个threadId的Event数据建成一棵树
class TreeBuilder
{
   public:
    TreeBuilder(std::shared_ptr<CANNWarehouse> &cannWarehouse, const uint32_t threadId)
        : cannWarehouse_(cannWarehouse), threadId_(threadId)
    {
    }
    // 用api和task_track建核心树
    std::shared_ptr<TreeNode> Build();
    // 多线程添加附加Event
    void MultiThreadAddLevelEvents();
    // 向核心树的Model, Node, Hccl 层节点添加附加Event
    bool AddLevelEvents(std::shared_ptr<EventQueue> &events, std::vector<std::shared_ptr<TreeNode>> &levelNodes,
                        EventType eventType) const;
    // 向叶子节点添加Runtime层的event
    bool AddTaskTrackEvents(std::shared_ptr<TreeNode> &treeNode, std::shared_ptr<EventQueue> &events,
                            uint16_t depth = 1);
    // 返回容纳taskTrackEvent的节点
    std::shared_ptr<TreeNode> MakeDummyNode(const std::shared_ptr<Event> &event);
    // 记录父节点绑定runtime event结果日志
    void LogTreeNode(std::shared_ptr<TreeNode> &treeNode);

   private:
    // 建树核心逻辑
    std::shared_ptr<TreeNode> BuildTree(std::shared_ptr<TreeNode>, int depth);
    // 将ctxIdEvents按Level分为Node和HCCL层
    EventQueuePair GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents);
    // 建树时记录各个Level的TreeNode
    void RecordTreeNode(const std::shared_ptr<TreeNode> &treeNode, const uint16_t &eventLevel);
    std::shared_ptr<TreeNode> GenerateRoot();
    void InitTaskTrackApiTypes();
    void SetApiEventKey(const std::shared_ptr<Event> &apiEvent, const std::shared_ptr<Event> &taskTrackEvent) const;
    static size_t GetEventOffset(size_t idx, std::shared_ptr<Event> &event,
                                 std::vector<std::shared_ptr<TreeNode>> &levelNodes, EventType eventType);

   private:
    // 用于建树的Model、Node、HCCL Level的Api Type Events
    std::shared_ptr<EventQueue> kernelEvents_ = nullptr;
    // 包含各个类型的events
    std::shared_ptr<CANNWarehouse> cannWarehouse_ = nullptr;
    // 此对象处理的threadId
    uint32_t threadId_ = 0;

    // 建树时Model, Node, Hccl, Runtime 层节点记录
    std::vector<std::shared_ptr<TreeNode>> modelLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> nodeLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> hcclLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> runtimeLevelNodes_;

    bool hasRecordEventType_ = false;
    uint16_t recordEventLevel_ = 0;
    uint64_t recordEventType_ = 0;
    bool hasWaitEventType_ = false;
    uint16_t waitEventLevel_ = 0;
    uint64_t waitEventType_ = 0;
};
}  // namespace Cann
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
