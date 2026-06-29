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
#include "analysis/csrc/domain/services/association/cann/include/tree_analyzer.h"

#include <memory>
#include <set>
#include <string>

#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"

namespace Analysis
{
namespace Domain
{
namespace Cann
{

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

namespace
{
bool IsChipV6()
{
    static bool isV6 =
        Environment::Context::GetInstance().IsChipV6(Environment::Context::GetInstance().GetPlatformVersion());
    return isV6;
}
inline uint16_t GetBatchId(uint32_t taskId) { return IsChipV6() ? 0 : static_cast<uint16_t>(taskId >> 16); }
inline uint32_t GetTaskIdBit() { return IsChipV6() ? 0xFFFFFFFF : 0x0000FFFF; }
const uint32_t VALID_CTXID_NUM = 2;
const int64_t INVALID_VALUE = -1;
const uint16_t AICORE_TASK_TYPE = 0;
const uint16_t AICPU_TASK_TYPE = 1;
const uint64_t INVALID_MODEL_ID = 4294967295;
const uint64_t PLACEHOLDER_OP_NAME = UINT64_MAX;
const std::string KERNEL_TASK_PREFIX = "KERNEL";
const std::string LCCL_PREFIX = "Lccl";
const std::string GE_STEP_INFO_API_TYPE = "step_info";
const uint32_t FFTS_PLUS_TASK_TYPE = 6;
const uint32_t HCCL_TASK_TYPE = 9;
const uint32_t HCCL_AI_CPU_TASK_TYPE = 256;
const std::string KERNEL_FFTS_PLUS_TASK_TYPE = "FFTS_PLUS";
const std::string KERNEL_STARS_COMMON_TASK_TYPE = "STARS_COMMON";
const std::string KERNEL_AI_CORE_TASK_TYPE = "KERNEL_AICORE";
const std::string KERNEL_AI_VECTOR_CORE_TASK_TYPE = "KERNEL_AIVEC";
const std::string KERNEL_MIX_AIC_TASK_TYPE = "KERNEL_MIX_AIC";
const std::string KERNEL_MIX_AIV_TASK_TYPE = "KERNEL_MIX_AIV";
const std::string KERNEL_AI_CPU_TASK_TYPE = "KERNEL_AICPU";

const std::set<std::string> CONTEXT_ID_WHITE_LIST = {
    KERNEL_AI_CORE_TASK_TYPE,   KERNEL_AI_VECTOR_CORE_TASK_TYPE,
    KERNEL_FFTS_PLUS_TASK_TYPE, Analysis::Common::KERNEL_SIMT_TASK_TYPE,
    KERNEL_MIX_AIC_TASK_TYPE,   KERNEL_MIX_AIV_TASK_TYPE};
const std::set<std::string> KERNEL_COMPUTE_WHITE_LIST = {
    KERNEL_AI_CORE_TASK_TYPE, KERNEL_AI_VECTOR_CORE_TASK_TYPE,
    KERNEL_AI_CPU_TASK_TYPE,  Analysis::Common::KERNEL_SIMT_TASK_TYPE,
    KERNEL_MIX_AIC_TASK_TYPE, KERNEL_MIX_AIV_TASK_TYPE};

uint64_t GetModelId(const std::shared_ptr<MsprofApi> &api, uint16_t deviceId, uint32_t streamId, uint16_t batchId,
                    uint64_t timestamp)
{
    return api != nullptr ? api->itemId
                          : RTAddInfoCenter::GetInstance().GetModelId(deviceId, streamId, batchId, timestamp);
}

}  // namespace

void TreeAnalyzer::Analyze() { DeepFirstSearch(root_); }

HostTasks &TreeAnalyzer::GetHCCLTasks() { return hcclTasks_; }

HostTasks &TreeAnalyzer::GetComputeTasks() { return computeTasks_; }

HostTasks &TreeAnalyzer::GetTasks() { return tasks_; }

HCCLBigOpDescs &TreeAnalyzer::GetHcclBigOps() { return hcclBigOpDescs_; }

GeFusionOpInfos &TreeAnalyzer::GetGeFusionOpInfos() { return geFusionOpInfos_; }

void TreeAnalyzer::DeepFirstSearch(const std::shared_ptr<TreeNode> &node)
{
    if (!node || !node->event)
    {
        ERROR("Node pointer or event is nullptr, threadId = %", threadId_);
        return;
    }
    AnalyzeNode(node);
    path_[node->event->info.level] = node;
    // DFS
    for (const auto &child : node->children)
    {
        DeepFirstSearch(child);
        if (node->event->info.level == child->event->info.level)
        {
            path_[node->event->info.level] = node;
        }
    }
    path_.erase(node->event->info.level);
}

void TreeAnalyzer::AnalyzeNode(const std::shared_ptr<TreeNode> &node)
{
    if (node->event->info.level == MSPROF_REPORT_RUNTIME_LEVEL)
    {
        AnalyzeRuntimeNode(node);
    }
    else if (node->event->info.level == MSPROF_REPORT_MODEL_LEVEL)
    {
        AnalyzeModelNode(node);
    }
}

void TreeAnalyzer::AnalyzeRuntimeNode(const std::shared_ptr<TreeNode> &node)
{
    auto isHccl = IsHcclTask();
    auto isOperateCompute = IsComputeTask(node);
    if (isOperateCompute)
    {
        auto computeTasks = GetComputeTaskDescs(node);
        for (auto &task : computeTasks)
        {
            if (task->op && task->op->type == OpType::OPTYPE_INVALID)
            {
                // 该任务不会被认为是compute任务
                continue;
            }
            computeTasks_.emplace_back(task);
        }

        tasks_.insert(tasks_.end(), computeTasks.begin(), computeTasks.end());
    }

    if (isHccl)
    {
        auto hcclTasks = GetHcclTaskDescs(node);

        for (auto &task : hcclTasks)
        {
            if (task->op && task->op->type == OpType::OPTYPE_INVALID)
            {
                // 该任务不会被认为是hccl任务
                continue;
            }
            hcclTasks_.emplace_back(task);
        }

        for (auto &task : hcclTasks)
        {
            if (isOperateCompute)
            {
                // task已插入，无需再次插入, ffts+模式的通信算子不会进入该分支
                continue;
            }
            tasks_.emplace_back(task);
        }
        UpdateHcclBigOpDescs(node);
    }

    if (!isOperateCompute && !isHccl)
    {
        auto otherTask = GetOtherTaskDesc(node);
        if (otherTask)
        {
            tasks_.emplace_back(otherTask);

            auto taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, otherTask->taskType);
            // 对于纯rts_track数据,只有算子类型在白名单中才生成computeTask数据
            if (KERNEL_COMPUTE_WHITE_LIST.find(taskType) != KERNEL_COMPUTE_WHITE_LIST.end())
            {
                computeTasks_.emplace_back(otherTask);
            }
        }
    }
}

void TreeAnalyzer::AnalyzeModelNode(const std::shared_ptr<TreeNode> &node)
{
    auto modelApi = (node != nullptr && node->event != nullptr) ? node->event->apiPtr : nullptr;
    std::string key = Utils::Join("_", node->event->info.start, node->event->info.end);
    if (visitedModel_.find(key) != visitedModel_.end())
    {
        return;
    }
    auto modelId = modelApi != nullptr ? modelApi->itemId : INVALID_MODEL_ID;
    for (const auto &record : node->records)
    {
        if (record->info.type == EventType::EVENT_TYPE_FUSION_OP_INFO)
        {
            auto fusionOp = record->additionPtr;
            if (fusionOp)
            {
                auto fusionStruct = Utils::ReinterpretConvert<ProfFusionOpInfo *>(fusionOp->data);
                std::shared_ptr<ProfFusionOpInfo> fusionOpInfo;
                MAKE_SHARED_RETURN_VOID(fusionOpInfo, ProfFusionOpInfo, *fusionStruct);
                std::shared_ptr<GeFusionOpInfo> geFusionOpInfo;
                MAKE_SHARED_RETURN_VOID(geFusionOpInfo, GeFusionOpInfo, modelId, fusionOpInfo);
                geFusionOpInfos_.emplace_back(geFusionOpInfo);
            }
        }
    }
    visitedModel_.insert(key);
}

bool TreeAnalyzer::IsHcclTask() { return path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) != path_.end(); }

bool TreeAnalyzer::IsComputeTask(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end())
    {
        return false;
    }

    // 临时规避 lccl算子过滤,后续依据lccl 特征独立判定
    auto node_api = path_[MSPROF_REPORT_NODE_LEVEL]->event->apiPtr;
    if (!node_api || TypeData::GetInstance().Get(node_api->level, node_api->type) == GE_STEP_INFO_API_TYPE)
    {
        return false;
    }
    std::string itemId = HashData::GetInstance().Get(node_api->itemId);
    if (itemId.substr(0, LCCL_PREFIX.size()) == LCCL_PREFIX)
    {
        return false;
    }

    for (const auto &record : node->records)
    {
        if (record->info.type != EventType::EVENT_TYPE_TASK_TRACK)
        {
            continue;
        }

        auto trace = record->compactPtr;
        auto taskType = TypeData::GetInstance().Get(node->event->info.level, trace->data.runtimeTrack.taskType);
        if (taskType.substr(0, KERNEL_TASK_PREFIX.size()) == KERNEL_TASK_PREFIX ||
            taskType == KERNEL_STARS_COMMON_TASK_TYPE)
        {
            // 传统core task和dsa task
            return true;
        }
        else if (taskType == KERNEL_FFTS_PLUS_TASK_TYPE)
        {
            // ffts+ task
            return path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) == path_.end();
        }
        return false;
    }
    return false;
}

void TreeAnalyzer::UpdateHcclBigOpDescs(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end())
    {
        return;
    }
    auto nodeNode = path_[MSPROF_REPORT_NODE_LEVEL];
    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty())
    {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return;
    }
    auto track = tracks.back()->compactPtr;
    auto deviceId = track->data.runtimeTrack.deviceId;

    if (!hcclBigOpDescs_.empty() && hcclBigOpDescs_.back()->hcclBigOpDesc->endTime == nodeNode->event->info.end)
    {
        WARN("Find same end time when update Hccl big op descs, threadId = %", threadId_);
        return;
    }

    auto modelTrace = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ? path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto modelApi = modelTrace != nullptr ? modelTrace->event->apiPtr : nullptr;
    auto indexId = modelApi != nullptr ? modelApi->reserve : -1;
    auto modelId = GetModelId(modelApi, deviceId, track->data.runtimeTrack.streamId,
                              GetBatchId(track->data.runtimeTrack.taskId), track->timeStamp);
    auto connectionId = nodeNode->event->id;
    auto nodeRecords = GetNodeRecordsByType(nodeNode, EventType::EVENT_TYPE_NODE_BASIC_INFO);
    std::shared_ptr<MsprofCompactInfo> nodeDesc = nullptr;
    if (!nodeRecords.empty() && nodeRecords.front() != nullptr)
    {
        nodeDesc = nodeRecords.front()->compactPtr;
    }
    auto hcclOpRecords = GetNodeRecordsByType(nodeNode, EventType::EVENT_TYPE_HCCL_OP_INFO);
    std::shared_ptr<MsprofCompactInfo> hcclOpDesc = nullptr;
    if (!hcclOpRecords.empty() && hcclOpRecords.front() != nullptr)
    {
        hcclOpDesc = hcclOpRecords.front()->compactPtr;
    }
    else
    {
        ERROR("Not report hccl op info for api: %", modelId);
    }
    int64_t kfcConnectionId = INVALID_VALUE;
    for (const auto &child : nodeNode->children)
    {
        if (child->event->info.level == MSPROF_REPORT_NODE_LEVEL)
        {
            kfcConnectionId = child->event->id;
        }
    }

    auto nodeApi = nodeNode->event->apiPtr;
    std::shared_ptr<HcclBigOpDesc> desc;
    MAKE_SHARED_RETURN_VOID(desc, HcclBigOpDesc, nodeApi->beginTime, nodeApi->endTime, deviceId, modelId, indexId,
                            connectionId, track->threadId, nodeDesc, hcclOpDesc, kfcConnectionId);
    std::shared_ptr<Operator> op;
    MAKE_SHARED_RETURN_VOID(op, Operator, desc, nodeApi->itemId, OpType::OPTYPE_HCCL_BIG);
    hcclBigOpDescs_.emplace_back(op);
}

std::vector<std::shared_ptr<Event>> TreeAnalyzer::GetNodeRecordsByType(const std::shared_ptr<TreeNode> &node,
                                                                       const EventType &type)
{
    std::vector<std::shared_ptr<Event>> records;
    for (const auto &record : node->records)
    {
        if (record->info.type == type)
        {
            records.emplace_back(record);
        }
    }
    return records;
}

std::shared_ptr<HostTask> TreeAnalyzer::GenHostTask(const std::shared_ptr<MsprofCompactInfo> &track,
                                                    const std::shared_ptr<MsprofApi> &modelApi,
                                                    const std::shared_ptr<Operator> &opPtr, uint32_t ctxId,
                                                    uint16_t taskType, int64_t connectionId)
{
    std::shared_ptr<HostTask> task;
    MAKE_SHARED0_RETURN_VALUE(task, HostTask, nullptr);

    task->connection_id = connectionId;
    task->streamId = track->data.runtimeTrack.streamId;
    task->taskId = static_cast<uint32_t>(track->data.runtimeTrack.taskId & GetTaskIdBit());
    task->contextId = ctxId;
    task->op = opPtr;
    task->batchId = GetBatchId(track->data.runtimeTrack.taskId);
    task->requestId = modelApi != nullptr ? modelApi->reserve : INVALID_VALUE;
    task->taskType = taskType;
    task->deviceId = track->data.runtimeTrack.deviceId;
    task->timeStamp = track->timeStamp;
    task->thread_id = track->threadId;
    task->kernelName = track->data.runtimeTrack.kernelName;
    task->modelId = GetModelId(modelApi, task->deviceId, task->streamId, task->batchId, task->timeStamp);
    return task;
}

HostTasks TreeAnalyzer::GenComputeHostTasks(ComputeOpDescs &ops, const std::shared_ptr<MsprofCompactInfo> &track,
                                            int64_t connection_id)
{
    auto modelNode = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ? path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto modelApi = modelNode == nullptr ? nullptr : modelNode->event->apiPtr;
    // L0场景没有上报Node层补充信息且该任务非ctx类任务
    if (ops.empty())
    {
        // 补充一条有效的Op信息
        auto nodeNode = path_.find(MSPROF_REPORT_NODE_LEVEL) != path_.end() ? path_[MSPROF_REPORT_NODE_LEVEL] : nullptr;
        if (nodeNode == nullptr)
        {
            return HostTasks{};
        }
        std::shared_ptr<Operator> op;
        std::shared_ptr<OpDesc> desc;
        MAKE_SHARED_RETURN_VALUE(desc, OpDesc, {});
        desc->runtimeTrackDesc = track;
        MAKE_SHARED_RETURN_VALUE(op, Operator, {}, desc, nodeNode->event->apiPtr->itemId, OpType::OPTYPE_COMPUTE);
        auto task =
            GenHostTask(track, modelApi, op, DEFAULT_CONTEXT_ID, track->data.runtimeTrack.taskType, connection_id);
        return (task != nullptr) ? HostTasks{task} : HostTasks{};
    }

    HostTasks results;
    for (const auto &pair : ops)
    {
        auto desc = pair.second->opDesc;
        if (!desc)
        {
            ERROR("No op % desc found for task timestamp: % when gen compute task", pair.second->name,
                  track->timeStamp);
            continue;
        }

        std::vector<uint32_t> ctxIds;
        // 只有FFTS+类型应该保留CtxId
        if (desc->ctxId && !IsChipV6())
        {
            auto ctxIdInfo = ReinterpretConvert<MsprofContextIdInfo *>(desc->ctxId->data);
            ctxIds.assign(ctxIdInfo->ctxIds, ctxIdInfo->ctxIds + ctxIdInfo->ctxIdNum);
        }
        else
        {
            ctxIds = {DEFAULT_CONTEXT_ID};
        }
        desc->runtimeTrackDesc = track;
        for (const auto &ctxId : ctxIds)
        {
            auto task =
                GenHostTask(track, modelApi, pair.second, ctxId, track->data.runtimeTrack.taskType, connection_id);
            if (task)
            {
                results.emplace_back(task);
            }
        }
    }
    return results;
}

void TreeAnalyzer::UpdateComputeDescForFftsSituation(ComputeOpDescs &descs, const std::shared_ptr<Event> &track)
{
    std::string specialKey;
    // notice： 特殊场景，刷新op描述
    for (auto &pair : descs)
    {
        if (!pair.second->opDesc)
        {
            ERROR("Illegal compute op, no op desc found, threadId = %", threadId_);
            continue;
        }
        if (!pair.second->opDesc->nodeDesc)
        {
            continue;
        }
        // 特殊场景1： ffts+模式会上报一条多余的nodebasic用于标记ffts图，删除
        if (pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType == FFTS_PLUS_TASK_TYPE)
        {
            specialKey = pair.first;
            // 该数据在一个task中只有1条
            break;
        }
    }
    if (!specialKey.empty())
    {
        descs.erase(specialKey);
    }
}

void TreeAnalyzer::UpdateComputeDescForHcclSituation(ComputeOpDescs &descs, const std::shared_ptr<Event> &track,
                                                     uint64_t item_id)
{
    if (descs.empty())
    {
        // 补充一个临时的算子描述，只发生在L0场景
        std::shared_ptr<OpDesc> desc;
        std::shared_ptr<Operator> op;
        MAKE_SHARED0_NO_OPERATION(desc, OpDesc);
        MAKE_SHARED0_NO_OPERATION(op, Operator, desc, item_id, OpType::OPTYPE_COMPUTE_HCCL);
        auto name = Utils::Join("_", std::to_string(PLACEHOLDER_OP_NAME), std::to_string(UINT64_MAX));
        descs[name] = op;
    }
    for (auto &pair : descs)
    {
        if (!pair.second->opDesc)
        {
            ERROR("Illegal compute op, no op desc found, threadId = %", threadId_);
            continue;
        }
        if (!pair.second->opDesc->nodeDesc)
        {
            // 避免内存泄露
            MAKE_SHARED0_NO_OPERATION(pair.second->opDesc->nodeDesc, MsprofCompactInfo);
        }
        // 特殊场景2：hccl类任务将opName刷成item id
        pair.second->opDesc->nodeDesc->data.nodeBasicInfo.opName = item_id;
        auto taskType = track->compactPtr->data.runtimeTrack.taskType;
        auto taskTypeStr = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, taskType);
        if (taskTypeStr == KERNEL_AI_CPU_TASK_TYPE)
        {
            // 特殊场景3：HCCL AICPU任务，该任务需要将任务类型刷为HCCL_AICPU
            pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType = HCCL_AI_CPU_TASK_TYPE;
        }
        else if (taskTypeStr == KERNEL_AI_CORE_TASK_TYPE)
        {
            // 特殊场景4：HCCL AICORE(Reduce TBE)任务，该任务需要将任务类型刷为AI_CORE
            pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType = HCCL_TASK_TYPE;
        }
    }
}

void TreeAnalyzer::UpdateComputeDescForHelperSituation(ComputeOpDescs &descs)
{
    for (auto &pair : descs)
    {
        if (!pair.second->opDesc || !pair.second->opDesc->nodeDesc)
        {
            continue;
        }
        // helper场景：HCCL算子运行在AI_CPU上, 但没有HCCL层api，任务类型刷为AICPU
        if (pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType == HCCL_TASK_TYPE)
        {
            pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType = AICPU_TASK_TYPE;
        }
    }
}

HostTasks TreeAnalyzer::GetComputeTaskDescs(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end())
    {
        return {};
    }
    auto nodeNode = path_.find(MSPROF_REPORT_NODE_LEVEL) != path_.end() ? path_[MSPROF_REPORT_NODE_LEVEL] : nullptr;
    auto modelNode = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ? path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;

    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty())
    {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return {};
    }

    std::string type =
        TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, tracks.back()->compactPtr->data.runtimeTrack.taskType);
    bool useCtxId = CONTEXT_ID_WHITE_LIST.find(type) != CONTEXT_ID_WHITE_LIST.end();
    ComputeOpDescs ops = GetComputeOpDescs(nodeNode, useCtxId);
    // 特殊场景，刷新算子描述
    UpdateComputeDescForFftsSituation(ops, tracks.back());
    if (IsHcclTask())
    {
        auto hcclNode =
            path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) != path_.end() ? path_[MSPROF_REPORT_HCCL_NODE_LEVEL] : nullptr;
        auto item_id = hcclNode == nullptr ? 0 : hcclNode->event->apiPtr->itemId;
        UpdateComputeDescForHcclSituation(ops, tracks.back(), item_id);
    }
    else if (type == KERNEL_AI_CPU_TASK_TYPE)
    {
        UpdateComputeDescForHelperSituation(ops);
    }
    auto track = tracks.back()->compactPtr;

    auto results = GenComputeHostTasks(ops, track, nodeNode->event->id);
    return results;
}

HostTasks TreeAnalyzer::GetHcclTaskDescs(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) == path_.end() || path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end())
    {
        return {};
    }
    auto hcclNode = path_[MSPROF_REPORT_HCCL_NODE_LEVEL];
    auto nodeNode = path_[MSPROF_REPORT_NODE_LEVEL];
    auto modelTrace = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ? path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto modelApi = modelTrace != nullptr ? modelTrace->event->apiPtr : nullptr;

    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty())
    {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return {};
    }
    auto track = tracks.back()->compactPtr;
    HCCLSmallOpDescs hcclOpDescs = GetHcclSmallOpDescs(hcclNode);

    HostTasks results;
    auto ret = Utils::Reserve(results, hcclOpDescs.size());
    if (!ret)
    {
        ERROR("Reserve results failed, threadId = %", threadId_);
        return results;
    }

    for (const auto &pair : hcclOpDescs)
    {
        auto desc = pair.second->hcclSmallOpDesc;
        auto task = GenHostTask(track, modelApi, pair.second, desc->ctxId, track->data.runtimeTrack.taskType,
                                nodeNode->event->id);
        if (task)
        {
            results.emplace_back(task);
        }
    }
    return results;
}

std::shared_ptr<HostTask> TreeAnalyzer::GetOtherTaskDesc(const std::shared_ptr<TreeNode> &node)
{
    auto modelNode = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ? path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto modelApi = modelNode != nullptr ? modelNode->event->apiPtr : nullptr;
    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty())
    {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return nullptr;
    }
    auto track = tracks.back()->compactPtr;
    auto taskType = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, track->data.runtimeTrack.taskType);
    uint32_t contextId = (taskType == KERNEL_MIX_AIC_TASK_TYPE || taskType == KERNEL_MIX_AIV_TASK_TYPE) && !IsChipV6()
                             ? 0
                             : DEFAULT_CONTEXT_ID;
    // 使用父节点的id作为connection_id，主要是为了将record_event的api与task_track关联起来
    std::shared_ptr<Operator> op;
    std::shared_ptr<OpDesc> desc;
    MAKE_SHARED_RETURN_VALUE(desc, OpDesc, {});
    desc->runtimeTrackDesc = track;
    MAKE_SHARED_RETURN_VALUE(op, Operator, {}, desc, track->data.runtimeTrack.kernelName, OpType::OPTYPE_RESERVED);
    auto task = GenHostTask(track, modelApi, op, contextId, track->data.runtimeTrack.taskType, node->parent->event->id);
    return task;
}

ComputeOpDescs TreeAnalyzer::GetComputeOpDescs(const std::shared_ptr<TreeNode> &nodeNode, bool useCtxId)
{
    ComputeOpDescs opDescs;
    if (!nodeNode)
    {
        ERROR("nodeNode is nullptr, threadId = %", threadId_);
        return opDescs;
    }
    for (const auto &record : nodeNode->records)
    {
        if (record->info.type == EventType::EVENT_TYPE_NODE_BASIC_INFO)
        {
            std::shared_ptr<MsprofCompactInfo> trace;
            MAKE_SHARED_RETURN_VALUE(trace, MsprofCompactInfo, opDescs, *(record->compactPtr));
            UpdateComputeOpDescs<MsprofCompactInfo, &OpDesc::nodeDesc>(opDescs, trace, trace->data.nodeBasicInfo.opName,
                                                                       nodeNode->event->id);
        }
        else if (record->info.type == EventType::EVENT_TYPE_NODE_ATTR_INFO)
        {
            std::shared_ptr<MsprofCompactInfo> trace;
            MAKE_SHARED_RETURN_VALUE(trace, MsprofCompactInfo, opDescs, *(record->compactPtr));
            UpdateComputeOpDescs<MsprofCompactInfo, &OpDesc::nodeAttr>(opDescs, trace, trace->data.nodeAttrInfo.opName,
                                                                       nodeNode->event->id);
        }
        else if (record->info.type == EventType::EVENT_TYPE_TENSOR_INFO)
        {
            std::shared_ptr<ConcatTensorInfo> trace;
            MAKE_SHARED_RETURN_VALUE(trace, ConcatTensorInfo, opDescs, *(record->tensorPtr));
            UpdateComputeOpDescs<ConcatTensorInfo, &OpDesc::tensorDesc>(opDescs, trace, trace->opName,
                                                                        nodeNode->event->id);
        }
        else if (record->info.type == EventType::EVENT_TYPE_CONTEXT_ID)
        {
            if (!useCtxId)
            {
                continue;
            }
            std::shared_ptr<MsprofAdditionalInfo> trace;
            MAKE_SHARED_RETURN_VALUE(trace, MsprofAdditionalInfo, opDescs, *(record->additionPtr));
            auto ctxIdNode = ReinterpretConvert<MsprofContextIdInfo *>(trace->data);
            UpdateComputeOpDescs<MsprofAdditionalInfo, &OpDesc::ctxId>(opDescs, trace, ctxIdNode->opName,
                                                                       nodeNode->event->id);
        }
        else if (record->info.type == EventType::EVENT_TYPE_HCCL_OP_INFO)
        {
            continue;
        }
        else
        {
            ERROR("Unsupported additional type = %, threadId = %", static_cast<uint16_t>(record->info.type), threadId_);
        }
    }
    if (!GetNodeRecordsByType(nodeNode, EventType::EVENT_TYPE_CONTEXT_ID).empty())
    {
        std::shared_ptr<OpDesc> desc;
        MAKE_SHARED_RETURN_VALUE(desc, OpDesc, {});
        std::shared_ptr<Operator> op;
        // 层次太深，参数传递过于复杂，逃生通道，用opType区分是否为占位的任务（标记ffts+任务群对应的大任务）
        MAKE_SHARED_RETURN_VALUE(op, Operator, {}, desc, PLACEHOLDER_OP_NAME, OpType::OPTYPE_INVALID);
        auto name = Utils::Join("_", std::to_string(PLACEHOLDER_OP_NAME), std::to_string(UINT64_MAX));
        opDescs.insert({name, op});
    }
    return opDescs;
}

HCCLSmallOpDescs TreeAnalyzer::GetHcclSmallOpDescs(const std::shared_ptr<TreeNode> &hcclNode)
{
    HCCLSmallOpDescs opDescs;
    auto ctxIdRecords = GetNodeRecordsByType(hcclNode, EventType::EVENT_TYPE_CONTEXT_ID);
    auto hcclInfoRecords = GetNodeRecordsByType(hcclNode, EventType::EVENT_TYPE_HCCL_INFO);
    auto hcclApi = hcclNode->event->apiPtr;
    auto isMaster = TypeData::GetInstance().Get(hcclApi->level, hcclApi->type) == "master" ? 1 : 0;

    if (!ctxIdRecords.empty())
    {
        // ctxId 存在，先根据其生成descs, 再补充hccl info
        auto ret = UpdateHcclSmallOpDescs(opDescs, ctxIdRecords, hcclInfoRecords, isMaster);
        if (!ret)
        {
            ERROR("Update hccl small op descs failed by ctxId and hcclInfo, threadId = %", threadId_);
        }
    }
    else if (!hcclInfoRecords.empty())
    {
        // 根据hccl info 生成
        auto ret = UpdateHcclSmallOpDescs(opDescs, hcclInfoRecords, isMaster);
        if (!ret)
        {
            ERROR("Update hccl small op descs failed by hcclInfo, threadId = %", threadId_);
        }
    }
    else
    {
        std::shared_ptr<HcclSmallOpDesc> desc;
        MAKE_SHARED_RETURN_VALUE(desc, HcclSmallOpDesc, opDescs, DEFAULT_CONTEXT_ID, isMaster, nullptr);
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VALUE(op, Operator, opDescs, desc, 0, OpType::OPTYPE_HCCL_SMALL);
        opDescs.insert({DEFAULT_CONTEXT_ID, op});
    }

    return opDescs;
}

bool TreeAnalyzer::UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                          const std::vector<std::shared_ptr<Event>> &ctxIdRecords,
                                          const std::vector<std::shared_ptr<Event>> &hcclInfoRecords, uint8_t isMaster)
{
    // 根据ctxId生成descs
    for (const auto &record : ctxIdRecords)
    {
        auto trace = record->additionPtr;
        auto ctxIdTrace = ReinterpretConvert<MsprofContextIdInfo *>(trace->data);
        if (ctxIdTrace->ctxIdNum != VALID_CTXID_NUM)
        {
            ERROR("Expect ctxIdNum is 2, but get ctxIdNum is %, threadId = %", ctxIdTrace->ctxIdNum, threadId_);
            return false;
        }
        for (uint32_t id = ctxIdTrace->ctxIds[0]; id <= ctxIdTrace->ctxIds[1]; ++id)
        {
            std::shared_ptr<HcclSmallOpDesc> desc;
            MAKE_SHARED_RETURN_VALUE(desc, HcclSmallOpDesc, false, DEFAULT_CONTEXT_ID, isMaster, nullptr);
            desc->ctxId = id;
            std::shared_ptr<Operator> op;
            MAKE_SHARED_RETURN_VALUE(op, Operator, false, desc, ctxIdTrace->opName, OpType::OPTYPE_HCCL_SMALL);
            descs.insert({id, op});
        }
    }
    if (!ctxIdRecords.empty())
    {
        std::shared_ptr<HcclSmallOpDesc> desc;
        MAKE_SHARED_RETURN_VALUE(desc, HcclSmallOpDesc, false, DEFAULT_CONTEXT_ID, isMaster, nullptr);
        std::shared_ptr<Operator> op;
        // 层次太深，参数传递过于复杂，逃生通道，用opType区分是否为占位的任务（标记ffts+任务群对应的大任务）
        MAKE_SHARED_RETURN_VALUE(op, Operator, false, desc, PLACEHOLDER_OP_NAME, OpType::OPTYPE_INVALID);
        descs.insert({DEFAULT_CONTEXT_ID, op});
    }

    // HcclInfo更新
    for (const auto &record : hcclInfoRecords)
    {
        auto trace = record->additionPtr;
        auto hcclTrace = ReinterpretConvert<MsprofHcclInfo *>(trace->data);
        auto key = hcclTrace->ctxID;
        if (descs.find(key) != descs.end())
        {
            auto hcclPtr = descs[key]->hcclSmallOpDesc;
            hcclPtr->hcclInfo = trace;
        }
        else
        {
            ERROR("Can not find ctxId : % in descs, timestamp = %, threadId = %", key, trace->timeStamp, threadId_);
        }
    }
    return true;
}

bool TreeAnalyzer::UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                          const std::vector<std::shared_ptr<Event>> &hcclInfoRecords, uint8_t isMaster)
{
    for (const auto &record : hcclInfoRecords)
    {
        auto trace = record->additionPtr;
        auto hcclTrace = ReinterpretConvert<MsprofHcclInfo *>(trace->data);
        auto key = hcclTrace->ctxID;

        std::shared_ptr<HcclSmallOpDesc> desc;
        MAKE_SHARED_RETURN_VALUE(desc, HcclSmallOpDesc, false, DEFAULT_CONTEXT_ID, isMaster, nullptr);
        desc->hcclInfo = trace;
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VALUE(op, Operator, false, desc, hcclTrace->itemId, OpType::OPTYPE_HCCL_SMALL);

        descs.insert({key, op});
        auto hcclPtr = descs[key]->hcclSmallOpDesc;
        hcclPtr->hcclInfo = trace;
    }
    return true;
}

}  // namespace Cann
}  // namespace Domain
}  // namespace Analysis
