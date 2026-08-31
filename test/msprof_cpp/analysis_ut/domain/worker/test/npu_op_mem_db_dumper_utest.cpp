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

#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/npu_op_mem_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/file.h"

using namespace Analysis::Domain;
using namespace Analysis::Domain::Host;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace {
const std::string TEST_DIR = "./npu_op_mem_dumper_test";
}

class NpuOpMemDBDumperUTest : public testing::Test {
protected:
    void SetUp() override
    {
        File::CreateDir(TEST_DIR);
        File::CreateDir(File::PathJoin({TEST_DIR, "sqlite"}));
    }

    void TearDown() override
    {
        File::RemoveDir(TEST_DIR, 0);
    }
};

TEST_F(NpuOpMemDBDumperUTest, DumpDataShouldCreateExpectedTables)
{
    auto raw = std::make_shared<ParserAdditionalInfo>();
    raw->dataLen = sizeof(ParserMemoryInfo);
    raw->timeStamp = 4;
    raw->threadId = 5;
    raw->level = 8;
    raw->type = 9;
    raw->memoryInfo.nodeId = 1;
    raw->memoryInfo.addr = 2;
    raw->memoryInfo.size = 3;
    raw->memoryInfo.totalAllocateMemory = 6;
    raw->memoryInfo.totalReserveMemory = 7;
    raw->memoryInfo.deviceId = 10;

    NpuOpMemRecordData record;
    record.timestamp = 4;
    record.totalReserveMemory = 7;
    record.totalAllocateMemory = 6;
    record.deviceType = "NPU:10";

    NpuOpMemLifecycleData lifecycle;
    lifecycle.operatorId = 1;
    lifecycle.size = 3;
    lifecycle.allocationTime = 4;
    lifecycle.releaseTime = 14;
    lifecycle.duration = 10;
    lifecycle.allocationTotalAllocated = 6;
    lifecycle.allocationTotalReserved = 7;
    lifecycle.releaseTotalAllocated = 16;
    lifecycle.releaseTotalReserved = 17;
    lifecycle.deviceType = "NPU:10";
    lifecycle.name = "op_name";

    ASSERT_TRUE(NpuOpMemRawDBDumper(TEST_DIR).DumpData(
        std::vector<std::shared_ptr<ParserAdditionalInfo>>{raw}));
    ASSERT_TRUE(NpuOpMemRecordDBDumper(TEST_DIR).DumpData(std::vector<NpuOpMemRecordData>{record}));
    ASSERT_TRUE(NpuOpMemLifecycleDBDumper(TEST_DIR).DumpData(std::vector<NpuOpMemLifecycleData>{lifecycle}));

    DBRunner runner(File::PathJoin({TEST_DIR, "sqlite", "task_memory.db"}));
    NpuOpMemRawDBData rawResult;
    NpuOpMemRecordDBData recordResult;
    NpuOpMemLifecycleDBData lifecycleResult;
    ASSERT_TRUE(runner.QueryData("select operator, addr, size, timestamp, thread_id, total_allocate_memory, "
                                 "total_reserve_memory, level, type, device_type from NpuOpMemRaw", rawResult));
    ASSERT_TRUE(runner.QueryData("select component, timestamp, total_reserve_memory, total_allocate_memory, "
                                 "device_type from NpuOpMemRec", recordResult));
    ASSERT_TRUE(runner.QueryData("select operator, size, allocation_time, release_time, duration, "
                                 "allocation_total_allocated, allocation_total_reserved, release_total_allocated, "
                                 "release_total_reserved, device_type, name from NpuOpMem", lifecycleResult));

    ASSERT_EQ(rawResult.size(), 1U);
    EXPECT_EQ(rawResult[0], NpuOpMemRawDBData::value_type("1", "2", 3, 4, 5, 6, 7, 8, 9, "NPU:10"));
    ASSERT_EQ(recordResult.size(), 1U);
    EXPECT_EQ(recordResult[0], NpuOpMemRecordDBData::value_type("GE", 4, 7, 6, "NPU:10"));
    ASSERT_EQ(lifecycleResult.size(), 1U);
    EXPECT_EQ(lifecycleResult[0], NpuOpMemLifecycleDBData::value_type(
        "1", 3, 4, 14, 10, 6, 7, 16, 17, "NPU:10", "op_name"));
}

TEST_F(NpuOpMemDBDumperUTest, DumpDataShouldDistinguishEmptyInputFromGenerateFailure)
{
    NpuOpMemRawDBDumper dumper(TEST_DIR);
    std::vector<std::shared_ptr<ParserAdditionalInfo>> emptyInput;
    std::vector<std::shared_ptr<ParserAdditionalInfo>> invalidInput{nullptr};

    EXPECT_TRUE(dumper.DumpData(emptyInput));
    EXPECT_FALSE(dumper.DumpData(invalidInput));
}
