/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "analysis/csrc/application/database/db_assembler.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/application/deliverable_consumer.h"
#include "analysis/csrc/application/summary/api_statistic_assembler.h"
#include "analysis/csrc/application/summary/npu_module_mem_assembler.h"
#include "analysis/csrc/application/summary/page_fault_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/application/timeline/ddr_assembler.h"
#include "analysis/csrc/application/timeline/llc_assembler.h"
#include "analysis/csrc/application/timeline/npu_mem_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/page_fault_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using Analysis::Infra::DBRunner;
using Analysis::Infra::DataInventory;

namespace
{
const int DEPTH = 0;
const uint32_t kRawPid = 100;
const uint16_t kDeviceId = 0;
const std::string BASE_PATH = "./concurrent_assembler_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE_PATH = File::PathJoin({PROF_PATH, "device_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
const std::string DB_DIR = File::PathJoin({BASE_PATH, "db"});
const std::string kDbPrefix = "DB:";

// Timeline samples. Values divide evenly so the exported numbers stay exact.
const uint64_t kNpuTs0 = 1000000000ULL;
const uint64_t kNpuTs1 = 2000000000ULL;
const uint64_t kDdrTs0 = 3000000000ULL;
const uint64_t kDdrTs1 = 4000000000ULL;
const uint64_t kLlcTs0 = 5000000000ULL;
const uint64_t kLlcTs1 = 6000000000ULL;
const uint64_t kModuleTs0 = 1000000ULL;
const uint64_t kModuleTs1 = 2000000ULL;
const uint64_t kAclDurationNs = 2000000ULL;
const uint64_t kRuntimeDurationNs = 3000000ULL;
const uint16_t kHcclModule = 3;
const uint16_t kRuntimeModule = 7;

void NoteFailure(std::mutex &mu, std::vector<std::string> &failures, const std::string &message)
{
    std::lock_guard<std::mutex> lock(mu);
    failures.emplace_back(message);
}

template <typename Fn>
void Launch(std::vector<std::thread> &threads, std::mutex &mu, std::vector<std::string> &failures, const char *name,
            Fn fn)
{
    threads.emplace_back([name, fn, &mu, &failures]()
                         {
                             try
                             {
                                 if (!fn())
                                 {
                                     NoteFailure(mu, failures, name);
                                 }
                             }
                             catch (const std::exception &ex)
                             {
                                 NoteFailure(mu, failures, std::string(name) + ": " + ex.what());
                             }
                             catch (...)
                             {
                                 NoteFailure(mu, failures, std::string(name) + ": unknown");
                             }
                         });
}

template <typename T>
std::shared_ptr<std::vector<T>> Share(const std::vector<T> &rows)
{
    std::shared_ptr<std::vector<T>> data;
    MAKE_SHARED_NO_OPERATION(data, std::vector<T>, rows);
    return data;
}

void PrepareDir(const std::string &dir)
{
    if (File::Exist(dir))
    {
        EXPECT_TRUE(File::RemoveDir(dir, DEPTH));
    }
    EXPECT_TRUE(File::CreateDir(dir));
}

std::vector<std::string> FilesNamed(const std::string &dir, const std::string &prefix)
{
    auto files = File::GetOriginData(dir, {prefix}, {});
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<std::string> ReadLines(const std::string &path)
{
    FileReader reader(path);
    std::vector<std::string> lines;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(lines));
    return lines;
}

std::string JoinText(const std::vector<std::string> &lines)
{
    std::string text;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (i != 0)
        {
            text.append("\n");
        }
        text.append(lines[i]);
    }
    return text;
}

void AddMeta(std::vector<nlohmann::json> &out, uint32_t pid, const LayerInfo &layer)
{
    nlohmann::json name;
    name["name"] = META_DATA_PROCESS_NAME;
    name["pid"] = pid;
    name["tid"] = DEFAULT_TID;
    name["ph"] = "M";
    name["args"]["name"] = layer.component;
    out.emplace_back(name);

    nlohmann::json label;
    label["name"] = META_DATA_PROCESS_LABEL;
    label["pid"] = pid;
    label["tid"] = DEFAULT_TID;
    label["ph"] = "M";
    label["args"]["labels"] = std::string("NPU ") + std::to_string(kDeviceId);
    out.emplace_back(label);

    nlohmann::json index;
    index["name"] = META_DATA_PROCESS_INDEX;
    index["pid"] = pid;
    index["tid"] = DEFAULT_TID;
    index["ph"] = "M";
    index["args"]["sort_index"] = JsonAssembler::GetDisplaySortIndex(layer);
    out.emplace_back(index);
}

void AddCounter(std::vector<nlohmann::json> &out, uint32_t pid, uint64_t timestamp, const std::string &name,
                const std::string &series, double value)
{
    nlohmann::json event;
    event["name"] = name;
    event["pid"] = pid;
    event["tid"] = DEFAULT_TID;
    event["ts"] = DivideByPowersOfTenWithPrecision(timestamp);
    event["ph"] = "C";
    event["args"][series] = value;
    out.emplace_back(event);
}

std::vector<std::string> Canon(std::vector<nlohmann::json> events)
{
    std::vector<std::string> out;
    out.reserve(events.size());
    for (size_t i = 0; i < events.size(); ++i)
    {
        out.emplace_back(events[i].dump());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<NpuMemData> MakeNpuMem()
{
    std::vector<NpuMemData> rows;
    NpuMemData app;
    app.deviceId = kDeviceId;
    app.event = "0";
    app.ddr = 0;
    app.hbm = BYTE_SIZE;
    app.memory = BYTE_SIZE * 2;
    app.timestamp = kNpuTs0;
    rows.emplace_back(app);
    NpuMemData device;
    device.deviceId = kDeviceId;
    device.event = "1";
    device.ddr = BYTE_SIZE * 3;
    device.hbm = BYTE_SIZE * 4;
    device.memory = BYTE_SIZE * 5;
    device.timestamp = kNpuTs1;
    rows.emplace_back(device);
    return rows;
}

std::vector<DDRData> MakeDdr()
{
    std::vector<DDRData> rows;
    DDRData first;
    first.deviceId = kDeviceId;
    first.fluxRead = 12.0;
    first.fluxWrite = 34.0;
    first.timestamp = kDdrTs0;
    rows.emplace_back(first);
    DDRData second;
    second.deviceId = kDeviceId;
    second.fluxRead = 56.0;
    second.fluxWrite = 78.0;
    second.timestamp = kDdrTs1;
    rows.emplace_back(second);
    return rows;
}

std::vector<LLcData> MakeLlc()
{
    std::vector<LLcData> rows;
    LLcData readRow;
    readRow.deviceId = kDeviceId;
    readRow.llcID = 1;
    readRow.mode = "read";
    readRow.hitRate = 5000.0;
    readRow.throughput = 8.0;
    readRow.timestamp = kLlcTs0;
    rows.emplace_back(readRow);
    LLcData writeRow;
    writeRow.deviceId = kDeviceId;
    writeRow.llcID = 2;
    writeRow.mode = "write";
    writeRow.hitRate = 2500.0;
    writeRow.throughput = 16.0;
    writeRow.timestamp = kLlcTs1;
    rows.emplace_back(writeRow);
    return rows;
}

std::vector<PageFaultData> MakePageFault()
{
    std::vector<PageFaultData> rows;
    PageFaultData conv;
    conv.deviceId = kDeviceId;
    conv.streamId = 15;
    conv.taskId = 31;
    conv.pageFaultNum = 3;
    conv.opName = "Conv2D";
    rows.emplace_back(conv);
    PageFaultData add;
    add.deviceId = kDeviceId;
    add.streamId = 17;
    add.taskId = 63;
    add.pageFaultNum = 1;
    add.opName = "Add";
    rows.emplace_back(add);
    return rows;
}

std::vector<ApiData> MakeApi()
{
    std::vector<ApiData> rows;
    ApiData acl;
    acl.level = MSPROF_REPORT_ACL_LEVEL;
    acl.apiName = "AclInit";
    acl.timestamp = 0;
    acl.end = kAclDurationNs;
    rows.emplace_back(acl);
    ApiData runtime;
    runtime.level = MSPROF_REPORT_RUNTIME_LEVEL;
    runtime.apiName = "rtMemcpy";
    runtime.timestamp = 0;
    runtime.end = kRuntimeDurationNs;
    rows.emplace_back(runtime);
    return rows;
}

std::vector<NpuModuleMemData> MakeModuleMem()
{
    // Insert RUNTIME before HCCL. The CSV assembler sorts; the DB saver keeps the source rows.
    std::vector<NpuModuleMemData> rows;
    NpuModuleMemData runtime;
    runtime.deviceId = kDeviceId;
    runtime.moduleId = kRuntimeModule;
    runtime.totalReserved = BYTE_SIZE * 4;
    runtime.deviceType = "npu";
    runtime.timestamp = kModuleTs1;
    rows.emplace_back(runtime);
    NpuModuleMemData hccl;
    hccl.deviceId = kDeviceId;
    hccl.moduleId = kHcclModule;
    hccl.totalReserved = BYTE_SIZE * 2;
    hccl.deviceType = "npu";
    hccl.timestamp = kModuleTs0;
    rows.emplace_back(hccl);
    return rows;
}

std::vector<nlohmann::json> ExpectedJson()
{
    const LayerInfo npuLayer = GetLayerInfo(PROCESS_NPU_MEM);
    const LayerInfo ddrLayer = GetLayerInfo(PROCESS_DDR);
    const LayerInfo llcLayer = GetLayerInfo(PROCESS_LLC);
    const uint32_t npuPid = JsonAssembler::GetFormatPid(kRawPid, npuLayer.sortIndex, kDeviceId);
    const uint32_t ddrPid = JsonAssembler::GetFormatPid(kRawPid, ddrLayer.sortIndex, kDeviceId);
    const uint32_t llcPid = JsonAssembler::GetFormatPid(kRawPid, llcLayer.sortIndex, kDeviceId);
    std::vector<nlohmann::json> events;
    AddMeta(events, npuPid, npuLayer);
    AddCounter(events, npuPid, kNpuTs0, "APP/DDR", "KB", 0.0);
    AddCounter(events, npuPid, kNpuTs0, "APP/HBM", "KB", 1.0);
    AddCounter(events, npuPid, kNpuTs0, "APP/Memory", "KB", 2.0);
    AddCounter(events, npuPid, kNpuTs1, "Device/DDR", "KB", 3.0);
    AddCounter(events, npuPid, kNpuTs1, "Device/HBM", "KB", 4.0);
    AddCounter(events, npuPid, kNpuTs1, "Device/Memory", "KB", 5.0);
    AddMeta(events, ddrPid, ddrLayer);
    AddCounter(events, ddrPid, kDdrTs0, "DDR/Read", "Read(MB/s)", 12.0);
    AddCounter(events, ddrPid, kDdrTs0, "DDR/Write", "Write(MB/s)", 34.0);
    AddCounter(events, ddrPid, kDdrTs1, "DDR/Read", "Read(MB/s)", 56.0);
    AddCounter(events, ddrPid, kDdrTs1, "DDR/Write", "Write(MB/s)", 78.0);
    AddMeta(events, llcPid, llcLayer);
    AddCounter(events, llcPid, kLlcTs0, "LLC 1 Read/Hit Rate", "Hit Rate(%)", 50.0);
    AddCounter(events, llcPid, kLlcTs0, "LLC 1 Read/Throughput", "Throughput(MB/s)", 8.0);
    AddCounter(events, llcPid, kLlcTs1, "LLC 2 Write/Hit Rate", "Hit Rate(%)", 25.0);
    AddCounter(events, llcPid, kLlcTs1, "LLC 2 Write/Throughput", "Throughput(MB/s)", 16.0);
    return events;
}

bool RunDbSaver(const std::shared_ptr<DBAssembler> &session, DataInventory &inventory, const std::string &processor)
{
    TopoBuildContext context;
    context.dbSession = session;
    context.profPath = PROF_PATH;
    const TopoNodeCreatorFactory factory = DBAssembler::CreateSaver(kDbPrefix + processor);
    const Analysis::Infra::ProcessCreator creator = factory(context);
    if (creator == nullptr)
    {
        return false;
    }
    const std::unique_ptr<Analysis::Infra::Process> process = creator();
    if (process == nullptr)
    {
        return false;
    }
    return process->Run(inventory, Analysis::Infra::Context()) == Analysis::ANALYSIS_OK;
}
}  // namespace

class ConcurrentAssemblerUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        DeliverableConsumer::Instance().FinishTimeline();
        DeliverableConsumer::Instance().CloseAllDb();
        DeliverableConsumer::Instance().ResetOk();
        PrepareDir(BASE_PATH);
        PrepareDir(PROF_PATH);
        PrepareDir(DEVICE_PATH);
        PrepareDir(RESULT_PATH);
        PrepareDir(DB_DIR);
        nlohmann::json info;
        info["pid"] = std::string("100");
        info["profLevel"] = std::string("l1");
        Analysis::Domain::Environment::Context::GetInstance().context_[PROF_PATH][kDeviceId] = info;
    }

    void TearDown() override
    {
        Analysis::Domain::Environment::Context::GetInstance().context_.erase(PROF_PATH);
        DeliverableConsumer::Instance().FinishTimeline();
        DeliverableConsumer::Instance().CloseAllDb();
        DeliverableConsumer::Instance().ResetOk();
        if (File::Exist(BASE_PATH))
        {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
    }
};

TEST_F(ConcurrentAssemblerUTest, ShouldKeepDeliverableContentWhenAssemblersRunTogether)
{
    DataInventory inventory;
    auto npuData = Share(MakeNpuMem());
    auto ddrData = Share(MakeDdr());
    auto llcData = Share(MakeLlc());
    auto pageFaultData = Share(MakePageFault());
    auto apiData = Share(MakeApi());
    auto moduleData = Share(MakeModuleMem());
    ASSERT_NE(npuData, nullptr);
    ASSERT_NE(ddrData, nullptr);
    ASSERT_NE(llcData, nullptr);
    ASSERT_NE(pageFaultData, nullptr);
    ASSERT_NE(apiData, nullptr);
    ASSERT_NE(moduleData, nullptr);
    inventory.Inject(npuData);
    inventory.Inject(ddrData);
    inventory.Inject(llcData);
    inventory.Inject(pageFaultData);
    inventory.Inject(apiData);
    inventory.Inject(moduleData);

    auto session = std::make_shared<DBAssembler>(PROF_PATH, DB_DIR);
    ASSERT_NE(session->msprofDB_.dbRunner, nullptr);
    const std::string dbPath = session->msprofDB_.dbRunner->GetPath();

    std::mutex failureMu;
    std::vector<std::string> failures;
    std::vector<std::thread> threads;
    Launch(threads, failureMu, failures, "npu_mem", [&inventory]()
           {
               NpuMemAssembler assembler;
               return assembler.Run(inventory, PROF_PATH);
           });
    Launch(threads, failureMu, failures, "ddr", [&inventory]()
           {
               DDRAssembler assembler;
               return assembler.Run(inventory, PROF_PATH);
           });
    Launch(threads, failureMu, failures, "llc", [&inventory]()
           {
               LLcAssembler assembler;
               return assembler.Run(inventory, PROF_PATH);
           });
    Launch(threads, failureMu, failures, "page_fault", [&inventory]()
           {
               PageFaultAssembler assembler(PROCESSOR_NAME_PAGE_FAULT, PROF_PATH);
               return assembler.Run(inventory);
           });
    Launch(threads, failureMu, failures, "api_statistic", [&inventory]()
           {
               ApiStatisticAssembler assembler(API_STATISTIC_NAME, PROF_PATH);
               return assembler.Run(inventory);
           });
    Launch(threads, failureMu, failures, "npu_module_mem", [&inventory]()
           {
               NpuModuleMemAssembler assembler(PROCESSOR_NAME_NPU_MODULE_MEM, PROF_PATH);
               return assembler.Run(inventory);
           });
    Launch(threads, failureMu, failures, "db_ddr", [session, &inventory]()
           { return RunDbSaver(session, inventory, PROCESSOR_NAME_DDR); });
    Launch(threads, failureMu, failures, "db_npu_module_mem", [session, &inventory]()
           { return RunDbSaver(session, inventory, PROCESSOR_NAME_NPU_MODULE_MEM); });
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    // The closing bracket is written when the timeline finishes, after every assembler has returned.
    DeliverableConsumer::Instance().FinishTimeline();
    session.reset();

    std::string failureText;
    for (size_t i = 0; i < failures.size(); ++i)
    {
        if (i != 0)
        {
            failureText.append(",");
        }
        failureText.append(failures[i]);
    }
    EXPECT_TRUE(failures.empty()) << failureText;
    EXPECT_TRUE(DeliverableConsumer::Instance().Ok());

    const auto jsonFiles = FilesNamed(RESULT_PATH, MSPROF_JSON_FILE);
    ASSERT_EQ(1ul, jsonFiles.size());
    nlohmann::json parsed;
    const std::string jsonText = JoinText(ReadLines(jsonFiles.front()));
    try
    {
        parsed = nlohmann::json::parse(jsonText);
    }
    catch (const std::exception &ex)
    {
        FAIL() << ex.what();
    }
    ASSERT_TRUE(parsed.is_array());
    std::vector<nlohmann::json> actualEvents;
    for (size_t i = 0; i < parsed.size(); ++i)
    {
        actualEvents.emplace_back(parsed[i]);
    }
    // Assemblers share one msprof file. Chunk order follows whichever writer acquires the stream first.
    EXPECT_EQ(Canon(ExpectedJson()), Canon(actualEvents));

    const auto pageFaultFiles = FilesNamed(RESULT_PATH, PAGE_FAULT_NAME);
    ASSERT_EQ(1ul, pageFaultFiles.size());
    EXPECT_EQ((std::vector<std::string>{"Device Id,Stream Id,Task Id,Page Fault Num,Op Name", "0,15,31,3,Conv2D",
                                        "0,17,63,1,Add"}),
              ReadLines(pageFaultFiles.front()));

    const auto apiFiles = FilesNamed(RESULT_PATH, API_STATISTIC_NAME);
    ASSERT_EQ(1ul, apiFiles.size());
    EXPECT_EQ((std::vector<std::string>{"Device_id,Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance",
                                        "host,acl,AclInit,2000,1,2000,2000,2000,0",
                                        "host,runtime,rtMemcpy,3000,1,3000,3000,3000,0"}),
              ReadLines(apiFiles.front()));

    const std::string moduleTs0 = DivideByPowersOfTenWithPrecision(kModuleTs0, true);
    const std::string moduleTs1 = DivideByPowersOfTenWithPrecision(kModuleTs1, true);
    const auto moduleFiles = FilesNamed(RESULT_PATH, NPU_MODULE_MEMORY_NAME);
    ASSERT_EQ(1ul, moduleFiles.size());
    EXPECT_EQ((std::vector<std::string>{"Device_id,Component,Timestamp(us),Total Reserved(KB),Device",
                                        std::string("0,HCCL,") + moduleTs0 + ",2,npu",
                                        std::string("0,RUNTIME,") + moduleTs1 + ",4,npu"}),
              ReadLines(moduleFiles.front()));

    const auto dbFiles = FilesNamed(DB_DIR, DB_NAME_MSPROF_DB);
    ASSERT_EQ(1ul, dbFiles.size());
    EXPECT_EQ(dbPath, dbFiles.front());
    DBRunner runner(dbPath);
    using DdrRow = std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>;
    std::vector<DdrRow> ddrRows;
    ASSERT_TRUE(runner.QueryData("SELECT deviceId, timestampNs, read, write FROM " + TABLE_NAME_DDR, ddrRows));
    std::vector<DdrRow> expectDdr;
    expectDdr.emplace_back(kDeviceId, kDdrTs0, 12ULL, 34ULL);
    expectDdr.emplace_back(kDeviceId, kDdrTs1, 56ULL, 78ULL);
    std::sort(ddrRows.begin(), ddrRows.end());
    std::sort(expectDdr.begin(), expectDdr.end());
    EXPECT_EQ(expectDdr, ddrRows);

    using ModuleRow = std::tuple<uint32_t, uint64_t, uint64_t, uint16_t>;
    std::vector<ModuleRow> moduleRows;
    ASSERT_TRUE(runner.QueryData(
        "SELECT moduleId, timestampNs, totalReserved, deviceId FROM " + TABLE_NAME_NPU_MODULE_MEM, moduleRows));
    std::vector<ModuleRow> expectModule;
    expectModule.emplace_back(kRuntimeModule, kModuleTs1, static_cast<uint64_t>(BYTE_SIZE) * 4, kDeviceId);
    expectModule.emplace_back(kHcclModule, kModuleTs0, static_cast<uint64_t>(BYTE_SIZE) * 2, kDeviceId);
    std::sort(moduleRows.begin(), moduleRows.end());
    std::sort(expectModule.begin(), expectModule.end());
    EXPECT_EQ(expectModule, moduleRows);
}
