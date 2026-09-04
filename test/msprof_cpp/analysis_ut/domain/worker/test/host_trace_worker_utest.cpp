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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/host_worker/host_trace_worker.h"
#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;

const std::string TEST_HOST_FILE_PATH = "./";

class HostTraceWorkerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
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
