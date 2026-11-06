/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"

#include <exception>

#include "analysis/csrc/domain/services/association/cann/include/tree_analyzer.h"
#include "analysis/csrc/domain/services/association/cann/include/tree_builder.h"
#include "analysis/csrc/domain/services/host_worker/host_cpu_freq_parser.h"
#include "analysis/csrc/domain/services/parser/host/cann/rt_add_info_center.h"
#include "analysis/csrc/domain/services/persistence/host/api_event_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/capture_stream_info_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/ccu_add_info_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_hccl_track_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/flip_task_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/ge_logic_stream_dumper.h"
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

void HostTraceWorker::AddCheckedTask(ThreadPool &pool, const std::string &step, const Task &task)
{
    pool.AddTask(
        [this, step, task]()
        {
            try
            {
                task();
            }
            catch (const std::exception &ex)
            {
                succeeded_ = false;
                ERROR("Host trace task % failed, host path: %, exception: %.", step, hostPath_, ex.what());
            }
            catch (...)
            {
                succeeded_ = false;
                ERROR("Host trace task % failed, host path: %, unknown exception.", step, hostPath_);
            }
        });
}

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
    return result && succeeded_.load();
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
    if (!pool.Start())
    {
        succeeded_ = false;
        return;
    }
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
    // DPU HCCL / GE persistence is temporarily disabled with their lookup calls.
    // DumpDpuHcclTrack(pool, grouper);
    // DumpGeLogicStream(pool, grouper);
    DumpStreamExpandSpec(pool, grouper);
    // DumpStaticOpMem(pool, grouper);
    DumpHostSystemProfileData(pool);
    DumpCcuAddInfo(pool, grouper);
    DumpApiEvent(pool, grouper);
    DumpRtsTrackData(pool, grouper);
    DumpModelName(pool, grouper);
    DumpCannTrace(pool);
}

void HostTraceWorker::DumpRtsTrackData(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpRtsTrackData",
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
    AddCheckedTask(pool, "DumpCannTrace",
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
        succeeded_ = false;
        ERROR("Format capture stream info failed.");
        return;
    }

    std::vector<Analysis::Domain::CaptureStreamInfo> centerData;
    if (!Utils::Reserve(centerData, formattedCaptureData.size()))
    {
        succeeded_ = false;
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
    AddCheckedTask(pool, "DumpCaptureStreamInfo",
                   [this, formattedCaptureData]()
                   {
                       CaptureStreamInfoDumper captureDumper(hostPath_);
                       if (!captureDumper.DumpData(formattedCaptureData))
                       {
                           succeeded_ = false;
                           ERROR("Dump capture stream info failed.");
                       }
                   });
}

void HostTraceWorker::DumpMc2CommInfo(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper,
                                      const CaptureStreamInfoData &formattedCaptureData)
{
    AddCheckedTask(pool, "DumpMc2CommInfo",
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
                           succeeded_ = false;
                           ERROR("Dump mc2 comm info failed.");
                       }
                   });
}

void HostTraceWorker::DumpHostSystemProfileData(ThreadPool &pool)
{
    AddCheckedTask(pool, "DumpHostSystemProfileData",
                   [this]()
                   {
                       INFO("Start parse host system profile data");
                       // 后续host数据解析统一重构
                       if (HostCpuFreqParser(hostPath_).Run() != ANALYSIS_OK)
                       {
                           succeeded_ = false;
                           ERROR("Host cpu freq parse failed");
                       }
                   });
}

void HostTraceWorker::MultiThreadBuildTree()
{
    TimeLogger t{"Multi thread build tree"};
    ThreadPool pool(poolSize_);
    if (!pool.Start())
    {
        succeeded_ = false;
        return;
    }
    for (auto tid : threadIds_)
    {
        AddCheckedTask(pool, "MultiThreadBuildTree tid=" + std::to_string(tid),
                       [this, tid]()
                       {
                           INFO("Start multi thread build tree, threadId = %", tid);
                           std::shared_ptr<CANNWarehouse> cannWareHouse;
                           cannWareHouse.reset(new (std::nothrow) CANNWarehouse(cannWarehouses_[tid]));
                           if (!cannWareHouse)
                           {
                               succeeded_ = false;
                               return;
                           }
                           std::shared_ptr<TreeBuilder> treeBuilder;
                           treeBuilder.reset(new (std::nothrow) TreeBuilder(cannWareHouse, tid));
                           if (!treeBuilder)
                           {
                               succeeded_ = false;
                               return;
                           }
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
    if (!pool.Start())
    {
        succeeded_ = false;
        return;
    }
    for (auto &p : treeNodes_)
    {
        AddCheckedTask(pool, "MultiThreadAnalyzeTreeDumpData tid=" + std::to_string(p.first),
                       [this, p]()
                       {
                           INFO("Start analyze tree and dump data, threadId = %", p.first);
                           // 分析
                           TreeAnalyzer ana{p.second, p.first};
                           ana.Analyze();
                           // 落盘
                           std::shared_ptr<CANNTraceDBDumper> dumper;
                           dumper.reset(new (std::nothrow) CANNTraceDBDumper(hostPath_));
                           if (!dumper)
                           {
                               succeeded_ = false;
                               return;
                           }
                           if (dumper->DumpData(ana))
                           {
                               INFO("Dump cann trace data done, threadId = %", p.first);
                           }
                           else
                           {
                               succeeded_ = false;
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
    dumper.reset(new (std::nothrow) RuntimeOpInfoDumper(hostPath_));
    if (!dumper)
    {
        succeeded_ = false;
        return;
    }
    if (!dumper->DumpData(center.GetDumpList()))
    {
        succeeded_ = false;
        ERROR("Dump runtime op info data failed");
    }
}

void HostTraceWorker::DumpApiEvent(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpApiEvent",
                   [this, &grouper]()
                   {
                       TimeLogger t{"Dump api data start"};
                       // api event 数据落盘
                       auto apiTraces = grouper->GetApiTraces();
                       std::shared_ptr<ApiEventDBDumper> apiDumper;
                       apiDumper.reset(new (std::nothrow) ApiEventDBDumper(hostPath_));
                       if (!apiDumper)
                       {
                           succeeded_ = false;
                           return;
                       }
                       auto ret = apiDumper->DumpData(apiTraces);
                       if (!ret)
                       {
                           succeeded_ = false;
                           ERROR("Dump api traces data failed");
                       }
                   });
}

void HostTraceWorker::DumpFlipTask(const std::shared_ptr<EventGrouper> &grouper)
{
    TimeLogger t{"Dump flip tasks data start"};
    auto flipTasks = grouper->GetFlipTasks();
    std::shared_ptr<FlipTaskDBDumper> flipDumper;
    flipDumper.reset(new (std::nothrow) FlipTaskDBDumper(hostPath_));
    if (!flipDumper)
    {
        succeeded_ = false;
        return;
    }
    auto ret = flipDumper->DumpData(flipTasks);
    if (!ret)
    {
        succeeded_ = false;
        ERROR("Dump flip tasks data failed");
    }
}

void HostTraceWorker::DumpModelName(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpModelName",
                   [this, grouper]()
                   {
                       TimeLogger t{"Dump model name data start"};
                       std::shared_ptr<ModelNameDBDumper> modelNameDumper;
                       modelNameDumper.reset(new (std::nothrow) ModelNameDBDumper(hostPath_));
                       if (!modelNameDumper)
                       {
                           succeeded_ = false;
                           return;
                       }
                       auto ret = modelNameDumper->DumpData(grouper->GetDumpWarehouse().graphIdMapData);
                       if (!ret)
                       {
                           succeeded_ = false;
                           ERROR("Dump model name data failed");
                       }
                   });
}

void HostTraceWorker::DumpDpuTaskTrack(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpDpuTaskTrack",
                   [this, &grouper]()
                   {
                       TimeLogger t{"Dump dpu task track data start"};
                       auto dpuTrackData = grouper->GetDumpWarehouse().dpuTrackData;
                       auto &dpuKernelNameMap = grouper->GetDpuKernelNameMap();
                       std::shared_ptr<DpuTaskTrackDBDumper> dpuDumper;
                       dpuDumper.reset(new (std::nothrow) DpuTaskTrackDBDumper(hostPath_));
                       if (!dpuDumper)
                       {
                           succeeded_ = false;
                           return;
                       }
                       dpuDumper->SetKernelNameMap(dpuKernelNameMap);
                       auto ret = dpuDumper->DumpData(dpuTrackData);
                       if (!ret)
                       {
                           succeeded_ = false;
                           ERROR("Dump dpu task track data failed");
                       }
                   });
}

void HostTraceWorker::DumpDpuHcclTrack(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpDpuHcclTrack",
                   [this, grouper]()
                   {
                       const auto &records = grouper->GetDumpWarehouse().dpuHcclInfoData;
                       if (records.empty())
                       {
                           return;
                       }
                       TimeLogger t{"Dump dpu hccl track data start"};
                       DpuHcclTrackDumper dumper(hostPath_);
                       if (!dumper.DumpData(records))
                       {
                           succeeded_ = false;
                           ERROR("Dump dpu hccl track data failed");
                       }
                   });
}

void HostTraceWorker::DumpGeLogicStream(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpGeLogicStream",
                   [this, grouper]()
                   {
                       const auto &records = grouper->GetDumpWarehouse().logicStreamInfoData;
                       if (records.empty())
                       {
                           return;
                       }
                       TimeLogger t{"Dump ge logic stream data start"};
                       GeLogicStreamRows rows;
                       if (!FormatLogicStreamInfo(records, rows))
                       {
                           succeeded_ = false;
                           ERROR("Format ge logic stream data failed");
                           return;
                       }
                       if (rows.empty())
                       {
                           return;
                       }
                       GeLogicStreamDumper dumper(hostPath_);
                       if (!dumper.DumpData(rows))
                       {
                           succeeded_ = false;
                           ERROR("Dump ge logic stream data failed");
                       }
                   });
}

void HostTraceWorker::DumpStreamExpandSpec(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpStreamExpandSpec",
                   [this, grouper]()
                   {
                       TimeLogger t{"Dump stream expand spec data start"};
                       StreamExpandSpecDBDumper dumper(hostPath_);
                       if (!dumper.DumpData(grouper->GetDumpWarehouse().streamExpandSpecData))
                       {
                           succeeded_ = false;
                           ERROR("Dump stream expand spec data failed");
                       }
                   });
}

void HostTraceWorker::DumpStaticOpMem(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpStaticOpMem",
                   [this, grouper]()
                   {
                       TimeLogger t{"Dump static op memory data start"};
                       StaticOpMemDBDumper dumper(hostPath_);
                       if (!dumper.DumpData(grouper->GetDumpWarehouse().staticOpMemData))
                       {
                           succeeded_ = false;
                           ERROR("Dump static op memory data failed");
                       }
                   });
}

void HostTraceWorker::DumpCcuAddInfo(ThreadPool &pool, const std::shared_ptr<EventGrouper> &grouper)
{
    AddCheckedTask(pool, "DumpCcuAddInfo",
                   [this, grouper]()
                   {
                       TimeLogger t{"Dump CCU add-info data start"};
                       CcuAddInfoDBDumper dumper(hostPath_);
                       for (const auto &data : grouper->GetDumpWarehouse().ccuInfoData)
                       {
                           if (!data || !dumper.DumpData(*data))
                           {
                               succeeded_ = false;
                               ERROR("Dump CCU add-info data failed");
                               return;
                           }
                       }
                   });
}

void HostTraceWorker::DumpMemcpyInfo(const std::shared_ptr<EventGrouper> &grouper)
{
    TimeLogger t{"Dump memcpy info data start"};
    std::shared_ptr<MemcpyInfoDumper> memcpyInfoDumper;
    memcpyInfoDumper.reset(new (std::nothrow) MemcpyInfoDumper(hostPath_));
    if (!memcpyInfoDumper)
    {
        succeeded_ = false;
        return;
    }
    auto ret = memcpyInfoDumper->DumpData(grouper->GetDumpWarehouse().memcpyInfoData);
    if (!ret)
    {
        succeeded_ = false;
        ERROR("Dump memcpy info data failed");
    }
}
}  // namespace Domain
}  // namespace Analysis
