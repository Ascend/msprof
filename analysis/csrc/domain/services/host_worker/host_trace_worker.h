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

#ifndef ANALYSIS_WORKER_HOST_TRACE_THREAD_H
#define ANALYSIS_WORKER_HOST_TRACE_THREAD_H
#include <set>
#include <string>
#include <utility>

#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "analysis/csrc/domain/services/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"
#include "analysis/csrc/infrastructure/utils/safe_unordered_map.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

namespace Analysis
{
namespace Domain
{
using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

// HostTraceWorker负责控制CANN Host侧数据解析->EventQueue->建树->分析树->Dump 流程
// 此流程由Worker拉起
class HostTraceWorker
{
    using TreeNode = Analysis::Domain::TreeNode;
    using CANNWarehouse = Analysis::Domain::Host::Cann::CANNWarehouse;
    using CANNWarehouses = Analysis::Utils::SafeUnorderedMap<uint32_t, CANNWarehouse>;

   public:
    // hostPath为采集侧落盘host二进制数据路径
    explicit HostTraceWorker(const std::string &hostPath) : hostPath_(hostPath) {}
    // 启动整个流程
    bool Run();

   private:
    void MultiThreadBuildTree();
    void MultiThreadAnalyzeTreeDumpData();
    void DumpApiEvent(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper);
    void DumpDpuTaskTrack(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper);
    void DumpFlipTask(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper);
    void DumpModelName(ThreadPool &pool, const std::string &hostDataPath);
    void DumpMemcpyInfo(const std::string &hostDataPath);

   private:
    const uint32_t poolSize_ = 10;
    std::mutex mutex_;
    std::vector<std::pair<uint32_t, std::shared_ptr<TreeNode>>> treeNodes_;
    std::set<uint32_t> threadIds_;
    std::string hostPath_;
    CANNWarehouses cannWarehouses_;
};

}  // namespace Domain
}  // namespace Analysis
#endif  // ANALYSIS_WORKER_HOST_TRACE_THREAD_H
