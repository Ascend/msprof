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

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H

#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis
{
namespace Domain
{
namespace Cann
{

using GeFusionOpInfo = Analysis::Domain::GeFusionOpInfo;
using GeFusionOpInfos = std::vector<std::shared_ptr<GeFusionOpInfo>>;
using HostTask = Analysis::Domain::HostTask;
using HostTasks = std::vector<std::shared_ptr<HostTask>>;
using Operator = Analysis::Domain::Operator;
// HCCL大算子描述信息
using HCCLBigOpDescs = std::vector<std::shared_ptr<Operator>>;
// 计算类算子描述信息，Key为opName和timestamp拼接成的字符串
using ComputeOpDescs = std::unordered_map<std::string, std::shared_ptr<Operator>>;
// HCCL小算子描述信息，Key为ctxId
using HCCLSmallOpDescs = std::unordered_map<uint32_t, std::shared_ptr<Operator>>;

class TreeAnalyzer
{
   public:
    using TypeData = Analysis::Domain::Host::Cann::TypeData;
    using TreeNode = Analysis::Domain::TreeNode;
    using Event = Analysis::Domain::Event;
    using EventType = Analysis::Domain::EventType;

    TreeAnalyzer(const std::shared_ptr<TreeNode> &node, uint32_t threadId) : root_(node), threadId_(threadId) {}
    // 入口函数
    void Analyze();
    // 获取HCCL相关Tasks数据
    HostTasks &GetHCCLTasks();
    // 获取计算类Tasks数据
    HostTasks &GetComputeTasks();
    // 获取所有Tasks数据
    HostTasks &GetTasks();
    // 获取HCCL大算子数据
    HCCLBigOpDescs &GetHcclBigOps();
    // 获取ge_model_info的GeFusionOpInfo
    GeFusionOpInfos &GetGeFusionOpInfos();

   private:
    // 更新计算类算子模板函数
    template <typename T, std::shared_ptr<T> Domain::OpDesc::*element>
    void UpdateComputeOpDescs(ComputeOpDescs &opDescs, const std::shared_ptr<T> &trace, uint64_t opName,
                              uint64_t nodeEventId)
    {
        std::string key = std::to_string(nodeEventId);
        if (opDescs.find(key) == opDescs.end())
        {
            std::shared_ptr<Domain::OpDesc> desc;
            MAKE_SHARED0_RETURN_VOID(desc, Domain::OpDesc);
            (*desc).*element = trace;
            std::shared_ptr<Operator> op;
            MAKE_SHARED_RETURN_VOID(op, Operator, desc, opName, Domain::OpType::OPTYPE_COMPUTE);
            opDescs.insert({key, op});
        }
        else
        {
            auto desc = opDescs[key]->opDesc;
            (*desc).*element = trace;
        }
    }

   private:
    // DFS遍历整棵树，记录分析树节点信息保存为落盘数据
    void DeepFirstSearch(const std::shared_ptr<TreeNode> &node);
    // 分析TreeNode入口，此函数内根据其所处的level走不同的逻辑
    void AnalyzeNode(const std::shared_ptr<TreeNode> &node);
    // 分析Runtime层的TreeNode，当DFS到Runtime层时进入此函数
    void AnalyzeRuntimeNode(const std::shared_ptr<TreeNode> &node);
    // 分析Model层的TreeNode，当DFS到Model层时进入此函数
    void AnalyzeModelNode(const std::shared_ptr<TreeNode> &node);

    // 判断当前task是否属于通讯类
    bool IsHcclTask();
    // 判断当前task是否属于计算类
    bool IsComputeTask(const std::shared_ptr<TreeNode> &node);

    // 获取当前计算类节点的信息
    HostTasks GetComputeTaskDescs(const std::shared_ptr<TreeNode> &node);
    // 获取HCCL类节点的信息
    HostTasks GetHcclTaskDescs(const std::shared_ptr<TreeNode> &node);
    // 获取其他节点的信息
    std::shared_ptr<HostTask> GetOtherTaskDesc(const std::shared_ptr<TreeNode> &node);
    // 获取算子信息，key为OpName + timeStamp
    ComputeOpDescs GetComputeOpDescs(const std::shared_ptr<TreeNode> &nodeNode, bool useCtxId);
    // 获取TreeNode中指定EventType的Records
    std::vector<std::shared_ptr<Event>> GetNodeRecordsByType(const std::shared_ptr<TreeNode> &node,
                                                             const EventType &type);
    // 获取HcclNode信息
    HCCLSmallOpDescs GetHcclSmallOpDescs(const std::shared_ptr<TreeNode> &hcclNode);
    // 更新Hccl大算子描述信息
    void UpdateHcclBigOpDescs(const std::shared_ptr<TreeNode> &node);
    // 根据ctxId和hcclinfo更新hccl小算子描述信息
    bool UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs, const std::vector<std::shared_ptr<Event>> &ctxIDRecords,
                                const std::vector<std::shared_ptr<Event>> &hcclInfoRecords, uint8_t isMaster);
    // 根据hcclinfo更新hccl小算子描述信息
    bool UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs, const std::vector<std::shared_ptr<Event>> &hcclInfoRecords,
                                uint8_t isMaster);

    // 根据输入参数生成HostTask，该函数纯粹负责HostTask生成
    std::shared_ptr<HostTask> GenHostTask(const std::shared_ptr<ParserCompactInfo> &track,
                                          const std::shared_ptr<ParserApi> &modelApi,
                                          const std::shared_ptr<Operator> &opPtr, uint32_t ctxId, uint16_t taskType,
                                          int64_t connectionId);
    // 根据输入参数生成HostTask数组
    HostTasks GenComputeHostTasks(ComputeOpDescs &ops, const std::shared_ptr<ParserCompactInfo> &track,
                                  int64_t connectionId);

    void UpdateComputeDescForFftsSituation(ComputeOpDescs &descs, const std::shared_ptr<Event> &track);
    void UpdateComputeDescForHcclSituation(ComputeOpDescs &descs, const std::shared_ptr<Event> &track,
                                           uint64_t item_id);
    static void UpdateComputeDescForHelperSituation(ComputeOpDescs &descs);

   private:
    // 树的root节点
    std::shared_ptr<TreeNode> root_;
    // 此对象处理的threadId
    uint32_t threadId_ = 0;
    // 路径记录, key为树的level
    std::map<uint16_t, std::shared_ptr<TreeNode>> path_;
    // 所有Task信息
    HostTasks tasks_;
    // HCCL相关Task信息
    HostTasks hcclTasks_;
    // 计算类Task信息
    HostTasks computeTasks_;
    // HCCL大算子信息
    HCCLBigOpDescs hcclBigOpDescs_;
    // GeFusionOpInfo表
    GeFusionOpInfos geFusionOpInfos_;
    // model节点的map，记录该节点是否被搜索过
    std::unordered_set<std::string> visitedModel_;
};

}  // namespace Cann
}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H
