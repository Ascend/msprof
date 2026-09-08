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
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/services/association/include/pmu_association.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/arith_metric_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeut_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeutext_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_l0_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_ub_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/resource_conflict_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_access_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/l2_cache_calculator.h"

using namespace testing;
using namespace Analysis::Utils;
using namespace Analysis::Infra;

namespace Analysis {
namespace Domain {
const int TEST_ID = 1;
class PmuAssociationUTest : public Test {
protected:
    void SetUp()
    {
        auto data = std::make_shared<std::vector<HalPmuData>>();
        dataInventory_.Inject(data);
        auto deviceTask = std::make_shared<std::map<TaskId, std::vector<DeviceTask>>>();
        dataInventory_.Inject(deviceTask);
    }

    void TearDown()
    {
        dataInventory_.RemoveRestData({});
    }

protected:
    DataInventory dataInventory_;
};

static std::vector<HalPmuData> GenerateHalPmuData()
{
    std::vector<HalPmuData> pmuData;
    constexpr size_t SEQ_NUM = 20;
    static const uint16_t TASK_ID_SEQ[SEQ_NUM]{1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    static const uint16_t BATCH_ID_SEQ[SEQ_NUM]{1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 1, 1, 1, 1, 1};
    static const uint64_t TIMESTAMP_SEQ[SEQ_NUM]{100, 150, 200, 250, 300, 110, 110, 110, 160, 160, 160, 260, 260, 260,
                                                 310, 310, 310, 310, 310, 310};
    static const HalPmuType PMU_TYPE_SEQ[SEQ_NUM]{PMU, PMU, PMU, PMU, PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU,
                                                  BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU,
                                                  BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU, BLOCK_PMU};
    static const AcceleratorType ACC_TYPE_SEQ[SEQ_NUM]{MIX_AIC, MIX_AIC, AIC, MIX_AIV, MIX_AIC, MIX_AIC, MIX_AIC,
                                                       MIX_AIC, MIX_AIC, MIX_AIC, MIX_AIC, MIX_AIV, MIX_AIV, MIX_AIV,
                                                       MIX_AIC, MIX_AIC, MIX_AIC, MIX_AIC, MIX_AIC, MIX_AIC};
    static const uint8_t CORE_TYPE_SEQ[SEQ_NUM]{0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0};
    HalPmuData pmuInfo{};
    for (size_t i = 0; i < SEQ_NUM; ++i) {
        pmuInfo.type = PMU_TYPE_SEQ[i];
        pmuInfo.hd.taskId.streamId = TEST_ID;
        pmuInfo.hd.taskId.taskId = TASK_ID_SEQ[i];
        pmuInfo.hd.taskId.batchId = BATCH_ID_SEQ[i];
        pmuInfo.hd.taskId.contextId = TEST_ID;
        pmuInfo.hd.timestamp = TIMESTAMP_SEQ[i];
        pmuInfo.pmu.acceleratorType = ACC_TYPE_SEQ[i];
        pmuInfo.pmu.coreType = CORE_TYPE_SEQ[i];
        pmuInfo.pmu.totalCycle = 100;  // 100为测试cycle
        for (size_t j = 0; j < DEFAULT_PMU_LENGTH; ++j) {
            pmuInfo.pmu.pmuList[j] = j;
        }
        pmuData.push_back(pmuInfo);
    }
    return pmuData;
}

static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTask()
{
    std::map<TaskId, std::vector<DeviceTask>> deviceTask;
    auto& res1 = deviceTask[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().mixBlockNum = 3;  // 3为从核数
    res1.back().blockNum = 8;  // 8为主核数
    res1.back().taskStart = 100; // 第一个task开始时间为100
    res1.back().taskEnd = 130;  // 第一个task结束时间为130
    res1.emplace_back();
    res1.back().mixBlockNum = 3;  // 3为从核数
    res1.back().blockNum = 8;  // 8为主核数
    res1.back().taskStart = 150; // 第2个task开始时间为150
    res1.back().taskEnd = 170;  // 第2个task结束时间为170
    auto& res2 = deviceTask[{1, 2, 1, 1}];
    res2.emplace_back();
    res2.back().mixBlockNum = 3;  // 3为从核数
    res2.back().blockNum = 8;  // 8为主核数
    res2.back().taskStart = 200; // 第3个task开始时间为200
    res2.back().taskEnd = 230;  // 第3个task结束时间为230
    auto& res3 = deviceTask[{1, 3, 1, 1}];
    res3.emplace_back();
    res3.back().mixBlockNum = 3;  // 3为从核数
    res3.back().blockNum = 8;  // 8为主核数
    res3.back().taskStart = 250; // 第4个task开始时间为250
    res3.back().taskEnd = 270; // 第4个task结束时间为270
    auto& res4 = deviceTask[{1, 1, 2, 1}];
    res4.emplace_back();
    res4.back().mixBlockNum = 3;  // 3为从核数
    res4.back().blockNum = 8;  // 8为主核数
    res4.back().taskStart = 300; // 第5个task开始时间为300
    res4.back().taskEnd = 330; // 第5个task结束时间为330
    return deviceTask;
}

static void SetDeviceContext(DeviceContext& context)
{
    uint32_t arr1[8]{0x416, 0x417, 0x9, 0x302, 0xc, 0x303, 0x54, 0x55};
    uint32_t arr2[8]{0x8, 0xa, 0x9, 0xb, 0xc, 0xd, 0x54, 0x55};
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_PIPE_UTILIZATION;
    context.deviceContextInfo.deviceInfo.aiCoreNum = 8;  // 8为cube核数
    context.deviceContextInfo.deviceInfo.aivNum = 8;   // 8为vector核数
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    for (size_t i = 0; i < DEFAULT_PMU_LENGTH; i++) {
        context.deviceContextInfo.sampleInfo.aivProfilingEvents[i] = arr2[i];
        context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents[i] = arr1[i];
    }
}

/**
 * 用例设计如下：
 *   一、DeviceTask：
 *   streamId-taskId-contextId-batchId        block_num     mix_block_num
 *                1-1-1-1                         8              3
 *                1-1-1-1                         8              3
 *                1-1-1-2                         8              3
 *                1-1-1-3                         8              3
 *                1-2-1-1                         8              3
 *   二、HalPmuData:
 *   core_type字段的默认值为0，取值范围：0：AIC， 1：AIV  该字段代表数据是由什么核上报的
 *   1、context级别PMU（没有core_type字段，全都给默认值0）：
 *   streamId-taskId-contextId-batchId       acceleratorType              pmuList               totalCycle     core_type
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-1-1-2                          AIC             [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-1-1-3                        MIX_AIV           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *   2、block级别PMU（有core_type字段）：
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-1-1-3                        MIX_AIV           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-1-1-3                        MIX_AIV           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-1-1-3                        MIX_AIV           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIV
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *                1-2-1-1                        MIX_AIC           [0, 1, 2, 3, 4, 5, 6, 7]         100           AIC
 *   三、taskBlock开关为false
 *   四、关联后的结果：DeviceTask
 *   ----------------------------------------------task1--------------------------------------
 *   TaskId is :1-1-1-1
 *   acceleratorType: 2-MIX_AIC
 *   total count : 3
 *   aicTotalCycle:100
 *   aivTotalCycle:300
 *   aic value is : 0.01 0.03 0.06 0.1 0.15 0.28 0.03625 0.03875 0.0425 0.0475 0.05375
 *   aiv value is : 0 0 0.06 0.06 0.18 0.18 0.57 0.19 0.19 0.21 0.21 0.25 0.25
 *   ----------------------------------------------task2--------------------------------------
 *   acceleratorType: 2-MIX_AIC
 *   total count : 3
 *   aicTotalCycle:100
 *   aivTotalCycle:300
 *   aic value is : 0.01 0.03 0.06 0.1 0.15 0.28 0.03625 0.03875 0.0425 0.0475 0.05375
 *   aiv value is : 0 0 0.06 0.06 0.18 0.18 0.57 0.19 0.19 0.21 0.21 0.25 0.25
 *   ----------------------------------------------task3-------------------------------------
 *   TaskId is :1-1-1-2
 *   acceleratorType: 0-AIC
 *   TotalCycle:100
 *   pmu value is : 0.01 0.03 0.06 0.1 0.15 0.28 0.03625 0.03875 0.0425 0.0475 0.05375
 *   ----------------------------------------------task4-------------------------------------
 *   TaskId is :1-1-1-3
 *   acceleratorType: 3-MIX_AIV
 *   total count : 3
 *   aicTotalCycle:300
 *   aivTotalCycle:100
 *   aic value is : 0 0.06 0.06 0.18 0.18 0.57 0.19 0.21 0.21 0.25 0.25
 *   aiv value is : 0 0.01 0.03 0.06 0.1 0.15 0.28 0.0933333 0.0966667 0.103333 0.113333 0.126667 0.143333
 *   ----------------------------------------------task5-------------------------------------
 *   TaskId is :1-2-1-1
 *   acceleratorType: 2-MIX_AIC
 *   total count : 3
 *   aicTotalCycle:100
 *   aivTotalCycle:300
 *   aic value is : 0.01 0.03 0.06 0.1 0.15 0.28 0.03625 0.03875 0.0425 0.0475 0.05375
 *   aiv value is : 0 0 0.06 0.06 0.18 0.18 0.57 0.19 0.19 0.21 0.21 0.25 0.25
 */
TEST_F(PmuAssociationUTest, ShouldAssociateRightWhenProcessRun)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetDeviceContext(context);
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_EQ(4ul, result->size());
    const uint64_t EXCEPT_AIC_TOTAL_CYCLE[4]{100, 300, 100};
    const uint64_t EXCEPT_AIV_TOTAL_CYCLE[4]{300, 100, 300};
    const uint64_t AIV_LENGTH = 13;  // 预期aiv计算指标长度
    const uint64_t AIC_LENGTH = 11;  // 预期aic计算指标长度
    int index = 0;
    for (auto& it : *result) {
        if (it.second[0].acceleratorType == MIX_AIC || it.second[0].acceleratorType == MIX_AIV) {
            auto res = dynamic_cast<PmuInfoMixAccelerator *>(it.second[0].pmuInfo.get());
            ASSERT_EQ(3ul, res->totalBlockCount);
            ASSERT_EQ(AIV_LENGTH, res->aivPmuResult.size());
            ASSERT_EQ(EXCEPT_AIC_TOTAL_CYCLE[index], res->aicTotalCycles);
            ASSERT_EQ(EXCEPT_AIV_TOTAL_CYCLE[index], res->aivTotalCycles);
            ++index;
            ASSERT_EQ(AIC_LENGTH, res->aicPmuResult.size());
        } else {
            auto res = dynamic_cast<PmuInfoSingleAccelerator *>(it.second[0].pmuInfo.get());
            ASSERT_EQ(100ul, res->totalCycles);
            ASSERT_EQ(AIC_LENGTH, res->pmuResult.size());
        }
    }
}

TEST_F(PmuAssociationUTest, ShouldReturnOKWhenDeviceTaskIsNull)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
}

TEST_F(PmuAssociationUTest, ShouldReturnErrorWhenPMUEventsNotAsExpected)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    std::vector<AicMetricsEventsType> aicEvents{AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION,
                                                AicMetricsEventsType::AIC_MEMORY,
                                                AicMetricsEventsType::AIC_MEMORY_UB,
                                                AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO};
    std::vector<AivMetricsEventsType> aivEvents{AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION,
                                                AivMetricsEventsType::AIV_MEMORY,
                                                AivMetricsEventsType::AIV_MEMORY_UB,
                                                AivMetricsEventsType::AIV_RESOURCE_CONFLICT_RATIO};
    for (size_t i = 0; i < aivEvents.size(); ++i) {
        context.deviceContextInfo.sampleInfo.aiCoreMetrics = aicEvents[i];
        context.deviceContextInfo.sampleInfo.aivMetrics = aivEvents[i];
        ASSERT_EQ(ANALYSIS_ERROR, pmuAssociation.Run(dataInventory_, context));
    }
}

TEST_F(PmuAssociationUTest, ShouldReturnOKWhenPMUISMemory)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_MEMORY_L0;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_MEMORY_L0;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    uint32_t arr1[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    uint32_t arr2[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    for (size_t i = 0; i < DEFAULT_PMU_LENGTH; i++) {
        context.deviceContextInfo.sampleInfo.aivProfilingEvents[i] = arr2[i];
        context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents[i] = arr1[i];
    }
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
}

TEST_F(PmuAssociationUTest, ShouldReturnERRORWhenMetricsInvalid)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_METRICS_UNKNOWN;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_METRICS_UNKNOWN;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    uint32_t arr1[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    uint32_t arr2[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    for (size_t i = 0; i < DEFAULT_PMU_LENGTH; i++) {
        context.deviceContextInfo.sampleInfo.aivProfilingEvents[i] = arr2[i];
        context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents[i] = arr1[i];
    }
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_ERROR, pmuAssociation.Run(dataInventory_, context));
}

TEST_F(PmuAssociationUTest, ShouldReturnErrorWhenEventSizeNotEqualWithPmuSize)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    uint32_t arr1[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    uint32_t arr2[8]{0x1b, 0x1c, 0x21, 0x22, 0x27, 0x28, 0x29, 0x2a};
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    for (size_t i = 0; i < DEFAULT_PMU_LENGTH; i++) {
        context.deviceContextInfo.sampleInfo.aivProfilingEvents[i] = arr2[i];
        context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents[i] = arr1[i];
    }
    context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents.push_back(0);
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_ERROR, pmuAssociation.Run(dataInventory_, context));
}

/*
 * 测试PMU数据比task数据多，例如：
 * pmu   [100, 500, 800]
 * task  [480-600, 800-900]
 * res   [480-600(500), 800-900(800)]
 */
static std::vector<HalPmuData> GeneratePMUMoreThanTask()
{
    std::vector<HalPmuData> pmuData;
    constexpr size_t pmuSize = 3;
    static const uint64_t TIME_SQE[pmuSize]{100, 500, 800};
    HalPmuData pmuInfo{};
    for (size_t i = 0; i < pmuSize; ++i) {
        pmuInfo.type = PMU;
        pmuInfo.hd.taskId.streamId = TEST_ID;
        pmuInfo.hd.taskId.taskId = TEST_ID;
        pmuInfo.hd.taskId.batchId = TEST_ID;
        pmuInfo.hd.taskId.contextId = TEST_ID;
        pmuInfo.hd.timestamp = TIME_SQE[i];
        pmuInfo.pmu.acceleratorType = AIC;
        pmuInfo.pmu.coreType = 0;
        pmuInfo.pmu.totalCycle = 100 * (i + 1);  // 100为测试cycle倍数
        for (size_t j = 0; j < DEFAULT_PMU_LENGTH; ++j) {
            pmuInfo.pmu.pmuList[j] = j;
        }
        pmuData.push_back(pmuInfo);
    }
    return pmuData;
}

static std::map<TaskId, std::vector<DeviceTask>> GenerateDeviceTaskForQueue()
{
    std::map<TaskId, std::vector<DeviceTask>> deviceTask;
    auto& res1 = deviceTask[{1, 1, 1, 1}];
    res1.emplace_back();
    res1.back().mixBlockNum = 3;  // 3为从核数
    res1.back().blockNum = 8;  // 8为主核数
    res1.back().taskStart = 480; // 第一个task开始时间为480
    res1.back().taskEnd = 600;  // 第一个task结束时间为600
    res1.emplace_back();
    res1.back().mixBlockNum = 3;  // 3为从核数
    res1.back().blockNum = 8;  // 8为主核数
    res1.back().taskStart = 800; // 第2个task开始时间为800
    res1.back().taskEnd = 900;  // 第2个task结束时间为900
    return deviceTask;
}

TEST_F(PmuAssociationUTest, ShouldMergeRightPmuWhenPmuMoreThanTask)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetDeviceContext(context);
    auto pmuData = GeneratePMUMoreThanTask();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTaskForQueue();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_EQ(1ul, result->size());
    std::vector<uint64_t> expectCycle{200, 300};
    for (const auto& it : *result) {
        int i = 0;
        for (const auto& data : it.second) {
            auto res = dynamic_cast<PmuInfoSingleAccelerator *>(data.pmuInfo.get());
            EXPECT_EQ(expectCycle[i], res->totalCycles);
            i++;
        }
    }
}

/*
 * 测试PMU数据比task数据少，例如：
 * pmu   [800]
 * task  [480-600, 800-900]
 * res   [800-900(800)]
 */
static std::vector<HalPmuData> GeneratePMULessThanTask()
{
    std::vector<HalPmuData> pmuData;
    HalPmuData pmuInfo{};
    pmuInfo.type = PMU;
    pmuInfo.hd.taskId.streamId = TEST_ID;
    pmuInfo.hd.taskId.taskId = TEST_ID;
    pmuInfo.hd.taskId.batchId = TEST_ID;
    pmuInfo.hd.taskId.contextId = TEST_ID;
    pmuInfo.hd.timestamp = 800; // pmu时间为800
    pmuInfo.pmu.acceleratorType = AIC;
    pmuInfo.pmu.coreType = 0;
    pmuInfo.pmu.totalCycle = 100; // totalCycle为100
    for (size_t j = 0; j < DEFAULT_PMU_LENGTH; ++j) {
        pmuInfo.pmu.pmuList[j] = j;
    }
    pmuData.push_back(pmuInfo);
    return pmuData;
}

TEST_F(PmuAssociationUTest, ShouldGetDeviceTaskWithPmuTheSameAsPmuCount)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetDeviceContext(context);
    auto pmuData = GeneratePMULessThanTask();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTaskForQueue();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_EQ(1ul, result->size());
    for (const auto& it : *result) {
        int i = 0;
        for (const auto& data : it.second) {
            auto res = dynamic_cast<PmuInfoSingleAccelerator *>(data.pmuInfo.get());
            if (i == 0) {
                EXPECT_EQ(AcceleratorType::INVALID, data.acceleratorType);
            } else {
                EXPECT_EQ(100ul, res->totalCycles);
            }
            i++;
        }
    }
}

TEST_F(PmuAssociationUTest, ShouldReturnOKWhenPMUISMemoryAccess)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_MEMORY_ACCESS;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_MEMORY_ACCESS;
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V4_1_0;
    uint32_t arr1[8]{0x32, 0x3d, 0x3e, 0x206, 0x20c, 0x50c, 0x50d, 0x50e};
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    for (size_t i = 0; i < DEFAULT_PMU_LENGTH; i++) {
    context.deviceContextInfo.sampleInfo.aivProfilingEvents[i] = arr1[i];
    context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents[i] = arr1[i];
    }
    auto pmuData = GenerateHalPmuData();
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateDeviceTask();
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
}

/**
 * 计算器工厂与芯片配置分发的单元测试。
 *  - 工厂只按指标类型创建计算器，不感知芯片（芯片信息由计算器内部获取）；
 *  - 有芯片差异的指标（L2Cache/ArithMetric等）也只实例化同一个计算器类，
 *    差异由计算器内部的 Get*Config(chipId) switch 分发。
 */
template <typename Expected>
static bool IsSameCalculatorType(const std::unique_ptr<MetricCalculator>& calculator)
{
    return calculator != nullptr && dynamic_cast<Expected *>(calculator.get()) != nullptr;
}

TEST_F(PmuAssociationUTest, ShouldCreateCalculatorByMetricType)
{
    ASSERT_TRUE(IsSameCalculatorType<ArithMetricCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION)));
    ASSERT_TRUE(IsSameCalculatorType<PipeUtCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_PIPE_UTILIZATION)));
    ASSERT_TRUE(IsSameCalculatorType<PipeUtExtCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT)));
    ASSERT_TRUE(
        IsSameCalculatorType<MemoryCalculator>(MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_MEMORY)));
    ASSERT_TRUE(IsSameCalculatorType<MemoryL0Calculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_MEMORY_L0)));
    ASSERT_TRUE(IsSameCalculatorType<ResourceConflictCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO)));
    ASSERT_TRUE(IsSameCalculatorType<MemoryUBCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_MEMORY_UB)));
    ASSERT_TRUE(IsSameCalculatorType<L2CacheCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_L2_CACHE)));
    ASSERT_TRUE(IsSameCalculatorType<MemoryAccessCalculator>(
        MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_MEMORY_ACCESS)));

    ASSERT_TRUE(IsSameCalculatorType<ArithMetricCalculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION)));
    ASSERT_TRUE(IsSameCalculatorType<PipeUtCalculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_PIPE_UTILIZATION)));
    ASSERT_TRUE(
        IsSameCalculatorType<MemoryCalculator>(MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_MEMORY)));
    ASSERT_TRUE(IsSameCalculatorType<MemoryL0Calculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_MEMORY_L0)));
    ASSERT_TRUE(IsSameCalculatorType<ResourceConflictCalculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_RESOURCE_CONFLICT_RATIO)));
    ASSERT_TRUE(
        IsSameCalculatorType<MemoryUBCalculator>(MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_MEMORY_UB)));
    ASSERT_TRUE(IsSameCalculatorType<L2CacheCalculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_L2_CACHE)));
    ASSERT_TRUE(IsSameCalculatorType<MemoryAccessCalculator>(
        MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_MEMORY_ACCESS)));

    // 无效的指标类型返回nullptr
    ASSERT_TRUE(MetricCalculatorFactory::GetAicCalculator(AicMetricsEventsType::AIC_METRICS_UNKNOWN) == nullptr);
    ASSERT_TRUE(MetricCalculatorFactory::GetAivCalculator(AivMetricsEventsType::AIV_METRICS_UNKNOWN) == nullptr);
}

TEST_F(PmuAssociationUTest, ShouldUseV6ConfigForArithMetric)
{
    ArithMetricCalculator v6Calculator(CHIP_V6_1_0);
    ArithMetricCalculator defaultCalculator(CHIP_V4_1_0);

    // V6 仅包含 3 个算术指标（无 vector 相关指标），默认芯片包含 8 个
    ASSERT_EQ(3ul, v6Calculator.GetPmuHeader().size());
    ASSERT_EQ(8ul, defaultCalculator.GetPmuHeader().size());

    // V6 使用新事件 ID 0x323/0x324，默认芯片不识别该事件组合
    std::vector<uint32_t> v6Events{0x323, 0x324};
    ASSERT_TRUE(v6Calculator.CheckMetricEventValid(v6Events));
    ASSERT_FALSE(defaultCalculator.CheckMetricEventValid(v6Events));
}

TEST_F(PmuAssociationUTest, ShouldUseChipConfigInsideL2CacheCalculator)
{
    L2CacheCalculator v6Calculator(CHIP_V6_1_0);
    L2CacheCalculator defaultCalculator(CHIP_V4_1_0);

    // V6 与默认芯片的 L2Cache 指标列名不同
    ASSERT_EQ((std::vector<std::string>{"read_local_l2_hit", "read_local_l2_miss", "read_local_l2_victim",
                                        "write_local_l2_hit", "write_local_l2_miss", "write_local_l2_victim"}),
              v6Calculator.GetPmuHeader());
    ASSERT_EQ((std::vector<std::string>{"write_cache_hit", "write_cache_miss_allocate", "r0_read_cache_hit",
                                        "r0_read_cache_miss_allocate", "r1_read_cache_hit", "r1_read_cache_miss_allocate"}),
              defaultCalculator.GetPmuHeader());

    // V6 使用寄存器 0x424~0x42c，默认芯片使用 0x500~0x50a，互不识别
    std::vector<uint32_t> v6Events{0x424, 0x425, 0x426, 0x42a, 0x42b, 0x42c};
    std::vector<uint32_t> defaultEvents{0x500, 0x502, 0x504, 0x506, 0x508, 0x50a};
    ASSERT_TRUE(v6Calculator.CheckMetricEventValid(v6Events));
    ASSERT_FALSE(v6Calculator.CheckMetricEventValid(defaultEvents));
    ASSERT_TRUE(defaultCalculator.CheckMetricEventValid(defaultEvents));
    ASSERT_FALSE(defaultCalculator.CheckMetricEventValid(v6Events));
}

/*
 * V6芯片相关用例公共数据构造：
 *   V6 PMU不携带streamId（解析时填UINT16_MAX），pmuList固定长度为V6_PMU_LENGTH(10)，
 *   事件使用V6 ArithMetric新事件ID（0x323/0x324）
 */
constexpr size_t V6_TEST_PMU_LENGTH = 10;  // 与chip6_pmu_parser_item.h中V6_PMU_LENGTH一致
constexpr uint32_t V6_TEST_STREAM_ID = 7;  // host task表中的真实streamId

static void SetV6DeviceContext(DeviceContext& context)
{
    context.deviceContextInfo.deviceInfo.chipID = CHIP_V6_1_0;
    context.deviceContextInfo.sampleInfo.aiCoreMetrics = AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION;
    context.deviceContextInfo.sampleInfo.aivMetrics = AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION;
    context.deviceContextInfo.deviceInfo.aiCoreNum = 8;  // 8为cube核数
    context.deviceContextInfo.deviceInfo.aivNum = 8;   // 8为vector核数
    context.deviceContextInfo.deviceInfo.aicFrequency = 100;  // 100为aic频率，单位：MHZ
    context.deviceContextInfo.deviceInfo.hwtsFrequency = 100.0;  // 100为hwts频率，单位：MHZ
    context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents = {0x323, 0x324};
    context.deviceContextInfo.sampleInfo.aivProfilingEvents = {0x323, 0x324};
}

static std::vector<HalPmuData> GenerateV6HalPmuData(uint64_t startSyscnt, uint64_t endSyscnt)
{
    std::vector<HalPmuData> pmuData;
    HalPmuData pmuInfo{};
    pmuInfo.type = PMU;
    pmuInfo.hd.taskId.streamId = UINT16_MAX;  // V6 PMU不携带streamId，解析时填默认值UINT16_MAX
    pmuInfo.hd.taskId.taskId = TEST_ID;
    pmuInfo.hd.taskId.batchId = TEST_ID;
    pmuInfo.hd.taskId.contextId = TEST_ID;
    pmuInfo.hd.timestamp = 100; // pmu上报时间为100
    pmuInfo.pmu.acceleratorType = AIC;
    pmuInfo.pmu.coreType = 0;
    pmuInfo.pmu.totalCycle = 100; // totalCycle为100
    pmuInfo.pmu.timeList[0] = startSyscnt;
    pmuInfo.pmu.timeList[1] = endSyscnt;
    pmuInfo.pmu.pmuList.resize(V6_TEST_PMU_LENGTH);
    for (size_t j = 0; j < V6_TEST_PMU_LENGTH; ++j) {
        pmuInfo.pmu.pmuList[j] = j;
    }
    pmuData.push_back(pmuInfo);
    return pmuData;
}

static std::map<TaskId, std::vector<DeviceTask>> GenerateV6DeviceTask(uint32_t streamId)
{
    std::map<TaskId, std::vector<DeviceTask>> deviceTask;
    auto& res = deviceTask[{streamId, TEST_ID, TEST_ID, TEST_ID}];  // {streamId, batchId, taskId, contextId}
    res.emplace_back();
    res.back().mixBlockNum = 3;  // 3为从核数
    res.back().blockNum = 8;  // 8为主核数
    res.back().taskStart = 50; // task开始时间为50
    res.back().taskEnd = 150;  // task结束时间为150
    return deviceTask;
}

/*
 * V6芯片：PMU原始streamId为UINT16_MAX，应被host task表中的streamId替换后再与task关联
 */
TEST_F(PmuAssociationUTest, ShouldReplaceStreamIdByHostStreamInfoWhenChipV6)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    ASSERT_EQ(UINT16_MAX, pmuData[0].hd.taskId.streamId);
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    // task表使用替换后的streamId，原始streamId(UINT16_MAX)无法直接匹配
    auto deviceTask = GenerateV6DeviceTask(V6_TEST_STREAM_ID);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    // host task表：taskId=1 -> streamId=7
    auto hostStreamInfo = std::make_shared<HostStreamInfo>();
    hostStreamInfo->streamIdMap[TEST_ID] = V6_TEST_STREAM_ID;
    dataInventory_.Inject(hostStreamInfo);

    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_EQ(1ul, result->size());
    const auto& task = result->begin()->second.at(0);
    auto res = dynamic_cast<PmuInfoSingleAccelerator *>(task.pmuInfo.get());
    ASSERT_NE(nullptr, res);
    // PMU成功关联到替换后streamId的task上，指标计算结果非空
    EXPECT_EQ(V6_TEST_STREAM_ID, result->begin()->first.streamId);
    EXPECT_FALSE(res->pmuResult.empty());
}

/*
 * V6芯片：events数量(11)超过pmuList长度(10)时返回错误
 */
TEST_F(PmuAssociationUTest, ShouldReturnErrorWhenV6EventSizeExceedsPmuList)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    // 11个事件超过pmuList长度V6_PMU_LENGTH(10)，前两个事件保持合法以通过calculator校验
    context.deviceContextInfo.sampleInfo.aiCoreProfilingEvents = {0x323, 0x324, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                                                                  0x8, 0x9};
    auto pmuData = GenerateV6HalPmuData(100, 200);
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(V6_TEST_STREAM_ID);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_ERROR, pmuAssociation.Run(dataInventory_, context));
}

/*
 * V6芯片非mix场景：timeList合法(单调递增)时使用syscnt时间差计算totalTime：
 *   overrideTotalTime = (endSyscnt - startSyscnt) / hwtsFrequency = (200 - 100) / 100 = 1.0
 */
TEST_F(PmuAssociationUTest, ShouldUseSyscntTimeWhenV6TimeListValid)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    auto res = dynamic_cast<PmuInfoSingleAccelerator *>(result->begin()->second.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_DOUBLE_EQ(1.0, res->totalTime);
}

/*
 * V6芯片非mix场景：timeList回绕(endSyscnt < startSyscnt)时不使用syscnt计算，
 *   totalTime回退为周期公式：totalCycle * FREQ_TO_Hz / freq / blockNum * ((blockNum + coreNum - 1) / coreNum)
 *   = 100 * 1e6 / (100 * 1e6) / 8 * ((8 + 8 - 1) / 8) = 0.125
 */
TEST_F(PmuAssociationUTest, ShouldFallBackToCycleFormulaWhenV6TimeListWarp)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(200, 100);  // endSyscnt < startSyscnt，模拟时钟回绕
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    auto res = dynamic_cast<PmuInfoSingleAccelerator *>(result->begin()->second.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_DOUBLE_EQ(0.125, res->totalTime);
}

/*
 * V6芯片mix场景：task仅收到AIC单侧context PMU（AIV侧丢失）时，
 *   关联仍成功，结果仅含AIC侧数据并打印WARN日志
 */
TEST_F(PmuAssociationUTest, ShouldOnlyFillAicResultWhenV6MixTaskReceiveSinglePmu)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    pmuData[0].pmu.acceleratorType = MIX_AIC;  // 仅收到MIX_AIC一侧的PMU
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    const auto& task = result->begin()->second.at(0);
    EXPECT_EQ(MIX_AIC, task.acceleratorType);
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(task.pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_FALSE(res->aicPmuResult.empty());
    EXPECT_TRUE(res->aivPmuResult.empty());
}

/*
 * V6芯片mix场景：同一task先后收到AIC、AIV两条context PMU（静态图下AIC/AIV各上报一条）。验证：
 *   1) MergeContextPmuToDeviceTask 的taskIndex推进：第二条PMU仍落在当前task窗口内，taskIndex不推进，
 *      两条PMU都归属同一task；
 *   2) CalculateContextPmu 的existingMix合并：第二条PMU不覆盖第一条，aic/aiv结果同时存在。
 */
TEST_F(PmuAssociationUTest, ShouldMergeAicAndAivPmuToSameTaskWhenV6Mix)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    pmuData[0].pmu.acceleratorType = MIX_AIC;
    pmuData[0].hd.timestamp = 100;
    HalPmuData aivPmu = pmuData[0];
    aivPmu.pmu.acceleratorType = MIX_AIV;
    aivPmu.hd.timestamp = 120;  // AIV侧PMU略晚，但与AIC同处task[50,150]窗口
    pmuData.push_back(aivPmu);
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    ASSERT_EQ(1ul, result->size());
    const auto& tasks = result->begin()->second;
    ASSERT_EQ(1ul, tasks.size());
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(tasks.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    // existingMix合并成功：AIC与AIV结果同时存在，互不覆盖
    EXPECT_FALSE(res->aicPmuResult.empty());
    EXPECT_FALSE(res->aivPmuResult.empty());
    EXPECT_EQ(100ul, res->aicTotalCycles);
    EXPECT_EQ(100ul, res->aivTotalCycles);
    // 本用例两条PMU均为从核(mst=0)：MIX_AIC从核写aivTime(周期公式0.125)，MIX_AIV从核将aiCoreTime归零
    EXPECT_DOUBLE_EQ(0.0, res->aiCoreTime);
    EXPECT_DOUBLE_EQ(0.125, res->aivTime);
}

/*
 * V6芯片mix场景：主核(mst=1)使用syscnt时间差计算时间。
 * MIX_AIC主核：aiCoreTime = (endSyscnt - startSyscnt) / hwtsFrequency = (200 - 100) / 100 = 1.0
 */
TEST_F(PmuAssociationUTest, ShouldUseSyscntTimeForAicMainCoreWhenV6Mix)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    pmuData[0].pmu.acceleratorType = MIX_AIC;
    pmuData[0].pmu.mst = 1;  // AIC为主核
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(result->begin()->second.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_DOUBLE_EQ(1.0, res->aiCoreTime);
    EXPECT_DOUBLE_EQ(0.0, res->aivTime);
}

/*
 * V6芯片mix场景：MIX_AIV主核(mst=1)使用syscnt时间差计算时间。
 * aivTime = (200 - 100) / 100 = 1.0
 */
TEST_F(PmuAssociationUTest, ShouldUseSyscntTimeForAivMainCoreWhenV6Mix)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    pmuData[0].pmu.acceleratorType = MIX_AIV;
    pmuData[0].pmu.mst = 1;  // AIV为主核
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(result->begin()->second.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_DOUBLE_EQ(0.0, res->aiCoreTime);
    EXPECT_DOUBLE_EQ(1.0, res->aivTime);
}

/*
 * V6芯片mix场景：从核(mst=0)不使用syscnt时间差，回退为周期公式。
 * MIX_AIC从核写aivTime = totalCycle * FREQ_TO_Hz / freq / blockNum * ((blockNum + coreNum - 1) / coreNum)
 *   = 100 * 1e6 / (100 * 1e6) / 8 * ((8 + 8 - 1) / 8) = 0.125
 */
TEST_F(PmuAssociationUTest, ShouldUseCycleFormulaForSlaveCoreWhenV6Mix)
{
    PmuAssociation pmuAssociation;
    DeviceContext context;
    SetV6DeviceContext(context);
    auto pmuData = GenerateV6HalPmuData(100, 200);
    pmuData[0].pmu.acceleratorType = MIX_AIC;
    pmuData[0].pmu.mst = 0;  // AIC为从核
    auto pmuDataS = dataInventory_.GetPtr<std::vector<HalPmuData>>();
    pmuDataS->swap(pmuData);
    auto deviceTask = GenerateV6DeviceTask(UINT16_MAX);
    auto deviceTaskS = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    deviceTaskS->swap(deviceTask);
    ASSERT_EQ(ANALYSIS_OK, pmuAssociation.Run(dataInventory_, context));
    auto result = dataInventory_.GetPtr<std::map<TaskId, std::vector<Domain::DeviceTask>>>();
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(result->begin()->second.at(0).pmuInfo.get());
    ASSERT_NE(nullptr, res);
    EXPECT_DOUBLE_EQ(0.0, res->aiCoreTime);
    EXPECT_DOUBLE_EQ(0.125, res->aivTime);
}
}
}