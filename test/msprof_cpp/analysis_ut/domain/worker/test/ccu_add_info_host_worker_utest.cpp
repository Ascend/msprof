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

#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
constexpr std::size_t RECORD_SIZE = 256;
const std::string HOST_PATH = "./ccu_add_info_host_worker_utest";
const std::string DATA_PATH = "./ccu_add_info_host_worker_utest/data";
const std::string DB_PATH = "./ccu_add_info_host_worker_utest/sqlite/ccu_add_info.db";

template <typename T>
void WriteLittleEndian(std::array<uint8_t, RECORD_SIZE>& data, std::size_t offset, T value)
{
    for (std::size_t index = 0; index < sizeof(T); ++index)
    {
        data[offset + index] = static_cast<uint8_t>((value >> (index * 8)) & 0xff);
    }
}

bool WriteTaskRecord()
{
    std::array<uint8_t, RECORD_SIZE> data{};
    data[24] = 1;
    data[25] = 2;
    WriteLittleEndian<uint64_t>(data, 32, 100);
    WriteLittleEndian<uint64_t>(data, 40, 200);
    WriteLittleEndian<uint32_t>(data, 48, 3);
    WriteLittleEndian<uint32_t>(data, 52, 4);
    WriteLittleEndian<uint16_t>(data, 56, 5);
    WriteLittleEndian<uint32_t>(data, 60, 6);
    data[64] = 7;
    data[65] = 8;
    WriteLittleEndian<uint16_t>(data, 66, 9);

    const std::string path = File::PathJoin({DATA_PATH, "unaging.additional.ccu_task_info.slice_0"});
    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return output.good();
}
}  // namespace

class CcuAddInfoHostWorkerUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        if (File::Exist(HOST_PATH))
        {
            ASSERT_TRUE(File::RemoveDir(HOST_PATH, 0));
        }
        ASSERT_TRUE(File::CreateDir(HOST_PATH));
        ASSERT_TRUE(File::CreateDir(DATA_PATH));
        ASSERT_TRUE(File::CreateDir(File::PathJoin({HOST_PATH, "sqlite"})));
    }

    void TearDown() override
    {
        ASSERT_TRUE(File::RemoveDir(HOST_PATH, 0));
    }
};

TEST_F(CcuAddInfoHostWorkerUTest, TestRunShouldParseAndDumpCcuAddInfo)
{
    ASSERT_TRUE(WriteTaskRecord());
    HostTraceWorker worker(HOST_PATH);
    ASSERT_TRUE(worker.Run());

    DBRunner runner(DB_PATH);
    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM CCUTaskInfo", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ("100", std::get<2>(rows[0]));
    EXPECT_EQ(uint32_t{6}, std::get<7>(rows[0]));
    EXPECT_EQ(uint32_t{9}, std::get<10>(rows[0]));
}

TEST_F(CcuAddInfoHostWorkerUTest, TestRetryShouldNotDuplicateCommittedRows)
{
    ASSERT_TRUE(WriteTaskRecord());
    ASSERT_TRUE(HostTraceWorker(HOST_PATH).Run());
    ASSERT_TRUE(HostTraceWorker(HOST_PATH).Run());
    const auto marker = File::PathJoin({DATA_PATH, "unaging.additional.ccu_task_info.slice_0.complete"});
    ASSERT_TRUE(File::Exist(marker));
    ASSERT_TRUE(File::DeleteFile(marker));
    EXPECT_TRUE(HostTraceWorker(HOST_PATH).Run());
    DBRunner runner(DB_PATH);
    std::vector<std::tuple<uint64_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT COUNT(*) FROM CCUTaskInfo", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ(1U, std::get<0>(rows[0]));
}

TEST_F(CcuAddInfoHostWorkerUTest, TestParseFailureShouldNotChangeWorkerResult)
{
    const auto file = File::PathJoin({DATA_PATH, "unaging.additional.ccu_task_info.slice_0"});
    std::ofstream(file, std::ios::binary).close();
    EXPECT_TRUE(HostTraceWorker(HOST_PATH).Run());
    EXPECT_FALSE(File::Exist(DB_PATH));
    EXPECT_FALSE(File::Exist(file + ".complete"));
}

TEST_F(CcuAddInfoHostWorkerUTest, TestLookupShouldParseBeforeDumpAndWorkerShouldConsumeWarehouse)
{
    ASSERT_TRUE(WriteTaskRecord());
    auto grouper = std::make_shared<Host::Cann::EventGrouper>(DATA_PATH);
    ASSERT_TRUE(grouper->Group());
    auto &batches = grouper->GetDumpWarehouse().ccuInfoData;
    ASSERT_EQ(1U, batches.size());
    ASSERT_EQ(1U, batches[0]->taskRecords.size());
    EXPECT_EQ(6U, batches[0]->taskRecords[0].taskId);
    EXPECT_TRUE(batches[0]->taskSourceParsed);
    EXPECT_FALSE(File::Exist(DB_PATH));
    EXPECT_FALSE(File::Exist(File::PathJoin({DATA_PATH, "unaging.additional.ccu_task_info.slice_0.complete"})));

    // The worker must consume this parsed batch, not read the original task id again.
    batches[0]->taskRecords[0].taskId = 42;
    HostTraceWorker worker(HOST_PATH);
    ThreadPool pool(1);
    pool.Start();
    worker.DumpCcuAddInfo(pool, grouper);
    pool.WaitAllTasks();
    pool.Stop();
    DBRunner runner(DB_PATH);
    std::vector<std::tuple<uint32_t>> rows;
    ASSERT_TRUE(runner.QueryData("SELECT task_id FROM CCUTaskInfo", rows));
    ASSERT_EQ(1U, rows.size());
    EXPECT_EQ(42U, std::get<0>(rows[0]));
}

TEST_F(CcuAddInfoHostWorkerUTest, TestFailedLookupShouldClearPreviousBatch)
{
    ASSERT_TRUE(WriteTaskRecord());
    Host::Cann::EventGrouper grouper(DATA_PATH);
    ASSERT_TRUE(grouper.Group());
    ASSERT_EQ(1U, grouper.GetDumpWarehouse().ccuInfoData.size());
    const auto file = File::PathJoin({DATA_PATH, "unaging.additional.ccu_task_info.slice_0"});
    std::ofstream(file, std::ios::binary | std::ios::trunc).close();
    EXPECT_TRUE(grouper.Group());
    EXPECT_TRUE(grouper.GetDumpWarehouse().ccuInfoData.empty());
    EXPECT_FALSE(File::Exist(DB_PATH));
    EXPECT_FALSE(File::Exist(file + ".complete"));
}
