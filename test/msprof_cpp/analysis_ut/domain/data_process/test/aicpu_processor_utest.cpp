/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/ai_task/aicpu_processor.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/aicpu_summary_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "opensource/json/include/nlohmann/json.hpp"

using namespace Analysis::Application;
using namespace Analysis::Common;
using namespace Analysis::Domain;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./aicpu_processor_utest";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});

const TableColumns AICPU_TABLE_COLS = {
    {"stream_id", SQL_INTEGER_TYPE},  {"task_id", SQL_INTEGER_TYPE},    {"sys_start", SQL_NUMERIC_TYPE},
    {"sys_end", SQL_NUMERIC_TYPE},    {"node_name", SQL_TEXT_TYPE},     {"compute_time", SQL_REAL_TYPE},
    {"memcpy_time", SQL_REAL_TYPE},   {"task_time", SQL_REAL_TYPE},     {"dispatch_time", SQL_REAL_TYPE},
    {"total_time", SQL_REAL_TYPE},
};

const TableColumns AICPU_DP_TABLE_COLS = {
    {"timestamp", SQL_NUMERIC_TYPE},
    {"action", SQL_TEXT_TYPE},
    {"source", SQL_TEXT_TYPE},
    {"buffer_size", SQL_INTEGER_TYPE},
};

const TableColumns DATA_QUEUE_TABLE_COLS = {
    {"node_name", SQL_TEXT_TYPE},  {"queue_size", SQL_INTEGER_TYPE}, {"start_time", SQL_REAL_TYPE},
    {"end_time", SQL_REAL_TYPE},   {"duration", SQL_REAL_TYPE},
};

using AiCpuInsert = OriAiCpuData;
using DpInsert = OriAiCpuDpData;
using MiInsert = std::vector<std::tuple<std::string, uint64_t, double, double, double>>;

nlohmann::json BuildTimeRecord(const std::string &startUs, const std::string &baseNs,
                               const std::string &platformVersion)
{
    return nlohmann::json{
        {"startCollectionTimeBegin", startUs},
        {"endCollectionTimeEnd", "999999999"},
        {"startClockMonotonicRaw", baseNs},
        {"platform_version", platformVersion},
    };
}

std::string DevicePath(uint16_t deviceId)
{
    return File::PathJoin({PROF_PATH, DEVICE_PREFIX + std::to_string(deviceId)});
}

std::string SqlitePath(uint16_t deviceId)
{
    return File::PathJoin({DevicePath(deviceId), SQLITE});
}

void CreateDeviceDir(uint16_t deviceId)
{
    EXPECT_TRUE(File::CreateDir(DevicePath(deviceId)));
    EXPECT_TRUE(File::CreateDir(SqlitePath(deviceId)));
}

void InsertTable(const std::string &dbPath, const std::string &tableName, const TableColumns &cols)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
    EXPECT_TRUE(dbRunner->CreateTable(tableName, cols));
}

template <typename T>
void InsertTableData(const std::string &dbPath, const std::string &tableName, const TableColumns &cols, const T &data)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
    EXPECT_TRUE(dbRunner->CreateTable(tableName, cols));
    if (!data.empty()) {
        EXPECT_TRUE(dbRunner->InsertData(tableName, data));
    }
}

void WriteAiCpuData(uint16_t deviceId, const AiCpuInsert &data)
{
    InsertTableData(File::PathJoin({SqlitePath(deviceId), DB_NAME_AI_CPU}), TABLE_NAME_AI_CPU, AICPU_TABLE_COLS, data);
}

void WriteDpData(uint16_t deviceId, const DpInsert &data)
{
    InsertTableData(File::PathJoin({SqlitePath(deviceId), DB_NAME_AI_CPU}), TABLE_NAME_AI_CPU_DP, AICPU_DP_TABLE_COLS,
                    data);
}

void WriteMiData(uint16_t deviceId, const MiInsert &data)
{
    InsertTableData(File::PathJoin({SqlitePath(deviceId), DB_NAME_DATA_PREPROCESS}), TABLE_NAME_DATA_QUEUE,
                    DATA_QUEUE_TABLE_COLS, data);
}

AscendTaskData MakeAscendTask(uint16_t deviceId, uint32_t streamId, uint32_t taskId, uint64_t startNs, uint64_t endNs,
                              uint32_t batchId, const std::string &hostType)
{
    AscendTaskData task;
    task.deviceId = deviceId;
    task.streamId = streamId;
    task.taskId = taskId;
    task.timestamp = startNs;
    task.end = endNs;
    task.batchId = batchId;
    task.hostType = hostType;
    return task;
}

TaskInfoData MakeTaskInfo(uint16_t deviceId, uint32_t streamId, uint32_t taskId, uint32_t batchId,
                          const std::string &taskType, const std::string &opName)
{
    TaskInfoData info;
    info.deviceId = deviceId;
    info.streamId = streamId;
    info.taskId = taskId;
    info.batchId = batchId;
    info.taskType = taskType;
    info.opName = opName;
    return info;
}

template <typename T>
void InjectVector(DataInventory &dataInventory, const std::vector<T> &data)
{
    std::shared_ptr<std::vector<T>> holder;
    MAKE_SHARED_NO_OPERATION(holder, std::vector<T>, data);
    dataInventory.Inject(holder);
}

const AicpuSummaryData *FindSummary(const std::vector<AicpuSummaryData> &data, uint16_t deviceId, uint32_t streamId,
                                    uint32_t taskId, uint64_t timestampNs)
{
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].deviceId == deviceId && data[i].streamId == streamId && data[i].taskId == taskId &&
            data[i].timestampNs == timestampNs) {
            return &data[i];
        }
    }
    return nullptr;
}
}  // namespace

class AicpuProcessorUTest : public testing::Test {
protected:
    void SetUp() override
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        MOCKER_CPP(&Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(BuildTimeRecord("0", "0", "5")));
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        if (File::Check(BASE_PATH)) {
            EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        }
    }
};

TEST_F(AicpuProcessorUTest, ShouldLoadAllThreeTypesAndFillDerivedFields)
{
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {10, 20, 1000000.0, 1500000.0, "", 1500.0, 2500.0, 500000.0, 500.0, 10500.0},
        {11, 30, 4000000.0, 3500000.0, "InvEnd", 3000.0, 4000.0, 8000.0, 1000.0, 20000.0},
    };
    DpInsert dpData{
        {1000000.0, "enqueue", "src0", 128},
        {2000000.0, "dequeue", "src1", 256},
    };
    MiInsert miData{
        {"QueueA", 8, 100.7, 200.2, 99.5},
        {"QueueB", 16, 300.0, 400.0, 100.0},
    };
    WriteAiCpuData(0, aicpuData);
    WriteDpData(0, dpData);
    WriteMiData(0, miData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(2ul, summary->size());
    EXPECT_EQ(0u, summary->at(0).deviceId);
    EXPECT_EQ(10u, summary->at(0).streamId);
    EXPECT_EQ(20u, summary->at(0).taskId);
    EXPECT_EQ(1000000ull, summary->at(0).timestampNs);
    EXPECT_EQ(1500000ull, summary->at(0).endNs);
    EXPECT_EQ(NA, summary->at(0).nodeName);
    EXPECT_DOUBLE_EQ(1.5, summary->at(0).computeTimeUs);
    EXPECT_DOUBLE_EQ(2.5, summary->at(0).memcpyTimeUs);
    EXPECT_DOUBLE_EQ(500.0, summary->at(0).taskTimeUs);
    EXPECT_DOUBLE_EQ(0.5, summary->at(0).dispatchTimeUs);
    EXPECT_DOUBLE_EQ(10.5, summary->at(0).totalTimeUs);
    EXPECT_EQ(UINT32_MAX, summary->at(0).batchId);

    EXPECT_EQ("InvEnd", summary->at(1).nodeName);
    EXPECT_DOUBLE_EQ(3.0, summary->at(1).computeTimeUs);
    EXPECT_DOUBLE_EQ(4.0, summary->at(1).memcpyTimeUs);
    EXPECT_DOUBLE_EQ(8.0, summary->at(1).taskTimeUs);
    EXPECT_DOUBLE_EQ(1.0, summary->at(1).dispatchTimeUs);
    EXPECT_DOUBLE_EQ(20.0, summary->at(1).totalTimeUs);

    auto dp = dataInventory.GetPtr<std::vector<AicpuDpData>>();
    ASSERT_NE(nullptr, dp);
    ASSERT_EQ(2ul, dp->size());
    EXPECT_EQ(1000000ull, dp->at(0).timestamp);
    EXPECT_EQ("enqueue", dp->at(0).action);
    EXPECT_EQ("src0", dp->at(0).source);
    EXPECT_EQ(128ull, dp->at(0).bufferSize);

    auto mi = dataInventory.GetPtr<std::vector<AicpuMiData>>();
    ASSERT_NE(nullptr, mi);
    ASSERT_EQ(2ul, mi->size());
    EXPECT_EQ("QueueA", mi->at(0).nodeName);
    EXPECT_EQ(100ull, mi->at(0).startTime);
    EXPECT_EQ(200ull, mi->at(0).endTime);
    EXPECT_EQ(8ull, mi->at(0).queueSize);
}

TEST_F(AicpuProcessorUTest, ShouldSkipRecordsBeforeCollectStart)
{
    MOCKER_CPP(&Environment::Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(BuildTimeRecord("2", "2000", "5")));
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {1, 1, 1000.0, 1500.0, "BeforeStart", 1.0, 1.0, 500.0, 1.0, 3.0},
        {1, 2, 5000.0, 6000.0, "AfterStart", 2.0, 2.0, 1000.0, 2.0, 6.0},
    };
    WriteAiCpuData(0, aicpuData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1ul, summary->size());
    EXPECT_EQ("AfterStart", summary->at(0).nodeName);
    EXPECT_EQ(5000ull, summary->at(0).timestampNs);
}

TEST_F(AicpuProcessorUTest, ShouldFillBatchAndNodeWhenNonV6Matched)
{
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {10, 20, 1000000.0, 1500000.0, "", 1.0, 1.0, 0.0, 1.0, 3.0},
        {10, 20, 2000000.0, 2500000.0, "Raw", 1.0, 1.0, 0.0, 1.0, 3.0},
        {11, 30, 3000000.0, 3100000.0, "KeepMe", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    WriteAiCpuData(0, aicpuData);

    std::vector<AscendTaskData> tasks;
    tasks.push_back(MakeAscendTask(0, 10, 20, 900000, 1600000, 7, KERNEL_AICPU_TASK_TYPE));
    tasks.push_back(MakeAscendTask(0, 10, 20, 1900000, 2600000, 8, KERNEL_AICPU_TASK_TYPE));
    tasks.push_back(MakeAscendTask(0, 10, 20, 500000, 4000000, 99, KERNEL_AICORE_TASK_TYPE));
    tasks.push_back(MakeAscendTask(0, 11, 30, 2900000, 3200000, 1, KERNEL_AICPU_TASK_TYPE));

    std::vector<TaskInfoData> geInfos;
    geInfos.push_back(MakeTaskInfo(0, 10, 20, 7, AI_CPU, "Conv2D"));
    geInfos.push_back(MakeTaskInfo(0, 10, 20, 8, AI_CPU, "MatMul"));
    geInfos.push_back(MakeTaskInfo(0, 11, 30, 1, "AI_CORE", "ShouldNotMatch"));

    DataInventory dataInventory;
    InjectVector(dataInventory, tasks);
    InjectVector(dataInventory, geInfos);

    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(3ul, summary->size());

    const AicpuSummaryData *first = FindSummary(*summary, 0, 10, 20, 1000000ull);
    const AicpuSummaryData *second = FindSummary(*summary, 0, 10, 20, 2000000ull);
    const AicpuSummaryData *third = FindSummary(*summary, 0, 11, 30, 3000000ull);
    ASSERT_NE(nullptr, first);
    ASSERT_NE(nullptr, second);
    ASSERT_NE(nullptr, third);
    EXPECT_EQ(7u, first->batchId);
    EXPECT_EQ("Conv2D", first->nodeName);
    EXPECT_EQ(8u, second->batchId);
    EXPECT_EQ("MatMul", second->nodeName);
    EXPECT_EQ(1u, third->batchId);
    EXPECT_EQ("KeepMe", third->nodeName);
}

TEST_F(AicpuProcessorUTest, ShouldSkipUnmatchedAicpuWithoutConsumingLaterTask)
{
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {10, 20, 1000000.0, 1100000.0, "Gap", 1.0, 1.0, 0.0, 1.0, 3.0},
        {10, 20, 2000000.0, 2300000.0, "Hit", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    WriteAiCpuData(0, aicpuData);

    std::vector<AscendTaskData> tasks;
    tasks.push_back(MakeAscendTask(0, 10, 20, 2000000, 2500000, 8, KERNEL_AICPU_TASK_TYPE));
    std::vector<TaskInfoData> geInfos;
    geInfos.push_back(MakeTaskInfo(0, 10, 20, 8, AI_CPU, "MatchedOp"));

    DataInventory dataInventory;
    InjectVector(dataInventory, tasks);
    InjectVector(dataInventory, geInfos);

    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(2ul, summary->size());
    EXPECT_EQ(UINT32_MAX, summary->at(0).batchId);
    EXPECT_EQ("Gap", summary->at(0).nodeName);
    EXPECT_EQ(8u, summary->at(1).batchId);
    EXPECT_EQ("MatchedOp", summary->at(1).nodeName);
}

TEST_F(AicpuProcessorUTest, ShouldMatchNodeByStreamTaskAndSkipBatchOnV6)
{
    MOCKER_CPP(&Environment::Context::GetPlatformVersion).stubs().will(returnValue(static_cast<uint16_t>(Chip::CHIP_V6_1_0)));
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {10, 20, 1000000.0, 1500000.0, "Raw", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    WriteAiCpuData(0, aicpuData);

    std::vector<AscendTaskData> tasks;
    tasks.push_back(MakeAscendTask(0, 10, 20, 900000, 1600000, 7, KERNEL_AICPU_TASK_TYPE));
    std::vector<TaskInfoData> geInfos;
    geInfos.push_back(MakeTaskInfo(0, 10, 20, 99, "AI_CORE", "V6Op"));

    DataInventory dataInventory;
    InjectVector(dataInventory, tasks);
    InjectVector(dataInventory, geInfos);

    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1ul, summary->size());
    EXPECT_EQ(UINT32_MAX, summary->at(0).batchId);
    EXPECT_EQ("V6Op", summary->at(0).nodeName);
}

TEST_F(AicpuProcessorUTest, ShouldNotCrossMatchDevicesAndShouldSortByTimestamp)
{
    CreateDeviceDir(0);
    CreateDeviceDir(1);
    AiCpuInsert device0{
        {10, 20, 3000000.0, 3500000.0, "Dev0", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    AiCpuInsert device1{
        {10, 20, 1000000.0, 1500000.0, "Dev1", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    WriteAiCpuData(0, device0);
    WriteAiCpuData(1, device1);

    std::vector<AscendTaskData> tasks;
    tasks.push_back(MakeAscendTask(0, 10, 20, 2900000, 3600000, 7, KERNEL_AICPU_TASK_TYPE));
    tasks.push_back(MakeAscendTask(1, 10, 20, 900000, 1600000, 8, KERNEL_AICPU_TASK_TYPE));
    std::vector<TaskInfoData> geInfos;
    geInfos.push_back(MakeTaskInfo(0, 10, 20, 7, AI_CPU, "Op0"));
    geInfos.push_back(MakeTaskInfo(1, 10, 20, 8, AI_CPU, "Op1"));

    DataInventory dataInventory;
    InjectVector(dataInventory, tasks);
    InjectVector(dataInventory, geInfos);

    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(2ul, summary->size());
    EXPECT_EQ(1u, summary->at(0).deviceId);
    EXPECT_EQ(1000000ull, summary->at(0).timestampNs);
    EXPECT_EQ(8u, summary->at(0).batchId);
    EXPECT_EQ("Op1", summary->at(0).nodeName);
    EXPECT_EQ(0u, summary->at(1).deviceId);
    EXPECT_EQ(3000000ull, summary->at(1).timestampNs);
    EXPECT_EQ(7u, summary->at(1).batchId);
    EXPECT_EQ("Op0", summary->at(1).nodeName);
}

TEST_F(AicpuProcessorUTest, ShouldApplyWallClockOffsetToDpAndAicpuTimestamp)
{
    MOCKER_CPP(&Environment::Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(BuildTimeRecord("100", "1000", "5")));
    CreateDeviceDir(0);
    const double rawNs = 1000000.0;
    const uint64_t expectedLocalNs = static_cast<uint64_t>(rawNs) + 100UL * MILLI_SECOND - 1000UL;
    AiCpuInsert aicpuData{
        {10, 20, rawNs, 1500000.0, "OffsetNode", 1500.0, 2500.0, 500000.0, 500.0, 10500.0},
    };
    DpInsert dpData{
        {rawNs, "enqueue", "src0", 128},
    };
    WriteAiCpuData(0, aicpuData);
    WriteDpData(0, dpData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(1ul, summary->size());
    EXPECT_EQ(expectedLocalNs, summary->at(0).timestampNs);

    auto dp = dataInventory.GetPtr<std::vector<AicpuDpData>>();
    ASSERT_NE(nullptr, dp);
    ASSERT_EQ(1ul, dp->size());
    EXPECT_EQ(expectedLocalNs, dp->at(0).timestamp);
}

TEST_F(AicpuProcessorUTest, ShouldKeepEqualTimestampOrderAfterStableSort)
{
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {1, 1, 1000000.0, 1100000.0, "First", 1000.0, 1000.0, 100000.0, 1000.0, 3000.0},
        {2, 2, 1000000.0, 1200000.0, "Second", 1000.0, 1000.0, 200000.0, 1000.0, 3000.0},
    };
    WriteAiCpuData(0, aicpuData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));

    auto summary = dataInventory.GetPtr<std::vector<AicpuSummaryData>>();
    ASSERT_NE(nullptr, summary);
    ASSERT_EQ(2ul, summary->size());
    EXPECT_EQ("First", summary->at(0).nodeName);
    EXPECT_EQ("Second", summary->at(1).nodeName);
    EXPECT_EQ(summary->at(0).timestampNs, summary->at(1).timestampNs);
}

TEST_F(AicpuProcessorUTest, ShouldSucceedWhenAicpuOnly)
{
    CreateDeviceDir(0);
    AiCpuInsert aicpuData{
        {1, 2, 1000000.0, 1100000.0, "OnlyAicpu", 1.0, 1.0, 0.0, 1.0, 3.0},
    };
    WriteAiCpuData(0, aicpuData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
    ASSERT_NE(nullptr, dataInventory.GetPtr<std::vector<AicpuSummaryData>>());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuDpData>>());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuMiData>>());
}

TEST_F(AicpuProcessorUTest, ShouldSucceedWhenDpOnly)
{
    CreateDeviceDir(0);
    DpInsert dpData{
        {1000000.0, "enqueue", "src", 64},
    };
    WriteDpData(0, dpData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuSummaryData>>());
    ASSERT_NE(nullptr, dataInventory.GetPtr<std::vector<AicpuDpData>>());
    EXPECT_EQ(1ul, dataInventory.GetPtr<std::vector<AicpuDpData>>()->size());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuMiData>>());
}

TEST_F(AicpuProcessorUTest, ShouldSucceedWhenMiOnly)
{
    CreateDeviceDir(0);
    MiInsert miData{
        {"OnlyMi", 4, 11.0, 22.0, 11.0},
    };
    WriteMiData(0, miData);

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuSummaryData>>());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuDpData>>());
    ASSERT_NE(nullptr, dataInventory.GetPtr<std::vector<AicpuMiData>>());
    EXPECT_EQ("OnlyMi", dataInventory.GetPtr<std::vector<AicpuMiData>>()->at(0).nodeName);
}

TEST_F(AicpuProcessorUTest, ShouldSucceedWhenTablesMissing)
{
    CreateDeviceDir(0);
    InsertTable(File::PathJoin({SqlitePath(0), DB_NAME_AI_CPU}), "DummyTable",
                TableColumns{{"id", SQL_INTEGER_TYPE}});
    InsertTable(File::PathJoin({SqlitePath(0), DB_NAME_DATA_PREPROCESS}), "DummyTable",
                TableColumns{{"id", SQL_INTEGER_TYPE}});

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuSummaryData>>());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuDpData>>());
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuMiData>>());
}

TEST_F(AicpuProcessorUTest, ShouldSucceedWhenNoDeviceDir)
{
    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<AicpuSummaryData>>());
}

TEST_F(AicpuProcessorUTest, ShouldReturnFalseWhenInvalidDeviceId)
{
    CreateDeviceDir(0);
    WriteAiCpuData(0, AiCpuInsert{{1, 1, 1000000.0, 1100000.0, "x", 1.0, 1.0, 0.0, 1.0, 3.0}});
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(INVALID_DEVICE_ID)));

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
}

TEST_F(AicpuProcessorUTest, ShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    CreateDeviceDir(0);
    MOCKER_CPP(&Environment::Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
}

TEST_F(AicpuProcessorUTest, ShouldReturnFalseWhenConstructDBRunnerFailed)
{
    CreateDeviceDir(0);
    WriteAiCpuData(0, AiCpuInsert{{1, 1, 1000000.0, 1100000.0, "x", 1.0, 1.0, 0.0, 1.0, 3.0}});
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
}

TEST_F(AicpuProcessorUTest, ShouldReturnFalseWhenFileCheckFailed)
{
    CreateDeviceDir(0);
    WriteAiCpuData(0, AiCpuInsert{{1, 1, 1000000.0, 1100000.0, "x", 1.0, 1.0, 0.0, 1.0, 3.0}});
    MOCKER_CPP(&FileReader::Check).stubs().will(returnValue(false));

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
}

TEST_F(AicpuProcessorUTest, ShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    CreateDeviceDir(0);
    WriteAiCpuData(0, AiCpuInsert{{1, 1, 1000000.0, 1100000.0, "x", 1.0, 1.0, 0.0, 1.0, 3.0}});
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<AicpuSummaryData>).stubs().will(returnValue(false));

    DataInventory dataInventory;
    AicpuProcessor processor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_AICPU));
}

TEST_F(AicpuProcessorUTest, ShouldRegisterAicpuProcessor)
{
    const auto *definition = TopoNodeRegistry::FindProcessorByName(PROCESSOR_NAME_AICPU);
    ASSERT_NE(definition, nullptr);
    EXPECT_EQ(definition->runtimeType, std::type_index(typeid(AicpuProcessor)));
    EXPECT_TRUE(static_cast<bool>(definition->creatorFactory));
}
