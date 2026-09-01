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

#include "analysis/csrc/domain/services/association/cann/include/tree_builder.h"

#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Cann
{

namespace
{
const int MAX_DEPTH = 20;
const int MAX_BINDING_DEPTH = 5;
const std::string RECORD_EVENT = "aclrtRecordEvent";
const std::string WAIT_EVENT = "aclrtStreamWaitEvent";

}  // namespace

using namespace Analysis::Utils;

void TreeBuilder::InitTaskTrackApiTypes()
{
    hasRecordEventType_ = false;
    recordEventLevel_ = 0;
    recordEventType_ = 0;
    hasWaitEventType_ = false;
    waitEventLevel_ = 0;
    waitEventType_ = 0;

    for (const auto &levelData : Host::Cann::TypeData::GetInstance().GetAll())
    {
        for (const auto &typeData : levelData.second)
        {
            if (typeData.second == RECORD_EVENT)
            {
                hasRecordEventType_ = true;
                recordEventLevel_ = levelData.first;
                recordEventType_ = typeData.first;
            }
            else if (typeData.second == WAIT_EVENT)
            {
                hasWaitEventType_ = true;
                waitEventLevel_ = levelData.first;
                waitEventType_ = typeData.first;
            }
            if (hasRecordEventType_ && hasWaitEventType_)
            {
                return;
            }
        }
    }
}

void TreeBuilder::SetApiEventKey(const std::shared_ptr<Event> &apiEvent,
                                 const std::shared_ptr<Event> &taskTrackEvent) const
{
    if (!apiEvent || !apiEvent->apiPtr || !taskTrackEvent || !taskTrackEvent->compactPtr ||
        taskTrackEvent->info.type != EventType::EVENT_TYPE_TASK_TRACK)
    {
        return;
    }
    if (hasRecordEventType_ && apiEvent->apiPtr->level == recordEventLevel_ &&
        apiEvent->apiPtr->type == recordEventType_)
    {
        apiEvent->key = taskTrackEvent->compactPtr->data.runtimeTrack.eventInfo.key;
    }
    else if (hasWaitEventType_ && apiEvent->apiPtr->level == waitEventLevel_ &&
             apiEvent->apiPtr->type == waitEventType_)
    {
        apiEvent->key = taskTrackEvent->compactPtr->data.runtimeTrack.notifyInfo.key;
    }
}
/*
 EventLevel      此Level包含的EventType
|- ACL
|- Model    [graph_id_map, fusion_op_info]
|- Node     [node_basic_info, node_attr_info, tensor_info, context_id, hccl_op_info]
|- HCCL     [hccl_info, context_id]
|- Runtime  [task_track, mem_cpy]

建树对外接口：
1. 使用Model、Node、HCCL层的Api Event建立核心树
    建树过程中需要记录 a.各个层级的api

2. 向核心树的各个level的节点上挂附加信息event
3. 向核心树的节点上添加tasktrack event
*/
std::shared_ptr<TreeNode> TreeBuilder::Build()
{
    if (!cannWarehouse_->kernelEvents && !cannWarehouse_->taskTrackEvents)
    {
        WARN("No key profiling info to build tree, threadId = %", threadId_);
        return nullptr;
    }
    kernelEvents_ = cannWarehouse_->kernelEvents;
    auto taskTrackEvents = cannWarehouse_->taskTrackEvents;

    auto rootNode = GenerateRoot();
    std::shared_ptr<TreeNode> tree;
    if (kernelEvents_ and !kernelEvents_->Empty())
    {
        // 1. 用Api和TaskTrack Event建立核心树
        tree = BuildTree(rootNode, 0);
    }
    else
    {
        // 1. root节点就是整颗核心树
        tree = rootNode;
    }
    if (!tree)
    {
        ERROR("Build Tree Failed, ThreadId = %", threadId_);
        return nullptr;
    }
    MultiThreadAddLevelEvents();
    InitTaskTrackApiTypes();
    // 向核心树添加TaskTrack类型的events
    AddTaskTrackEvents(rootNode, taskTrackEvents);
    if (taskTrackEvents && !taskTrackEvents->Empty())
    {
        ERROR("taskTrackEvents matching is not complete, threadId: %", threadId_);
    }
    INFO("Build Tree End, ThreadId = %", threadId_);
    return tree;
}

void TreeBuilder::MultiThreadAddLevelEvents()
{
    auto fusionOpInfoEvents = cannWarehouse_->fusionOpInfoEvents;
    auto nodeBasicInfoEvents = cannWarehouse_->nodeBasicInfoEvents;
    auto nodeAttrInfoEvents = cannWarehouse_->nodeAttrInfoEvents;
    auto tensorInfoEvents = cannWarehouse_->tensorInfoEvents;
    auto contextIdEvents = cannWarehouse_->contextIdEvents;
    auto hcclInfoEvents = cannWarehouse_->hcclInfoEvents;
    auto hcclOpInfoEvents = cannWarehouse_->hcclOpInfoEvents;
    auto ctxIdPair = GroupCtxIdEvents(contextIdEvents);
    auto nodeCtxIdEvents = ctxIdPair.first;
    auto hcclCtxIdEvents = ctxIdPair.second;

    // 2. 向核心树的对应Level的TreeNode添加附加Event
    // 多线程添加，每一个Level一个线程
    const uint16_t levels = 3;
    ThreadPool pool(levels);
    pool.Start();

    // Model Level
    pool.AddTask([this, &fusionOpInfoEvents]()
                 { AddLevelEvents(fusionOpInfoEvents, modelLevelNodes_, EventType::EVENT_TYPE_FUSION_OP_INFO); });

    // Node Level
    pool.AddTask(
        [this, &nodeCtxIdEvents, &tensorInfoEvents, &nodeBasicInfoEvents, &nodeAttrInfoEvents, &hcclOpInfoEvents]()
        {
            AddLevelEvents(nodeBasicInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_NODE_BASIC_INFO);
            AddLevelEvents(nodeAttrInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_NODE_ATTR_INFO);
            AddLevelEvents(tensorInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_TENSOR_INFO);
            AddLevelEvents(nodeCtxIdEvents, nodeLevelNodes_, EventType::EVENT_TYPE_CONTEXT_ID);
            AddLevelEvents(hcclOpInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_HCCL_OP_INFO);
        });

    // Hccl level
    pool.AddTask(
        [this, &hcclCtxIdEvents, &hcclInfoEvents]()
        {
            AddLevelEvents(hcclCtxIdEvents, hcclLevelNodes_, EventType::EVENT_TYPE_CONTEXT_ID);
            AddLevelEvents(hcclInfoEvents, hcclLevelNodes_, EventType::EVENT_TYPE_HCCL_INFO);
        });

    pool.WaitAllTasks();
    pool.Stop();
}

std::shared_ptr<TreeNode> TreeBuilder::GenerateRoot()
{
    uint64_t bound;
    // rootNode的时间片应该覆盖整个EventQueue
    if (!kernelEvents_ or kernelEvents_->Empty())
    {
        bound = cannWarehouse_->taskTrackEvents->GetBound();
    }
    else
    {
        auto tkBound = cannWarehouse_->taskTrackEvents ? cannWarehouse_->taskTrackEvents->GetBound() : 0;
        bound = std::max(tkBound, kernelEvents_->GetBound());
    }
    EventInfo rootInfo{EventType::EVENT_TYPE_API, 0, 0, bound + 1};
    std::shared_ptr<ParserApi> api;
    MAKE_SHARED0_RETURN_VALUE(api, ParserApi, nullptr);
    std::shared_ptr<Event> rootEvent;
    MAKE_SHARED_RETURN_VALUE(rootEvent, Event, nullptr, api, rootInfo);
    std::shared_ptr<TreeNode> rootNode;
    MAKE_SHARED_RETURN_VALUE(rootNode, TreeNode, nullptr, rootEvent);
    return rootNode;
}

// 将ctxIdEvents按Level分为Node和HCCL层
EventQueuePair TreeBuilder::GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents)
{
    uint64_t ctxIdSize = 0;
    if (ctxIdEvents)
    {
        ctxIdSize = ctxIdEvents->GetSize();
    }
    std::shared_ptr<EventQueue> nodeCtxIdEvent;
    MAKE_SHARED_RETURN_VALUE(nodeCtxIdEvent, EventQueue, {}, threadId_, ctxIdSize);
    std::shared_ptr<EventQueue> hcclCtxIdEvent;
    MAKE_SHARED_RETURN_VALUE(hcclCtxIdEvent, EventQueue, {}, threadId_, ctxIdSize);

    while (ctxIdEvents && !ctxIdEvents->Empty())
    {
        auto ctxId = ctxIdEvents->Pop();
        if (ctxId->info.level == MSPROF_REPORT_NODE_LEVEL)
        {
            nodeCtxIdEvent->Push(ctxId);
        }
        else if (ctxId->info.level == MSPROF_REPORT_HCCL_NODE_LEVEL)
        {
            hcclCtxIdEvent->Push(ctxId);
        }
    }

    return std::make_pair(nodeCtxIdEvent, hcclCtxIdEvent);
}

size_t TreeBuilder::GetEventOffset(size_t idx, std::shared_ptr<Event> &event,
                                   std::vector<std::shared_ptr<TreeNode>> &levelNodes, EventType eventType)
{
    // hccl aicpu下发场景, 会出现2层node嵌套, 需要计算offset, 关联至准确的node api
    // [=======================Node(hcom_allReduce_)==============================]
    //   [===Node(hcomAicpuInit)==]     [===Node(allreduceAicpuKernel)==]
    if (eventType != EventType::EVENT_TYPE_NODE_BASIC_INFO)
    {
        // hccl aicpu下发场景暂时只有node basic info上报
        return 0;
    }
    size_t offset = 1;
    while ((idx + offset) < levelNodes.size() && levelNodes[idx + offset]->event->info.start <= event->info.start)
    {
        if (event->info.start <= levelNodes[idx + offset]->event->info.end)
        {
            return offset;
        }
        ++offset;
    }
    return 0;
}

// 双指针遍历向Model Node HCCL Level的TreeNode中添加events
bool TreeBuilder::AddLevelEvents(std::shared_ptr<EventQueue> &events,
                                 std::vector<std::shared_ptr<TreeNode>> &levelNodes, EventType eventType) const
{
    // 检查输入各Type的Events指针
    if (!events || events->Empty())
    {
        WARN("No Events, event type: %, threadId = %", static_cast<int>(eventType), threadId_);
        return false;
    }
    // 保证非空
    if (levelNodes.empty())
    {
        ERROR("LevelNodes is empty, level: %, threadId = % ", events->Top()->info.level, threadId_);
        return false;
    }
    uint64_t matchCnt = 0;     // 在时间片范围内的数量
    uint64_t mismatchCnt = 0;  // 在时间片范围外的数量
    auto it = levelNodes.begin();
    size_t idx = 0;
    while (!events->Empty() && it != levelNodes.end())
    {
        auto event = events->Top();
        if (event->info.start > (*it)->event->info.start && event->info.start <= (*it)->event->info.end)
        {
            // event在TreeNode时间片内：(start, end]，将event记录在TreeNode的附加信息中
            // hccl aicpu下发场景, 计算offset, 关联至准确的node api
            auto offset = GetEventOffset(idx, event, levelNodes, eventType);
            (*(it + offset))->records.emplace_back(event);
            events->Pop();
            matchCnt++;
        }
        else if (event->info.start > (*it)->event->info.end)
        {
            // event超过TreeNode的结束时间，需要检查是否在下一个TreeNode的开始之前
            if (std::next(it) != levelNodes.end() && event->info.start < (*std::next(it))->event->info.start)
            {
                events->Pop();  // 丢掉此event
                mismatchCnt++;
                ERROR(
                    "Drop event threadId = %, start = %, event type: %, its start time between "
                    "last TreeNode end and next TreeNode start",
                    threadId_, event->info.start, static_cast<int>(eventType));
            }
            it++;
            idx++;
        }
        else
        {
            // event小于TreeNode的开始时间，此event对应的TreeNode丢失
            events->Pop();  // 丢掉此event
            mismatchCnt++;
            ERROR("Drop event threadId = %, event type: %, event time range: [%, %], node time range: [%, %]",
                  threadId_, static_cast<int>(eventType), event->info.start, event->info.end, (*it)->event->info.start,
                  (*it)->event->info.end);
        }
    }
    INFO("Add LevelEvents done, threadId = %, event type: %, matchCnt = %, mismatchCnt = % ", threadId_,
         static_cast<int>(eventType), matchCnt, mismatchCnt);
    // events没有匹配完
    if (!events->Empty())
    {
        ERROR("After matching events is not empty, size = %, threadId = %", events->GetSize(), threadId_);
        return false;
    }
    return true;
}

std::shared_ptr<TreeNode> TreeBuilder::MakeDummyNode(const std::shared_ptr<Event> &event)
{
    // 建立dummy节点
    EventInfo dummyInfo{EventType::EVENT_TYPE_DUMMY, MSPROF_REPORT_RUNTIME_LEVEL, event->info.start, event->info.end};
    std::shared_ptr<ParserApi> api;
    MAKE_SHARED0_RETURN_VALUE(api, ParserApi, nullptr);
    std::shared_ptr<Event> dummyEvent;
    MAKE_SHARED_RETURN_VALUE(dummyEvent, Event, nullptr, api, dummyInfo);
    std::shared_ptr<TreeNode> dummyNode;
    MAKE_SHARED_RETURN_VALUE(dummyNode, TreeNode, nullptr, dummyEvent);
    dummyNode->records.emplace_back(event);
    return dummyNode;
}

// 深度优先搜索+队列匹配添加TaskTrackEvents，基于rootNode和events有序
bool TreeBuilder::AddTaskTrackEvents(std::shared_ptr<TreeNode> &treeNode, std::shared_ptr<EventQueue> &events,
                                     uint16_t depth)
{
    if (!treeNode || depth > MAX_BINDING_DEPTH)
    {
        ERROR("The tree node is null or recursion depth % exceeds the maximum depth %.", depth, MAX_BINDING_DEPTH);
        return false;
    }
    // 检查输入Runtime Events指针
    if (!events || events->Empty())
    {
        INFO("TaskTrack Events have been processed, threadId = %", threadId_);
        return true;
    }
    // event属于根节点时间范围内情况
    auto oriSize = treeNode->children.size();  // 只遍历原始数据大小
    uint64_t prevEndTime = treeNode->event->info.start;
    for (size_t i = 0; i < oriSize; ++i)
    {  // 使用索引遍历，避免迭代器失效
        auto child = treeNode->children[i];
        // event在当前子节点开始时间和上一个子节点结束时间之间情况(包括第一个节点之前），添加到根节点
        while (!events->Empty() && events->Top()->info.start > prevEndTime &&
               events->Top()->info.start <= child->event->info.start)
        {
            SetApiEventKey(treeNode->event, events->Top());
            auto dummyNode = MakeDummyNode(events->Pop());
            if (!dummyNode)
            {
                return false;
            }
            dummyNode->parent = treeNode;
            treeNode->children.emplace_back(dummyNode);
        }
        // event在当前子节点的时间范围内情况
        while (!events->Empty() && events->Top()->info.start > child->event->info.start &&
               events->Top()->info.start <= child->event->info.end)
        {
            SetApiEventKey(child->event, events->Top());
            // 递归调用AddTaskTrackEvents添加event
            if (!AddTaskTrackEvents(child, events, depth + 1))
            {
                return false;
            }
        }
        prevEndTime = child->event->info.end;
    }
    // event不在根节点的所有子节点时间范围内，但在根节点时间范围内情况
    while (!events->Empty() && events->Top()->info.start > treeNode->event->info.start &&
           events->Top()->info.start <= treeNode->event->info.end)
    {
        SetApiEventKey(treeNode->event, events->Top());
        auto dummyNode = MakeDummyNode(events->Pop());
        if (!dummyNode)
        {
            return false;
        }
        dummyNode->parent = treeNode;
        treeNode->children.emplace_back(dummyNode);
    }
    LogTreeNode(treeNode);
    return true;
}

void TreeBuilder::LogTreeNode(std::shared_ptr<TreeNode> &treeNode)
{
    uint64_t runtimeNodes = 0;
    for (auto child : treeNode->children)
    {
        if (child->event->info.level == MSPROF_REPORT_RUNTIME_LEVEL)
        {
            runtimeNodes++;
        }
    }
    DEBUG("TreeNode(%, %) processing completed, level: %, children count: %, runtime children count: %",
          treeNode->event->info.start, treeNode->event->info.end, treeNode->event->info.level,
          treeNode->children.size(), runtimeNodes);
}

void TreeBuilder::RecordTreeNode(const std::shared_ptr<TreeNode> &treeNode, const uint16_t &eventLevel)
{
    if (eventLevel == MSPROF_REPORT_MODEL_LEVEL)
    {
        modelLevelNodes_.emplace_back(treeNode);
    }
    else if (eventLevel == MSPROF_REPORT_NODE_LEVEL)
    {
        nodeLevelNodes_.emplace_back(treeNode);
    }
    else if (eventLevel == MSPROF_REPORT_HCCL_NODE_LEVEL)
    {
        hcclLevelNodes_.emplace_back(treeNode);
    }
    else if (eventLevel == MSPROF_REPORT_RUNTIME_LEVEL)
    {
        runtimeLevelNodes_.emplace_back(treeNode);
    }
}

std::shared_ptr<TreeNode> TreeBuilder::BuildTree(std::shared_ptr<TreeNode> parent, int depth)
{
    if (depth >= MAX_DEPTH)
    {
        ERROR("The maximum recursion depth is exceeded, thread id: %", threadId_);
        return nullptr;
    }
    if (!parent)
    {
        ERROR("Parent pointer is nullptr, thread id: %", threadId_);
        return nullptr;
    }

    while (true)
    {
        auto checkEvent = kernelEvents_->Top();
        if (!checkEvent || checkEvent->info.end > parent->event->info.end)
        {
            break;
        }
        auto event = kernelEvents_->Pop();
        std::shared_ptr<TreeNode> childNode;
        MAKE_SHARED_RETURN_VALUE(childNode, TreeNode, nullptr, event);
        // 记录除了Api和TaskTrack类型的TreeNode，按照level记录
        RecordTreeNode(childNode, event->info.level);

        auto childTree = BuildTree(childNode, depth + 1);
        if (childTree != nullptr)
        {
            childTree->parent = parent;
            parent->children.emplace_back(childTree);
        }
    }
    return parent;
}

}  // namespace Cann
}  // namespace Domain
}  // namespace Analysis
