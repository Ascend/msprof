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
#include "analysis/csrc/domain/services/host_worker/host_cpu_freq_parser.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/persistence/host/api_event_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/flip_task_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/mc2_comm_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/memcpy_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/runtime_op_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/static_op_mem_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/stream_expand_spec_db_dumper.h"

using namespace Analysis::Domain::Cann;

namespace Analysis
{
namespace Domain
{

bool HostTraceWorker::Run()
{
    TimeLogger t{"HostTraceWorker"};
    auto hostDataPath = Utils::File::PathJoin({hostPath_, "data"});
    std::shared_ptr<EventGrouper> grouper;
    MAKE_SHARED_RETURN_VALUE(grouper, EventGrouper, false, hostDataPath);
    bool result = grouper->Group();
    CaptureStreamInfoData formattedCaptureData;
    PrepareLookupData(grouper, formattedCaptureData);
    cannWarehouses_ = grouper->GetGroupEvents();
    threadIds_ = grouper->GetThreadIdSet();
    DumpHostData(grouper, formattedCaptureData);
    return result;
}

void HostTraceWorker::PrepareLookupData(const std::shared_ptr<EventGrouper> &grouper,
                                        CaptureStreamInfoData &formattedCaptureData)
{
    // 建树 GetModelId 依赖内存中的 Capture 时间窗，必须在 dump/建树前注入
    PrepareCaptureStreamInfo(grouper, formattedCaptureData);
    auto sqlitePath = Utils::File::PathJoin({hostPath_, "sqlite"});
    RTAddInfoCenter::GetInstance().Load(sqlitePath);
}

void HostTraceWorker::DumpHostData(const std::shared_ptr<EventGrouper> &grouper,
                                   const CaptureStreamInfoData &formattedCaptureData)
{
    ThreadPool pool(poolSize_);
    pool.Start();
    DumpAsyncHostData(pool, grouper, formattedCaptureData);
    pool.WaitAllTasks();
    pool.Stop();
    DumpMemcpyInfo(grouper);  // 依赖runtime.db中的HostTask, 不能放在pool中
}

void HostTraceWorker::DumpAsyncHostData(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper,
                                        const CaptureStreamInfoData &formattedCaptureData)
{
    DumpCaptureStreamInfo(pool, formattedCaptureData);
    DumpMc2CommInfo(pool, grouper, formattedCaptureData);
    DumpDpuTaskTrack(pool, grouper);
    DumpStreamExpandSpec(pool, grouper);
    // DumpStaticOpMem(pool, grouper);
    DumpHostSystemProfileData(pool);
    DumpApiEvent(pool, grouper);
    DumpRtsTrackData(pool, grouper);
    DumpModelName(pool, grouper);
    DumpCannTrace(pool);
}

void HostTraceWorker::DumpRtsTrackData(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, grouper]()
        {
            DumpRuntimeOpInfo();
            if (!cannWarehouses_.Empty())
            {
                DumpFlipTask(grouper);
            }
        });
}

void HostTraceWorker::DumpCannTrace(ThreadPool &pool)
{
    if (cannWarehouses_.Empty())
    {
        return;
    }
    pool.AddTask(
        [this]()
        {
            MultiThreadBuildTree();
            MultiThreadAnalyzeTreeDumpData();
        });
}

void HostTraceWorker::PrepareCaptureStreamInfo(const std::shared_ptr<EventGrouper> &grouper,
                                               CaptureStreamInfoData &formattedCaptureData)
{
    RTAddInfoCenter::GetInstance().SetCaptureStreamInfoData({});
    const auto &captureData = grouper->GetDumpWarehouse().captureStreamInfoData;
    if (captureData.empty())
    {
        return;
    }

    CaptureStreamInfoDumper captureDumper(hostPath_);
    formattedCaptureData = captureDumper.FormatData(captureData);
    if (formattedCaptureData.empty())
    {
        ERROR("Format capture stream info failed.");
        return;
    }

    std::vector<Analysis::Domain::CaptureStreamInfo> centerData;
    if (!Utils::Reserve(centerData, formattedCaptureData.size()))
    {
        ERROR("Reserve capture stream info center data failed.");
        return;
    }
    for (const auto &item : formattedCaptureData)
    {
        centerData.emplace_back(item.modelId, item.timeStamp, item.streamId, item.originalStreamId, item.deviceId,
                                static_cast<uint16_t>(item.batchId), item.captureStatus);
    }
    RTAddInfoCenter::GetInstance().SetCaptureStreamInfoData(centerData);
}

void HostTraceWorker::DumpCaptureStreamInfo(ThreadPool &pool, const CaptureStreamInfoData &formattedCaptureData)
{
    if (formattedCaptureData.empty())
    {
        return;
    }
    pool.AddTask(
        [this, formattedCaptureData]()
        {
            CaptureStreamInfoDumper captureDumper(hostPath_);
            if (!captureDumper.DumpData(formattedCaptureData))
            {
                ERROR("Dump capture stream info failed.");
            }
        });
}

void HostTraceWorker::DumpMc2CommInfo(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper,
                                      const CaptureStreamInfoData &formattedCaptureData)
{
    pool.AddTask(
        [this, grouper, formattedCaptureData]()
        {
            const auto &mc2Data = grouper->GetDumpWarehouse().mc2CommInfoData;
            if (mc2Data.empty())
            {
                return;
            }
            Mc2CommInfoDumper mc2Dumper(hostPath_, formattedCaptureData);
            if (!mc2Dumper.DumpData(mc2Data))
            {
                ERROR("Dump mc2 comm info failed.");
            }
        });
}

void HostTraceWorker::DumpHostSystemProfileData(ThreadPool &pool)
{
    pool.AddTask(
        [this]()
        {
            INFO("Start parse host system profile data");
            // 后续host数据解析统一重构
            if (HostCpuFreqParser(hostPath_).Run() != ANALYSIS_OK)
            {
                ERROR("Host cpu freq parse failed");
            }
        });
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

void HostTraceWorker::DumpRuntimeOpInfo()
{
    auto &center = RTAddInfoCenter::GetInstance();
    if (!center.LoadedFromBinary() || center.Empty())
    {
        return;
    }
    TimeLogger t{"Dump runtime op info"};
    std::shared_ptr<RuntimeOpInfoDumper> dumper;
    MAKE_SHARED_RETURN_VOID(dumper, RuntimeOpInfoDumper, hostPath_);
    if (!dumper->DumpData(center.GetDumpList()))
    {
        ERROR("Dump runtime op info data failed");
    }
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

void HostTraceWorker::DumpFlipTask(const std::shared_ptr<EventGrouper> &grouper)
{
    TimeLogger t{"Dump flip tasks data start"};
    auto flipTasks = grouper->GetFlipTasks();
    std::shared_ptr<FlipTaskDBDumper> flipDumper;
    MAKE_SHARED_RETURN_VOID(flipDumper, FlipTaskDBDumper, hostPath_);
    auto ret = flipDumper->DumpData(flipTasks);
    if (!ret)
    {
        ERROR("Dump flip tasks data failed");
    }
}

void HostTraceWorker::DumpModelName(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, grouper]()
        {
            TimeLogger t{"Dump model name data start"};
            std::shared_ptr<ModelNameDBDumper> modelNameDumper;
            MAKE_SHARED_RETURN_VOID(modelNameDumper, ModelNameDBDumper, hostPath_);
            auto ret = modelNameDumper->DumpData(grouper->GetDumpWarehouse().graphIdMapData);
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
            auto dpuTrackData = grouper->GetDumpWarehouse().dpuTrackData;
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

void HostTraceWorker::DumpStreamExpandSpec(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, grouper]()
        {
            TimeLogger t{"Dump stream expand spec data start"};
            StreamExpandSpecDBDumper dumper(hostPath_);
            if (!dumper.DumpData(grouper->GetDumpWarehouse().streamExpandSpecData))
            {
                ERROR("Dump stream expand spec data failed");
            }
        });
}

void HostTraceWorker::DumpStaticOpMem(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    pool.AddTask(
        [this, grouper]()
        {
            TimeLogger t{"Dump static op memory data start"};
            StaticOpMemDBDumper dumper(hostPath_);
            if (!dumper.DumpData(grouper->GetDumpWarehouse().staticOpMemData))
            {
                ERROR("Dump static op memory data failed");
            }
        });
}

void HostTraceWorker::DumpMemcpyInfo(const std::shared_ptr<EventGrouper> &grouper)
{
    TimeLogger t{"Dump memcpy info data start"};
    std::shared_ptr<MemcpyInfoDumper> memcpyInfoDumper;
    MAKE_SHARED_RETURN_VOID(memcpyInfoDumper, MemcpyInfoDumper, hostPath_);
    auto ret = memcpyInfoDumper->DumpData(grouper->GetDumpWarehouse().memcpyInfoData);
    if (!ret)
    {
        ERROR("Dump memcpy info data failed");
    }
}
}  // namespace Domain
}  // namespace Analysis
