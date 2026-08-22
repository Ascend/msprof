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
#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/persistence/device/device_hccl_persistence.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

using namespace Analysis::Utils;
using namespace Analysis::Application;
using namespace Analysis::Infra;

const std::string LOCAL_DIR = "./hccl_persistence";
const std::string DEVICE_DIR = File::PathJoin({LOCAL_DIR, "device_0"});
const std::string SQLITE_DIR = File::PathJoin({DEVICE_DIR, "sqlite"});
const std::string DB_PATH = File::PathJoin({SQLITE_DIR, "hccl_single_device.db"});

// 与 device_hccl_persistence.cpp 中 SaveHcclOpData 写入顺序一致：
// modelId, indexId, opName, taskType, opType, start, end, relay, retry, dataType, algType, count, groupName,
// connectionId, rankSize, iterId
using HcclOpDataFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, std::string, uint64_t,
        uint64_t, int32_t, int32_t, std::string, std::string, int32_t, std::string, int64_t, int64_t, uint32_t>>;
const HcclOpDataFormat HCCL_OP_DATA = {
    {4294967295, 0, "hcom_allReduce_", "HCCL", "hcom_allReduce_", 1000, 2000, 0, 0, "FP16", "HD",
        6291456, "group_1", 8, 8, 1},
    {4294967295, 0, "hcom_allGather_", "HCCL", "hcom_allGather_", 3000, 4000, 0, 0, "FP16", "RING",
        6291456, "group_2", 19, 8, 1},
};

// 与 device_hccl_persistence.cpp 中 SaveHcclTaskData 写入顺序一致：
// modelId, indexId, hcclName, groupName, planeId, timestamp, duration,
// opId, isMaster, streamId, taskId, contextId, batchId, size, bandwidth,
// localRank, remoteRank, rankSize, transportType, dataType, linkType, rdmaType, notifyId, iterId
using HcclTaskDataFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, std::string, int32_t, double,
        double, int64_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, double, double, int64_t, int64_t,
        int64_t, std::string, std::string, std::string, std::string, std::string, uint32_t>>;
const HcclTaskDataFormat HCCL_TASK_DATA = {
    {4294967295, -1, "RDMASend", "group_1", 0, 1000.0, 320.0, 8, 1, 4, 285, 1, 0, 1024.0, 0.0125,
        9, 1, 8, "RDMA", "FP16", "ROCE", "RDMA_SEND_NOTIFY", "4294967692", 1},
    {4294967295, -1, "Memcpy", "group_1", 0, 2000.0, 100.0, 8, 0, 4, 286, 1, 0, 2048.0, 0.01,
        9, 9, 8, "SDMA", "FP16", "ON_CHIP", "INVALID_TYPE", "38654705788", 1},
};

// 与 device_hccl_persistence.cpp 中 SaveHcclStatisticsData 写入顺序一致：
// opType, count, totalTime, min, avg, max
using HcclStatisticsFormat = std::vector<std::tuple<std::string, uint32_t, double, double, double, double>>;
const HcclStatisticsFormat HCCL_STATISTICS_DATA = {
    {"hcom_allReduce_", 1352, 5852671300, 211920, 4328898.890533, 133867780},
    {"hcom_batchSendRecv_", 96, 243562100, 542100, 2537105.208333, 105191720},
    {"hcom_broadcast_", 96, 70865740, 236820, 738184.791667, 3929660},
    {"hcom_allGather_", 50, 52400100, 530920, 1048002, 1810800},
};

class DeviceHcclPersistenceUTest : public testing::Test {
protected:
    Analysis::Infra::DataInventory dataInventory_;
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(LOCAL_DIR));
        EXPECT_TRUE(File::CreateDir(DEVICE_DIR));
        EXPECT_TRUE(File::CreateDir(SQLITE_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(LOCAL_DIR, 0));
    }

    void SetUp() override
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        auto hcclOp = GenerateHcclOpData();
        std::shared_ptr<std::vector<Analysis::Domain::DeviceHcclOp>> opData;
        MAKE_SHARED0_NO_OPERATION(opData, std::vector<Analysis::Domain::DeviceHcclOp>, std::move(hcclOp));
        dataInventory_.Inject(opData);

        auto hcclTask = GenerateHcclTaskData();
        std::shared_ptr<std::vector<Analysis::Domain::DeviceHcclTask>> hcclTaskData;
        MAKE_SHARED0_NO_OPERATION(hcclTaskData, std::vector<Analysis::Domain::DeviceHcclTask>, std::move(hcclTask));
        dataInventory_.Inject(hcclTaskData);

        auto hcclStatistics = GenerateHcclStatisticsData();
        std::shared_ptr<std::vector<Analysis::Domain::HcclStatistics>> hcclStatisticsData;
        MAKE_SHARED0_NO_OPERATION(hcclStatisticsData, std::vector<Analysis::Domain::HcclStatistics>,
                                  std::move(hcclStatistics));
        dataInventory_.Inject(hcclStatisticsData);
    }

    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }

    static std::vector<Analysis::Domain::DeviceHcclOp> GenerateHcclOpData()
    {
        std::vector<Analysis::Domain::DeviceHcclOp> opData;
        EXPECT_TRUE(Reserve(opData, HCCL_OP_DATA.size()));
        for (const auto& data : HCCL_OP_DATA) {
            Analysis::Domain::DeviceHcclOp op{};
            std::tie(op.modelId, op.indexId, op.opName, op.taskType, op.opType, op.start, op.end,
                     op.relay, op.retry, op.dataType, op.algType, op.count, op.groupName,
                     op.connectionId, op.rankSize, op.iterId) = data;
            opData.emplace_back(op);
        }
        return opData;
    }

    static std::vector<Analysis::Domain::DeviceHcclTask> GenerateHcclTaskData()
    {
        std::vector<Analysis::Domain::DeviceHcclTask> taskData;
        EXPECT_TRUE(Reserve(taskData, HCCL_TASK_DATA.size()));
        for (const auto& data : HCCL_TASK_DATA) {
            Analysis::Domain::DeviceHcclTask task{};
            std::tie(task.modelId, task.indexId, task.hcclName, task.groupName, task.planeId, task.timestamp,
                     task.duration, task.opId, task.isMaster, task.streamId, task.taskId, task.contextId,
                     task.batchId, task.size, task.bandwidth, task.localRank, task.remoteRank, task.rankSize,
                     task.transportType, task.dataType, task.linkType, task.rdmaType, task.notifyId,
                     task.iterId) = data;
            taskData.emplace_back(task);
        }
        return taskData;
    }

    static std::vector<Analysis::Domain::HcclStatistics> GenerateHcclStatisticsData()
    {
        std::vector<Analysis::Domain::HcclStatistics> taskData;
        EXPECT_TRUE(Reserve(taskData, HCCL_STATISTICS_DATA.size()));
        for (const auto& data : HCCL_STATISTICS_DATA) {
            Analysis::Domain::HcclStatistics task;
            std::tie(task.opType, task.count, task.totalTime, task.min, task.avg, task.max) = data;
            taskData.emplace_back(task);
        }
        return taskData;
    }
};

TEST_F(DeviceHcclPersistenceUTest, TestProcessEntryWhenProcessSuccessThenReturnOK)
{
    Analysis::Domain::DeviceHcclPersistence per;
    Analysis::Domain::DeviceContext context;
    MOCKER_CPP(&Analysis::Domain::DeviceContext::GetDeviceFilePath).stubs().will(returnValue(DEVICE_DIR));
    ASSERT_EQ(Analysis::ANALYSIS_OK, per.Run(dataInventory_, context));
    MOCKER_CPP(&Analysis::Domain::DeviceContext::GetDeviceFilePath).reset();

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    ASSERT_NE(dbRunner, nullptr);

    HcclOpDataFormat opData;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from HCCLOpSingleDevice", opData));
    EXPECT_EQ(HCCL_OP_DATA, opData);

    HcclTaskDataFormat taskData;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from HCCLTaskSingleDevice", taskData));
    EXPECT_EQ(HCCL_TASK_DATA, taskData);

    HcclStatisticsFormat statisticsData;
    EXPECT_TRUE(dbRunner->QueryData("SELECT * from HcclOpReport", statisticsData));
    EXPECT_EQ(HCCL_STATISTICS_DATA, statisticsData);
}

TEST_F(DeviceHcclPersistenceUTest, TestProcessEntryWhenCreateTableFailedThenReturnError)
{
    Analysis::Domain::DeviceHcclPersistence per;
    Analysis::Domain::DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_DIR;
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    ASSERT_EQ(Analysis::ANALYSIS_ERROR, per.Run(dataInventory_, context));
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}

TEST_F(DeviceHcclPersistenceUTest, TestProcessEntryWhenDataPointerIsNullThenReturnError)
{
    Analysis::Domain::DeviceHcclPersistence per;
    Analysis::Domain::DeviceContext context;
    Analysis::Infra::DataInventory tempDataInventory;
    ASSERT_EQ(Analysis::ANALYSIS_ERROR, per.Run(tempDataInventory, context));
}

TEST_F(DeviceHcclPersistenceUTest, TestProcessEntryWhenDataEmptyThenReturnOK)
{
    Analysis::Domain::DeviceHcclPersistence per;
    Analysis::Domain::DeviceContext context;
    Analysis::Infra::DataInventory tempDataInventory;

    std::vector<Analysis::Domain::DeviceHcclOp> hcclOp;
    std::shared_ptr<std::vector<Analysis::Domain::DeviceHcclOp>> opData;
    MAKE_SHARED0_NO_OPERATION(opData, std::vector<Analysis::Domain::DeviceHcclOp>, std::move(hcclOp));
    tempDataInventory.Inject(opData);

    std::vector<Analysis::Domain::DeviceHcclTask> hcclTask;
    std::shared_ptr<std::vector<Analysis::Domain::DeviceHcclTask>> hcclTaskData;
    MAKE_SHARED0_NO_OPERATION(hcclTaskData, std::vector<Analysis::Domain::DeviceHcclTask>, std::move(hcclTask));
    tempDataInventory.Inject(hcclTaskData);

    std::vector<Analysis::Domain::HcclStatistics> hcclStatistics;
    std::shared_ptr<std::vector<Analysis::Domain::HcclStatistics>> hcclStatisticsData;
    MAKE_SHARED0_NO_OPERATION(hcclStatisticsData, std::vector<Analysis::Domain::HcclStatistics>,
                              std::move(hcclStatistics));
    tempDataInventory.Inject(hcclStatisticsData);

    ASSERT_EQ(Analysis::ANALYSIS_OK, per.Run(tempDataInventory, context));
}
