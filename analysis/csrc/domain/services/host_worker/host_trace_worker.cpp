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

#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"

#include "analysis/csrc/domain/services/association/cann/include/tree_analyzer.h"
#include "analysis/csrc/domain/services/association/cann/include/tree_builder.h"
#include "analysis/csrc/domain/services/persistence/host/api_event_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/flip_task_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/memcpy_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"

using namespace Analysis::Domain::Cann;

namespace Analysis
{
namespace Domain
{

bool HostTraceWorker::Run()
{
    TimeLogger t{"HostTraceWorker"};
    // 1. 解析生成CANNWarehouse
    auto hostDataPath = Utils::File::PathJoin({hostPath_, "data"});
    std::shared_ptr<EventGrouper> grouper;
    MAKE_SHARED_RETURN_VALUE(grouper, EventGrouper, false, hostDataPath);
    grouper->Group();

    cannWarehouses_ = grouper->GetGroupEvents();
    threadIds_ = grouper->GetThreadIdSet();
    ThreadPool pool(poolSize_);
    pool.Start();
    DumpApiEvent(pool, grouper);
    DumpDpuTaskTrack(pool, grouper);
    if (!cannWarehouses_.Empty())
    {
        DumpFlipTask(pool, grouper);
        DumpModelName(pool, hostDataPath);
        pool.AddTask(
            [this]()
            {
                // 建树
                // 建树前已经对KernelEvents排序
                MultiThreadBuildTree();
                // 分析树 & DB Dump
                MultiThreadAnalyzeTreeDumpData();
            });
    }
    pool.WaitAllTasks();
    pool.Stop();
    DumpMemcpyInfo(hostDataPath);  // 依赖runtime.db中的HostTask, 不能放在pool中
    return true;
}

void HostTraceWorker::MultiThreadBuildTree()
{
    TimeLogger t{"Multi thread build tree"};
    ThreadPool pool(poolSize_);
    pool.Start();
    for (auto tid : threadIds_)
    {
        pool.AddTask(
            [this, tid]()
            {
                INFO("Start multi thread build tree, threadId = %", tid);
                std::shared_ptr<CANNWarehouse> cannWareHouse;
                MAKE_SHARED_RETURN_VOID(cannWareHouse, CANNWarehouse, cannWarehouses_[tid]);
                std::shared_ptr<TreeBuilder> treeBuilder;
                MAKE_SHARED_RETURN_VOID(treeBuilder, TreeBuilder, cannWareHouse, tid);
                auto treeNode = treeBuilder->Build();
                if (treeNode)
                {
                    // 保存建树完成的根节点
                    std::lock_guard<std::mutex> lock(mutex_);
                    treeNodes_.emplace_back(tid, treeNode);
                    INFO("Multi thread build tree done, threadId = %", tid);
                }
            });
    }

    pool.WaitAllTasks();
    pool.Stop();
}

void HostTraceWorker::MultiThreadAnalyzeTreeDumpData()
{
    TimeLogger t{"Multi thread analyze tree and dump data"};
    ThreadPool pool(poolSize_);
    pool.Start();
    for (auto &p : treeNodes_)
    {
        pool.AddTask(
            [this, p]()
            {
                INFO("Start analyze tree and dump data, threadId = %", p.first);
                // 分析
                TreeAnalyzer ana{p.second, p.first};
                ana.Analyze();
                // 落盘
                std::shared_ptr<CANNTraceDBDumper> dumper;
                MAKE_SHARED_RETURN_VOID(dumper, CANNTraceDBDumper, hostPath_);
                if (dumper->DumpData(ana))
                {
                    INFO("Dump cann trace data done, threadId = %", p.first);
                }
                else
                {
                    ERROR("Dump cann trace data failed, threadId = %", p.first);
                }
            });
    }

    pool.WaitAllTasks();
    pool.Stop();
}

void HostTraceWorker::DumpApiEvent(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, &grouper]()
        {
            TimeLogger t{"Dump api data start"};
            // api event 数据落盘
            auto apiTraces = grouper->GetApiTraces();
            std::shared_ptr<ApiEventDBDumper> apiDumper;
            MAKE_SHARED_RETURN_VOID(apiDumper, ApiEventDBDumper, hostPath_);
            auto ret = apiDumper->DumpData(apiTraces);
            if (!ret)
            {
                ERROR("Dump api traces data failed");
            }
        });
}

void HostTraceWorker::DumpFlipTask(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, &grouper]()
        {
            TimeLogger t{"Dump flip tasks data start"};
            // flip tasks 数据落盘
            auto flipTasks = grouper->GetFlipTasks();
            std::shared_ptr<FlipTaskDBDumper> flipDumper;
            MAKE_SHARED_RETURN_VOID(flipDumper, FlipTaskDBDumper, hostPath_);
            auto ret = flipDumper->DumpData(flipTasks);
            if (!ret)
            {
                ERROR("Dump flip tasks data failed");
            }
        });
}

void HostTraceWorker::DumpModelName(ThreadPool &pool, const std::string &hostDataPath)
{
    pool.AddTask(
        [this, &hostDataPath]()
        {
            TimeLogger t{"Dump model name data start"};
            std::shared_ptr<GraphIdParser> parser;
            MAKE_SHARED_RETURN_VOID(parser, GraphIdParser, hostDataPath);
            auto traces = parser->ParseData<MsprofAdditionalInfo>();
            // ModelName 数据落盘
            std::shared_ptr<ModelNameDBDumper> modelNameDumper;
            MAKE_SHARED_RETURN_VOID(modelNameDumper, ModelNameDBDumper, hostPath_);
            auto ret = modelNameDumper->DumpData(traces);
            if (!ret)
            {
                ERROR("Dump model name data failed");
            }
        });
}

void HostTraceWorker::DumpDpuTaskTrack(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, &grouper]()
        {
            TimeLogger t{"Dump dpu task track data start"};
            auto dpuTrackData = grouper->GetDpuTrackData();
            auto &dpuKernelNameMap = grouper->GetDpuKernelNameMap();
            std::shared_ptr<DpuTaskTrackDBDumper> dpuDumper;
            MAKE_SHARED_RETURN_VOID(dpuDumper, DpuTaskTrackDBDumper, hostPath_);
            dpuDumper->SetKernelNameMap(dpuKernelNameMap);
            auto ret = dpuDumper->DumpData(dpuTrackData);
            if (!ret)
            {
                ERROR("Dump dpu task track data failed");
            }
        });
}

void HostTraceWorker::DumpMemcpyInfo(const std::string &hostDataPath)
{
    TimeLogger t{"Dump memcpy info data start"};
    std::shared_ptr<MemcpyInfoParser> parser;
    MAKE_SHARED_RETURN_VOID(parser, MemcpyInfoParser, hostDataPath);
    auto traces = parser->ParseData<MsprofCompactInfo>();
    // MemcpyInfo 数据落盘
    std::shared_ptr<MemcpyInfoDumper> memcpyInfoDumper;
    MAKE_SHARED_RETURN_VOID(memcpyInfoDumper, MemcpyInfoDumper, hostPath_);
    auto ret = memcpyInfoDumper->DumpData(traces);
    if (!ret)
    {
        ERROR("Dump memcpy info data failed");
    }
}
}  // namespace Domain
}  // namespace Analysis
