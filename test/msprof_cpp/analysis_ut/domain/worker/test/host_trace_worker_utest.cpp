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
#include <memory>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"
#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
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
