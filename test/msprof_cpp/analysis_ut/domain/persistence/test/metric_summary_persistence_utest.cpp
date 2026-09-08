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
#include <gmock/gmock.h>
#include "analysis/csrc/domain/services/persistence/device/metric_summary_persistence.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

using namespace testing;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace Analysis {
namespace Domain {
namespace {
    const std::string DEVICE_PATH = "./device_0";
}
class MetricSummaryPersistenceUTest : public Test {
protected:
    void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(DEVICE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({DEVICE_PATH, "sqlite"})));
        auto data = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
        dataInventory_.Inject(data);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(DEVICE_PATH, 0));
    }

protected:
    DataInventory dataInventory_;
};

static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTask()
{
    std::map<TaskId, std::vector<DeviceTask>> res;
    auto& res1 = res[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().acceleratorType = MIX_AIC;
    PmuInfoMixAccelerator mixAccelerator;
    mixAccelerator.aiCoreTime = 100.0;  // aic time 100.0
    mixAccelerator.aivTime = 200.0;  // aic time 200.0
    mixAccelerator.aicTotalCycles = 100;  // aic totalCycle 100
    mixAccelerator.aivTotalCycles = 200;  // aiv totalCycle 200
    mixAccelerator.aicPmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};  // aic pmu result 11个
    mixAccelerator.aivPmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0};  // aic 13个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoMixAccelerator>(mixAccelerator);
    res1.emplace_back();
    res1.back().acceleratorType = AIC;
    PmuInfoSingleAccelerator singleAccelerator1;
    singleAccelerator1.totalTime = 100.0;  // aic time 100.0
    singleAccelerator1.totalCycles = 100;  // aic totalCycle 100
    singleAccelerator1.pmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};  // aic pmu result 11个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(singleAccelerator1);
    res1.emplace_back();
    res1.back().acceleratorType = AIV;
    PmuInfoSingleAccelerator singleAccelerator2;
    singleAccelerator2.totalTime = 100.0;  // aiv time 100.0
    singleAccelerator2.totalCycles = 100;  // aiv totalCycle 100
    singleAccelerator2.pmuResult = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0};  // aiv 13个
    res1.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(singleAccelerator2);
    return res;
}

TEST_F(MetricSummaryPersistenceUTest, ShouldSavePmuDataSuccess)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_PIPE_UTILIZATION;
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateDeviceTask();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(MetricSummaryPersistenceUTest, ShouldReturnNoPMUWhenNoMetric)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateDeviceTask();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
}

TEST_F(MetricSummaryPersistenceUTest, TestRunShouldReturnErrorWhenDataIsNull)
{
    dataInventory_.RemoveRestData({});
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context));
}

TEST_F(MetricSummaryPersistenceUTest, ShouldReturnOkWhenDynamic)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_PIPE_UTILIZATION;
    context.deviceContextInfo.sampleInfo.dynamic = true;
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateDeviceTask();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));
    std::string sql = "SELECT COUNT(*) FROM MetricSummary";
    std::string dbPath = File::PathJoin({DEVICE_PATH, "sqlite", "metric_summary.db"});
    DBRunner runner(dbPath);
    std::vector<std::tuple<uint64_t>> res;
    runner.QueryData(sql, res);
    EXPECT_EQ(2ul, std::get<0>(res.back()));
}

static std::map<TaskId, std::vector<DeviceTask>> GenerateV6DeviceTask()
{
    std::map<TaskId, std::vector<DeviceTask>> res;
    auto& tasks = res[{1, 1, 1, 1}];
    tasks.emplace_back();
    tasks.back().acceleratorType = AIC;
    PmuInfoSingleAccelerator aicPmu;
    aicPmu.totalTime = 100.0;
    aicPmu.totalCycles = 100;
    aicPmu.pmuResult = {1.0, 2.0, 3.0};
    tasks.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(aicPmu);
    tasks.emplace_back();
    tasks.back().acceleratorType = AIV;
    PmuInfoSingleAccelerator aivPmu;
    aivPmu.totalTime = 200.0;
    aivPmu.totalCycles = 200;
    aivPmu.pmuResult = {1.0, 2.0, 3.0};
    tasks.back().pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(aivPmu);
    return res;
}

static std::vector<HalPmuData> GenerateV6BlockPmuData()
{
    // 模拟经PmuAssociation处理后的数据：host表中命中的task(100)已替换为真实streamId(7)，
    // 未命中的task(200)保持解析时默认值UINT16_MAX
    std::vector<HalPmuData> res;
    HalPmuData blockPmu;
    blockPmu.type = BLOCK_PMU;
    blockPmu.hd.taskId.taskId = 100;
    blockPmu.hd.taskId.streamId = 7;  // host表命中，已被替换为真实streamId
    blockPmu.pmu.timeList[0] = 1000000;
    blockPmu.pmu.timeList[1] = 2000000;
    blockPmu.pmu.coreType = 1;
    blockPmu.pmu.coreId = 2;
    res.push_back(blockPmu);

    HalPmuData blockPmu2;
    blockPmu2.type = BLOCK_PMU;
    blockPmu2.hd.taskId.taskId = 200;
    blockPmu2.hd.taskId.streamId = 65535;  // host表未命中，保持解析时默认值UINT16_MAX
    blockPmu2.pmu.timeList[0] = 1000000;
    blockPmu2.pmu.timeList[1] = 2000000;
    blockPmu2.pmu.coreType = 0;
    blockPmu2.pmu.coreId = 3;
    res.push_back(blockPmu2);

    HalPmuData contextPmu;
    contextPmu.type = PMU;
    contextPmu.hd.taskId.taskId = 300;
    contextPmu.pmu.timeList[0] = 1;
    contextPmu.pmu.timeList[1] = 2;
    res.push_back(contextPmu);
    return res;
}

TEST_F(MetricSummaryPersistenceUTest, ShouldSaveV6BlockPmuDataSuccess)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    context.deviceContextInfo.deviceInfo.hwtsFrequency = 1.0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION;

    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateV6DeviceTask();
    deviceTaskS->swap(deviceTask);
    // streamId替换由上游PmuAssociation完成（见PmuAssociationUTest），此处数据为替换后状态
    dataInventory_.Inject(std::make_shared<std::vector<HalPmuData>>(GenerateV6BlockPmuData()));

    ASSERT_EQ(ANALYSIS_OK, persistence.Run(dataInventory_, context));

    std::string dbPath = File::PathJoin({DEVICE_PATH, "sqlite", "metric_summary.db"});
    DBRunner runner(dbPath);
    std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, double, double, uint64_t, uint64_t>> rows;
    std::string sql = "SELECT stream_id, task_id, subtask_id, batch_id, start_time, duration, core_type, core_id "
                      "FROM V6BlockPmu ORDER BY task_id";
    ASSERT_TRUE(runner.QueryData(sql, rows));
    ASSERT_EQ(2ul, rows.size());

    EXPECT_EQ(7ul, std::get<0>(rows[0]));
    EXPECT_EQ(100ul, std::get<1>(rows[0]));
    EXPECT_EQ(4294967295ul, std::get<2>(rows[0]));
    EXPECT_EQ(0ul, std::get<3>(rows[0]));  // batch_id落盘为NULL，回读为0
    EXPECT_DOUBLE_EQ(1e9, std::get<4>(rows[0]));
    EXPECT_DOUBLE_EQ(1e6, std::get<5>(rows[0]));
    EXPECT_EQ(1ul, std::get<6>(rows[0]));
    EXPECT_EQ(2ul, std::get<7>(rows[0]));

    EXPECT_EQ(65535ul, std::get<0>(rows[1]));
    EXPECT_EQ(200ul, std::get<1>(rows[1]));
    EXPECT_EQ(4294967295ul, std::get<2>(rows[1]));
    EXPECT_EQ(0ul, std::get<3>(rows[1]));
    EXPECT_DOUBLE_EQ(1e9, std::get<4>(rows[1]));
    EXPECT_DOUBLE_EQ(1e6, std::get<5>(rows[1]));
    EXPECT_EQ(0ul, std::get<6>(rows[1]));
    EXPECT_EQ(3ul, std::get<7>(rows[1]));
}

TEST_F(MetricSummaryPersistenceUTest, ShouldReturnErrorWhenV6BlockPmuDataNull)
{
    MetricSummaryPersistence persistence;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = DEVICE_PATH;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    context.deviceContextInfo.deviceInfo.hwtsFrequency = 1.0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION;

    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto deviceTask = GenerateV6DeviceTask();
    deviceTaskS->swap(deviceTask);
    // 未注入 HalPmuData，SaveV6BlockPmuData 应返回失败
    ASSERT_EQ(ANALYSIS_ERROR, persistence.Run(dataInventory_, context));
}
}
}