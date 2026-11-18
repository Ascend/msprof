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

#include <cerrno>
#include <fstream>
#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"
#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "securec.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

// 这里必须是真实的子目录，不能是 "./"：HostTraceWorker 解析链路里的 HostCpuFreqParser 会无条件
// 创建 <hostPath>/sqlite（host_cpu_freq_parser.cpp 先建目录、后判断有无数据），而 UT 的工作目录是
// 仓库根（test/cmake/depend.cmake 的 run_llt_test 指定 WORKING_DIRECTORY 为 TOP_DIR），
// 写成 "./" 会把 sqlite 目录落到工作区里。
const std::string TEST_HOST_FILE_PATH = "./host_trace_host";

class HostTraceWorkerUtest : public testing::Test
{
   protected:
    virtual void SetUp()
    {
        if (File::Exist(TEST_HOST_FILE_PATH))
        {
            File::RemoveDir(TEST_HOST_FILE_PATH, 0);
        }
        File::CreateDir(TEST_HOST_FILE_PATH);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        if (File::Exist(TEST_HOST_FILE_PATH))
        {
            File::RemoveDir(TEST_HOST_FILE_PATH, 0);
        }
    }
};

TEST_F(HostTraceWorkerUtest, TestKernelHostTraceWorkerShouldReturnTrueWhenInputDirIsEmpty)
{
    HostTraceWorker hostTraceWorker{TEST_HOST_FILE_PATH};
    auto ret = hostTraceWorker.Run();
    EXPECT_EQ(ret, true);
}

TEST_F(HostTraceWorkerUtest, TestKernelHostTraceWorkerShouldReturnTrueWhenGroupResultIsNotEmpty)
{
    HostTraceWorker hostTraceWorker{TEST_HOST_FILE_PATH};
    CANNWarehouses mockWarehouses;
    mockWarehouses.Insert(0, CANNWarehouse{});
    MOCKER_CPP(&EventGrouper::GetGroupEvents).stubs().will(returnValue(mockWarehouses));
    auto ret = hostTraceWorker.Run();
    EXPECT_EQ(ret, true);
}

TEST_F(HostTraceWorkerUtest, GeZeroPhysicalStreamsDoNotCreateDatabase)
{
    auto grouper = std::make_shared<EventGrouper>(TEST_HOST_FILE_PATH);
    auto info = std::make_shared<ParserAdditionalInfo>();
    *info = {};
    info->data[0] = 7;
    grouper->GetDumpWarehouse().logicStreamInfoData.emplace_back(info);
    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ThreadPool pool(1);
    pool.Start();
    worker.DumpGeLogicStream(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_logic_stream_info.db"})));
}

TEST_F(HostTraceWorkerUtest, GePopulatedRowsDumpWithoutPlatformFiltering)
{
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    auto grouper = std::make_shared<EventGrouper>(TEST_HOST_FILE_PATH);
    auto info = std::make_shared<ParserAdditionalInfo>();
    const uint32_t payload[] = {7, 1, 100};
    *info = {};
    ASSERT_EQ(EOK, memcpy_s(info->data, sizeof(info->data), payload, sizeof(payload)));
    grouper->GetDumpWarehouse().logicStreamInfoData.emplace_back(info);
    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ThreadPool pool(1);
    pool.Start();
    worker.DumpGeLogicStream(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();
    Analysis::Infra::DBRunner runner(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_logic_stream_info.db"}));
    std::vector<std::tuple<uint32_t, uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT physic_stream, logic_stream FROM GeLogicStreamInfo", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ(100U, std::get<0>(rows.front()));
    EXPECT_EQ(7U, std::get<1>(rows.front()));
}

TEST_F(HostTraceWorkerUtest, GeInvalidPhysicalCountDoesNotCreateDatabase)
{
    auto grouper = std::make_shared<EventGrouper>(TEST_HOST_FILE_PATH);
    auto info = std::make_shared<ParserAdditionalInfo>();
    *info = {};
    info->logicStreamInfo.physicStreamNum = GE_LOGIC_STREAM_PHYSIC_SLOT + 1;
    grouper->GetDumpWarehouse().logicStreamInfoData.emplace_back(info);
    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ThreadPool pool(1);
    pool.Start();
    worker.DumpGeLogicStream(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();
    EXPECT_FALSE(File::Exist(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_logic_stream_info.db"})));
}

TEST_F(HostTraceWorkerUtest, RunParsesAndDumpsDpuHcclAndGeLookupRecords)
{
    const auto dataPath = File::PathJoin({TEST_HOST_FILE_PATH, "data"});
    ASSERT_TRUE(File::CreateDir(dataPath));
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    MsprofAdditionalInfo record{};
    record.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    record.level = MSPROF_REPORT_NODE_LEVEL;
    record.threadId = 23;
    record.timeStamp = 456;
    ParserDpuHcclInfo hcclPayload{};
    hcclPayload.npuDeviceId = 2;
    hcclPayload.filed24 = 0xA005;
    hcclPayload.localRank = 3;
    hcclPayload.remoteRank = 7;
    hcclPayload.rankSize = 16;
    hcclPayload.startTime = 123;
    hcclPayload.dataSize = 2048;
    hcclPayload.taskId = 42;
    hcclPayload.streamId = 11;
    record.dataLen = sizeof(hcclPayload);
    ASSERT_EQ(EOK, memcpy_s(record.data, sizeof(record.data), &hcclPayload, sizeof(hcclPayload)));
    {
        std::ofstream file(File::PathJoin({dataPath, "unaging.additional.dpu_hccl_info.slice_0"}),
                           std::ios::out | std::ios::binary);
        file.write(reinterpret_cast<const char *>(&record), sizeof(record));
        ASSERT_TRUE(file.good());
    }
    MsprofAdditionalInfo logicRecord{};
    logicRecord.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    logicRecord.level = MSPROF_REPORT_NODE_LEVEL;
    const uint32_t payload[] = {7, 1, 100};
    logicRecord.dataLen = sizeof(payload);
    ASSERT_EQ(EOK, memcpy_s(logicRecord.data, sizeof(logicRecord.data), payload, sizeof(payload)));
    {
        std::ofstream file(File::PathJoin({dataPath, "unaging.additional.logic_stream_info.slice_0"}),
                           std::ios::out | std::ios::binary);
        file.write(reinterpret_cast<const char *>(&logicRecord), sizeof(logicRecord));
        ASSERT_TRUE(file.good());
    }
    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ASSERT_TRUE(worker.Run());

    Analysis::Infra::DBRunner dpuRunner(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "dpu.db"}));
    std::vector<std::tuple<uint32_t, uint64_t>> dpuRows;
    ASSERT_TRUE(dpuRunner.QueryData("SELECT thread_id, end_time FROM DPUHcclTrack", dpuRows));
    ASSERT_EQ(1U, dpuRows.size());
    EXPECT_EQ(23U, std::get<0>(dpuRows.front()));
    EXPECT_EQ(456U, std::get<1>(dpuRows.front()));

    using PayloadRow = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, uint64_t,
                                  uint32_t, uint32_t, std::string, std::string>;
    std::vector<PayloadRow> payloadRows;
    ASSERT_TRUE(dpuRunner.QueryData(
        "SELECT npu_device_id, dpu_device_id, local_rank, remote_rank, rank_size, start_time, data_size, "
        "task_id, stream_id, op_type, data_type FROM DPUHcclTrack", payloadRows));
    ASSERT_EQ(1U, payloadRows.size());
    const auto &row = payloadRows.front();
    EXPECT_EQ(2U, std::get<0>(row));
    EXPECT_EQ(5U, std::get<1>(row));
    EXPECT_EQ(3U, std::get<2>(row));
    EXPECT_EQ(7U, std::get<3>(row));
    EXPECT_EQ(16U, std::get<4>(row));
    EXPECT_EQ(123U, std::get<5>(row));
    EXPECT_EQ(2048U, std::get<6>(row));
    EXPECT_EQ(42U, std::get<7>(row));
    EXPECT_EQ(11U, std::get<8>(row));
    EXPECT_EQ("SUM", std::get<9>(row));
    EXPECT_EQ("INT8", std::get<10>(row));

    Analysis::Infra::DBRunner geRunner(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "ge_logic_stream_info.db"}));
    std::vector<std::tuple<uint32_t, uint32_t>> geRows;
    ASSERT_TRUE(geRunner.QueryData("SELECT physic_stream, logic_stream FROM GeLogicStreamInfo", geRows));
    ASSERT_EQ(1U, geRows.size());
    EXPECT_EQ(100U, std::get<0>(geRows.front()));
    EXPECT_EQ(7U, std::get<1>(geRows.front()));
}

TEST_F(HostTraceWorkerUtest, DpuTaskTrackTaskAlsoDumpsHcclToSharedDatabase)
{
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    auto grouper = std::make_shared<EventGrouper>(TEST_HOST_FILE_PATH);
    auto task = std::make_shared<ParserCompactInfo>();
    *task = {};
    grouper->GetDumpWarehouse().dpuTrackData.emplace_back(task);
    auto hccl = std::make_shared<ParserAdditionalInfo>();
    *hccl = {};
    grouper->GetDumpWarehouse().dpuHcclInfoData.emplace_back(hccl);

    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ThreadPool pool(2);
    ASSERT_TRUE(pool.Start());
    worker.DumpDpuTaskTrack(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();

    Analysis::Infra::DBRunner runner(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "dpu.db"}));
    std::vector<std::tuple<uint64_t>> taskRows;
    std::vector<std::tuple<uint64_t>> hcclRows;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM DPUTaskTrack", taskRows));
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM DPUHcclTrack", hcclRows));
    ASSERT_EQ(1U, taskRows.size());
    ASSERT_EQ(1U, hcclRows.size());
    EXPECT_EQ(1U, std::get<0>(taskRows.front()));
    EXPECT_EQ(1U, std::get<0>(hcclRows.front()));
}

TEST_F(HostTraceWorkerUtest, DpuTaskTrackFailureStillDumpsHcclToSharedDatabase)
{
    ASSERT_TRUE(File::CreateDir(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite"})));
    auto grouper = std::make_shared<EventGrouper>(TEST_HOST_FILE_PATH);
    auto task = std::make_shared<ParserCompactInfo>();
    *task = {};
    grouper->GetDumpWarehouse().dpuTrackData.emplace_back(task);
    auto hccl = std::make_shared<ParserAdditionalInfo>();
    *hccl = {};
    grouper->GetDumpWarehouse().dpuHcclInfoData.emplace_back(hccl);
    MOCKER_CPP(&DpuTaskTrackDBDumper::GenerateData).expects(once()).will(returnValue(DpuTaskTrackData{}));

    HostTraceWorker worker(TEST_HOST_FILE_PATH);
    ThreadPool pool(2);
    ASSERT_TRUE(pool.Start());
    worker.DumpDpuTaskTrack(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();

    EXPECT_FALSE(worker.succeeded_.load());
    Analysis::Infra::DBRunner runner(File::PathJoin({TEST_HOST_FILE_PATH, "sqlite", "dpu.db"}));
    std::vector<std::tuple<uint64_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM DPUHcclTrack", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ(1U, std::get<0>(rows.front()));
}
