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
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include "gtest/gtest.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"
#include "analysis/csrc/domain/services/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/domain/services/association/cann/include/tree_builder.h"
#include "analysis/csrc/domain/services/association/cann/include/tree_analyzer.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"

using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Domain::Cann;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

const std::string TYPE_INFO_FILE = "./aging.additional.type_info_dic.slice_0";
const std::string HASH_INFO_FILE = "./unaging.additional.hash_dic.slice_0";

class TreeAnalyzerUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        // 创建typeInfo文件
        FileWriter typeInfo(TYPE_INFO_FILE);
        typeInfo.WriteText("5000_2:EVENT_RECORD\n");    // Runtime level other
        typeInfo.WriteText("5000_52:FFTS_PLUS\n");      // Runtime level ffts+
        typeInfo.WriteText("5000_50:STARS_COMMON\n");   // Runtime level stars common
        typeInfo.WriteText("5000_5:MEMCPY_ASYNC\n");    // Runtime level memcpy
        typeInfo.WriteText("5000_0:KERNEL_AICORE\n");   // Runtime level aicore
        typeInfo.WriteText("5000_1:KERNEL_AICPU\n");   // Runtime level aicpu
        typeInfo.WriteText("5000_66:KERNEL_AIVEC\n");   // Runtime level aiv
        typeInfo.WriteText("5000_108:KERNEL_MIX_AIC\n");   // Runtime level mix_aic
        typeInfo.WriteText("5000_109:KERNEL_MIX_AIV\n");   // Runtime level mix_aiv
        typeInfo.WriteText("5000_1045:NotifyRecord\n"); // Runtime level hccl
        typeInfo.Close();

        // 创建hashInfo文件
        FileWriter hashInfo(HASH_INFO_FILE);
        hashInfo.WriteText("11111:LcclAllReduce\n");    // LcclAllReduce
        hashInfo.WriteText("22222:Matmul\n");           // Matmul
        hashInfo.WriteText("33333:hcom_allReduce_\n");  // hcom_allReduce_
        hashInfo.WriteText("44444:NA\n");               // NA
        hashInfo.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile(TYPE_INFO_FILE);  // 删除typeInfo文件
        File::DeleteFile(HASH_INFO_FILE);  // 删除hashInfo文件
    }
};

struct HostTaskCompCtx {
    bool operator()(std::shared_ptr<HostTask> &t1, std::shared_ptr<HostTask> &t2)
    {
        return (t1->contextId < t2->contextId);
    }
};

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueue(std::unordered_map<
    std::string, std::vector<std::pair<uint64_t, uint64_t>>> &apiEvents)
{
    auto kernelEvents = std::make_shared<EventQueue>(1, 100);
    std::vector<std::string> levels{"Model", "Node", "Communication"};
    std::unordered_map<std::string, uint16_t> levelNum{
        {"Model", MSPROF_REPORT_MODEL_LEVEL},
        {"Node", MSPROF_REPORT_NODE_LEVEL},
        {"Communication", MSPROF_REPORT_HCCL_NODE_LEVEL},
    };
    for (const auto &le: levels) {
        for (auto range: apiEvents[le]) {
            if (le == "Model") {
                FakeEventGenerator::AddApiEvent(kernelEvents, levelNum[le], range.first, range.second,
                                                range.second, range.second);
            } else {
                FakeEventGenerator::AddApiEvent(kernelEvents, levelNum[le], range.first, range.second);
            }
        }
    }
    kernelEvents->Sort();
    return kernelEvents;
}

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueueWithTradComm()
{
    // Model :  [1,          200][201,                                                        400][401,  800]
    // Node  :  [2,99]  [100,150][202,                                250] [251,  301] [329,351]   [410,500][1020, 1160]
    // Node:                      [204,208]          [221,223]
    // Hccl  :                             [210,220]           [230, 250]  [252, 300] [330,350]             [1020, 1160]
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Model", {{1, 200}, {201, 400}, {401, 800}}},
        {"Node", {{2, 99}, {100, 150}, {202, 250}, {204, 208}, {221, 223}, {251, 301}, {329, 351}, {410, 500},
                  {1020, 1160}}},
        {"Communication", {{210, 220}, {230, 250}, {252, 300}, {330, 350}, {1020, 1160}}},
    };

    return GenKernelEventQueue(apiEvents);
}

// 生成建树所需的四层节点
std::shared_ptr<EventQueue> GenKernelEventQueueWithFFtsComm()
{
    // Acl   :  [1，                                               800]  [801,  900]  [1000,    1200]
    // Model :  [1,            200]  [201,             400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,  300] [302, 399]                            [1020, 1160]
    // Hccl  :                       [202,  300] [302, 399]                            [1020, 1160]
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Acl", {{1, 800}, {801, 900}, {1000, 1200}}},
        {"Model", {{1, 200}, {201, 400}, {401, 800}}},
        {"Node", {{2, 99}, {100, 150}, {160, 195}, {202, 300}, {302, 399}, {1020, 1160}}},
        {"Communication", {{202, 300}, {302, 399}, {1020, 1160}}},
    };

    return GenKernelEventQueue(apiEvents);
}

std::shared_ptr<EventQueue> GenHcclInfoEventQueue(const std::vector<uint64_t> &dots,
                                                  const std::vector<uint64_t> &ctxIds)
{
    auto hcclInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (uint32_t i = 0; i < dots.size(); i++) {
        FakeEventGenerator::AddHcclInfoEvent(hcclInfoEvents, dots[i], ctxIds[i]);
    }
    hcclInfoEvents->Sort();
    return hcclInfoEvents;
}

// OpDesc represents <dot, op_type, task_type>
std::shared_ptr<EventQueue> GenNodeBasicInfoEventQueue(const std::vector<std::tuple<uint64_t,
                                                                                    uint64_t,
                                                                                    uint32_t>> OpDescs)
{
    auto nodeBasicInfoEvents = std::make_shared<EventQueue>(1, 10);
    const uint16_t dotIndex = 0;
    const uint16_t opTypeIndex = 1;
    const uint16_t taskTypeIndex = 2;
    for (auto &t: OpDescs) {
        FakeEventGenerator::AddNodeBasicEvent(nodeBasicInfoEvents, std::get<dotIndex>(t),
                                              std::get<opTypeIndex>(t), std::get<taskTypeIndex>(t));
    }
    nodeBasicInfoEvents->Sort();
    return nodeBasicInfoEvents;
}

// OpDesc represents <dot, hashId>
std::shared_ptr<EventQueue> GenNodeAttrInfoEventQueue(const std::vector<std::tuple<uint64_t, uint64_t>> OpDescs)
{
    auto nodeAttrInfoEvents = std::make_shared<EventQueue>(1, 10);
    const uint16_t dotIndex = 0;
    const uint16_t hashIdIndex = 1;
    for (auto &t: OpDescs) {
        FakeEventGenerator::AddNodeAttrEvent(nodeAttrInfoEvents, std::get<dotIndex>(t), std::get<hashIdIndex>(t));
    }
    nodeAttrInfoEvents->Sort();
    return nodeAttrInfoEvents;
}

// tensorDesc represents dot
std::shared_ptr<EventQueue> GenTensorInfoEventQueue(const std::vector<uint64_t> &tensorDescs)
{
    auto tensorInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (auto dot: tensorDescs) {
        FakeEventGenerator::AddTensorInfoEvent(tensorInfoEvents, dot);
    }

    tensorInfoEvents->Sort();
    return tensorInfoEvents;
}

std::shared_ptr<EventQueue> GenHcclCtxIdEventQueue()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 10);
    std::vector<uint64_t> dots = {230, 310};
    uint32_t endCtxId = 2;
    for (auto &dot :dots) {
        FakeEventGenerator::AddHcclCtxIdEvent(contextIdEvents, dot, endCtxId);
    }
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenNodeCtxIdEventQueue()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 5);
    std::vector<uint64_t> dots = {125, 126, 127, 128};
    for (uint32_t i = 0; i < dots.size(); i++) {
        auto dot = dots[i];
        FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, dot, {i * 2, i * 2 + 1});
    }
    contextIdEvents->Sort();
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenTaskTrackEventQueue(const std::vector<std::pair<uint64_t, uint64_t>> &runtimeDescs)
{
    auto taskTrackEvents = std::make_shared<EventQueue>(1, runtimeDescs.size());
    for (auto item : runtimeDescs) {
        FakeEventGenerator::AddTaskTrackEvent(taskTrackEvents, item.first, item.second);
    }
    taskTrackEvents->Sort();
    return taskTrackEvents;
}

std::shared_ptr<Event> GenCtxIdEvent(uint64_t dot, uint16_t level, uint32_t ctxIdNum)
{
    EventInfo testInfo{EventType::EVENT_TYPE_CONTEXT_ID, level, dot, dot};
    auto additionInfo = std::make_shared<MsprofAdditionalInfo>();
    uint32_t dataLen = 2;
    additionInfo->timeStamp = dot;
    additionInfo->dataLen = dataLen;

    MsprofContextIdInfo ctxId;
    ctxId.opName = dot;
    ctxId.ctxIdNum = ctxIdNum;
    uint32_t ids[2] = {0, 1};
    std::memcpy(ctxId.ctxIds, &ids, sizeof(ids));
    std::memcpy(additionInfo->data, &ctxId, sizeof(ctxId));

    auto eventPtr = std::make_shared<Event>(additionInfo, testInfo);
    return eventPtr;
}

std::shared_ptr<Event> GenHcclInfoEvent(uint64_t dot)
{
    uint32_t cnt = 0;
    EventInfo testInfo{EventType::EVENT_TYPE_HCCL_INFO, MSPROF_REPORT_HCCL_NODE_LEVEL, dot, dot};
    auto hcclInfo = std::make_shared<MsprofAdditionalInfo>();
    hcclInfo->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    hcclInfo->level = MSPROF_REPORT_HCCL_NODE_LEVEL;
    hcclInfo->type = static_cast<uint32_t>(EventType::EVENT_TYPE_HCCL_INFO);
    hcclInfo->timeStamp = static_cast<uint64_t>(dot);

    auto hcclTrace = MsprofHcclInfo{};
    hcclTrace.ctxID = cnt;
    hcclTrace.itemId = dot;
    std::memcpy(hcclInfo->data, &hcclTrace, sizeof(hcclTrace));
    auto eventPtr = std::make_shared<Event>(hcclInfo, testInfo);

    return eventPtr;
}

std::shared_ptr<EventQueue> GenHcclOpInfoEventQueue(const std::vector<uint64_t> &timeStamps,
                                                    const std::vector<std::vector<uint16_t>> &algList)
{
    auto hcclOpInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (size_t i = 0; i < timeStamps.size(); ++i) {
        FakeEventGenerator::AddHcclOpEvent(hcclOpInfoEvents, timeStamps[i], algList[i]);
    }
    hcclOpInfoEvents->Sort();
    return hcclOpInfoEvents;
}

std::shared_ptr<EventQueue> GenFusionOpEventQueue(const std::vector<uint64_t> &fusionOps)
{
    auto fusionOpInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (auto dot: fusionOps) {
        auto additionPtr = std::make_shared<MsprofAdditionalInfo>();
        FakeEventGenerator::AddFusionOpEvent(fusionOpInfoEvents, dot, additionPtr);
    }
    fusionOpInfoEvents->Sort();
    return fusionOpInfoEvents;
}

TEST_F(TreeAnalyzerUTest, TestUpdateHcclSmallOpDescsShouldReturn2DescsWhenInputCtxIdLenIs2)
{
    uint64_t dot = 123;
    uint64_t ctxIdNum = 2;
    HCCLSmallOpDescs descs;
    std::vector<std::shared_ptr<Event>> ctxIdRecords{
        GenCtxIdEvent(dot, MSPROF_REPORT_NODE_LEVEL, ctxIdNum),
    };

    std::vector<std::shared_ptr<Event>> hcclInfoRecords{
        GenHcclInfoEvent(dot)
    };

    auto treeNode = std::make_shared<TreeNode>(nullptr);
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);

    auto ret = ana->UpdateHcclSmallOpDescs(descs, ctxIdRecords, hcclInfoRecords, 0);

    std::vector<uint32_t> expectCtxIds{0, 1, DEFAULT_CONTEXT_ID};
    EXPECT_EQ(ret, true);
    EXPECT_EQ(descs.size(), expectCtxIds.size());

    // 检查ctxIds
    for (auto id : expectCtxIds) {
        auto check = descs.count(id) != 0;
        EXPECT_EQ(check, true);
    }
}

// 测试输入为空指针
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerShouldReturnZeroWhenInputNullptr)
{
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    EXPECT_EQ(computeTasks.size(), 0);
    EXPECT_EQ(hcclTasks.size(), 0);
    EXPECT_EQ(bigOPs.size(), 0);
    EXPECT_EQ(tasks.size(), 0);
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForNoKernelEventsScenario()
{
    // 逻辑树如下
    // Runtime:  tk(50)    tk(110)     tk(220)    tk(240)
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},     // Runtime OtherTask
        {110, 52},   // Runtime FFTS_PLUS
        {220, 1045}, // Runtime NOTIFY_RECORD
        {240, 1045}, // Runtime NOTIFY_RECORD
        {300, 0}, // Runtime KERNEL_AICORE
        {310, 1}, // Runtime KERNEL_AICPU
        {330, 66}, // Runtime KERNEL_AIVEC
        {340, 108}, // Runtime KERNEL_MIX_AIC
        {350, 109}, // Runtime KERNEL_MIX_AIV
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->taskTrackEvents = taskTrackEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    HashData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 特殊测试场景1：只有tk数据
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerForNoKernelEventsScenario)
{
    auto ana = GetAnalyzerForNoKernelEventsScenario();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::vector<uint64_t> expectComputeTaskTimes{300, 310, 330, 340, 350};
    std::vector<uint64_t> expectHcclTaskTimes;
    // 110中多一条是ffts+大任务的标记
    std::vector<uint64_t> expectTaskTimes{50, 110, 220, 240, 300, 310, 330, 340, 350};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    // 先检查个数正确
    ASSERT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    ASSERT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    ASSERT_EQ(tasks.size(), expectTaskTimes.size());
    ASSERT_EQ(tasks.size(), expectTaskCtxIds.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }

    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario1L0()
{
    // 逻辑树如下，其中[n]标识每个context id event中带几个context id
    // Model :  [1,            200]  [201,                                                  400][401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,                               250][251,301][329,351] [410,500]   [1020, 1160]
    //                  c(125,126)[2]
    //                  c(127,128)[2]
    // Node  :                       [204, 208]        [221, 223]
    //                                         hop(210)                      hop(253)   hop(330)
    // Hccl  :                                 [210, 220]         [230, 250] [252,300][330,350]             [1020, 1160]
    // Runtime:  tk(50)    tk(110)    tk(205)   tk(220)tk(222)tk(225)tk(240) tk(260) tk(340) tk(430) tk(880) tk(1050)
    auto kernelEvents = GenKernelEventQueueWithTradComm();
    auto contextIdEvents = GenNodeCtxIdEventQueue();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},     // Runtime OtherTask
        {110, 52},   // Runtime FFTS_PLUS
        {205, 1},    // Runtime KERNEL_AICPU, hccl aicpu场景, hcomInitAicpu
        {220, 1045}, // Runtime NOTIFY_RECORD
        {222, 1},    // Runtime KERNEL_AICPU, hccl aicpu场景
        {225, 1},    // Runtime KERNEL_AICPU, helper场景
        {240, 1045}, // Runtime NOTIFY_RECORD
        {260, 0},    // Runtime KERNEL_AICORE
        {340, 1},    // Runtime KERNEL_AICPU
        {430, 1},    // Runtime KERNEL_AICPU 注意： 非计算类算子
        {880, 1},    // Runtime KERNEL_AICPU 注意： 非计算类算子
        {1050, 66},   // Runtime KERNEL_AIVEC lccl 纯通信算子
        {1300, 108},   // Runtime KERNEL_MIX_AIC 裸rts_track
        {1320, 109},   // Runtime KERNEL_MIX_AIV 裸rts_track
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);
    std::vector<std::vector<uint16_t>> algList{
        {AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_MESH, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_RING},
        {AlgType::HCCL_ALG_NHR, AlgType::HCCL_ALG_STAR, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_HD},
        {AlgType::HCCL_ALG_MESH, AlgType::HCCL_ALG_RING, AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_PIPELINE},
        {AlgType::HCCL_ALG_NONE},
    };
    auto hcclOpInfoEvents = GenHcclOpInfoEventQueue({210, 253, 330, 1040}, algList);
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->hcclOpInfoEvents = hcclOpInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    HashData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景1 L0 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario1L0)
{
    auto ana = GetAnalyzerForScenario1L0();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());

    std::vector<uint64_t> expectComputeTaskTimes{110, 1320, 1300, 110, 110, 110, 110, 110, 110, 110, 225,
                                                 260, 340, 430, 1050, 880, 222, 205};
    std::vector<uint64_t> expectComputeTaskCtxIds{
        0, 0, 0, 1, 2, 3, 4, 5, 6, 7, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
        DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID
    };
    std::vector<uint64_t> expectHcclTaskTimes{220, 240, 260, 340, 1050};
    // 110中多一条是ffts+大任务的标记
    // 1050 在 880 前面,是因为建树时候task添加流程是 compute - hccl - others,
    std::vector<uint64_t> expectTaskTimes{50, 110, 110, 110, 110, 110, 110, 110, 110, 110,
                                          205, 220, 222, 240, 225, 260, 340, 430, 1050, 880, 1300, 1320};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 1, 2, 3, 4, 5, 6, 7, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID};

    std::vector<uint64_t> expectModelIds{200, 200, 200, 200, 200, 200, 200, 200, 200, 200,
                                         400, 400, 400, 400, 400, 400, 400, 800,
                                         UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    std::vector<int64_t> expectRequestIds{200, 200, 200, 200, 200, 200, 200, 200, 200, 200,
                                          400, 400, 400, 400, 400, 400, 400, 800, -1, -1, -1, -1};
    // 特殊场景校验分别代表AI_CORE和HCCL_AICPU
    std::string expectHcclComputeTaskTaskTypeStr = "null null null null null null null null null null "
                                                   "null 9 256 null 0 null null null ";
    const uint64_t expectbigOPsNum = 4;
    std::vector<uint64_t> expectbigOPsCount{420, 506, 660, 2080};
    std::vector<std::string> expectKfcConnectionIds{"28,29", "9223372036854775807", "9223372036854775807",
                                                    "9223372036854775807"};
    std::vector<std::string> expectbigOPsAlgType{"HD-MESH-NB-RING", "NHR-STAR-NB-HD", "MESH-RING-HD-PIPELINE", "NA"};
    // 先检查个数正确
    ASSERT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    ASSERT_EQ(bigOPs.size(), expectbigOPsNum);

    // 再匹配内容正确
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }

    std::string taskTypeStr;
    for (auto task : computeTasks) {
        if (task->op && task->op->opDesc && task->op->opDesc->nodeDesc) {
            taskTypeStr += std::to_string(task->op->opDesc->nodeDesc->data.nodeBasicInfo.taskType) + " ";
        } else {
            taskTypeStr += "null ";
        }
    }

    for (uint16_t i = 0; i < bigOPs.size(); i++) {
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->opInfoDesc->data.hcclopInfo.count, expectbigOPsCount[i]);
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->kfcConnectionIds, expectKfcConnectionIds[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario1L2()
{
// 逻辑树如下，其中c-n-a-t表示ctxId, nodeBasicInfo, nodeAttrInfo, tensorInfo,
// 【d】表示异常情况，[n]标识每个context id event中带几个context id
// ng表示图的nodeBasic数据
// tk(260)    tk(340)为aicpu的hccl和aicore的hccl（reduce tbe）任务
// Model :  [1,            200]  [201,                                                       400][401,   800]
// Node  :  [2, 99]  [100, 150]  [202,                               250] [251,  301] [329, 351] [410, 500][1020, 1160]
//                 c-n-a-t(125)[2]                         n(224)          n(255)        n(340)    n(430)
//                 c-n-a-t(126)[2]
//                 c-n-a-t(127)[2]
//                 c-n-a-t(128)[2]
//                    ng(129)
//                   t(144【d】)
// Node:                          [204,208]         [221, 223]
//                                         hop(211)                         hop(252)  hop(330)
// Hccl  :                                 [210, 220]          [230, 250]   [252, 300][330, 350]           [1020, 1160]
//                                         hI(211)             hI(231)        hI(253)   hI(331)
// Runtime:  tk(50)    tk(110)     tk(205)  tk(220) tk(222) tK(225) tk(240)    tk(260)   tk(340)    tk(430)  tk(1050)
    auto kernelEvents = GenKernelEventQueueWithTradComm();
    auto hcclInfoEvents = GenHcclInfoEventQueue({211, 231, 253, 331}, {DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID});
    auto nodeBasicInfoEvents = GenNodeBasicInfoEventQueue({{125, 1, 0}, {126, 2, 0}, {127, 1, 1},
                                                           {128, 0, 1}, {129, 100, 6}, {224, 6, 9},
                                                           {255, 7, 9}, {340, 8, 9}, {430, 8, 9}, {1050, 8, 9}});
    auto nodeAttrInfoEvents = GenNodeAttrInfoEventQueue({{125, 1}, {126, 2}, {127, 1}, {128, 0}, {129, 100},
                                                         {224, 6}, {255, 7}, {340, 8}, {430, 8}, {1050, 8}});
    auto tensorInfoEvents = GenTensorInfoEventQueue({125, 126, 127, 128, 144});
    auto contextIdEvents = GenNodeCtxIdEventQueue();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},     // Runtime OtherTask
        {110, 52},   // Runtime FFTS_PLUS
        {205, 1},    // Runtime KERNEL_AICPU, hccl aicpu场景, hcomInitAicpu
        {220, 1045}, // Runtime NOTIFY_RECORD
        {222, 1},    // Runtime KERNEL_AICPU, hccl aicpu场景
        {225, 1},    // Runtime KERNEL_AICPU, helper场景
        {240, 1045}, // Runtime NOTIFY_RECORD
        {260, 0},    // Runtime KERNEL_AICORE
        {340, 1},    // Runtime KERNEL_AICPU
        {430, 1},    // Runtime KERNEL_AICPU
        {1050, 66},   // Runtime KERNEL_AIVEC lccl 纯通信算子
        {1300, 108},   // Runtime KERNEL_MIX_AIC 裸rts_track
        {1320, 109},   // Runtime KERNEL_MIX_AIV 裸rts_track
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);
    std::vector<std::vector<uint16_t>> algList{
        {AlgType::HCCL_ALG_PAIRWISE, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_RING},
        {AlgType::HCCL_ALG_STAR, AlgType::HCCL_ALG_NHR, AlgType::HCCL_ALG_PIPELINE, AlgType::HCCL_ALG_NB},
        {AlgType::HCCL_ALG_MESH, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_STAR},
        {AlgType::HCCL_ALG_NONE},
    };
    auto hcclOpInfoEvents = GenHcclOpInfoEventQueue({211, 252, 330, 1040}, algList);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->nodeAttrInfoEvents = nodeAttrInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;
    cannWarehouse->hcclOpInfoEvents = hcclOpInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    HashData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景1 L2 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
// 相比L1只增加了nodeAttrInfo，其余信息不变，所以L2场景的用例可以覆盖L1
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario1L2)
{
    auto ana = GetAnalyzerForScenario1L2();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());

    std::vector<uint64_t> expectComputeTaskTimes{1320, 1300, 110, 110, 110, 110, 110, 110, 110, 110, 205,
                                                 222, 225, 260, 340, 430, 1050, 110};
    std::vector<uint64_t> expectComputeTaskCtxIds{0, 0, 0, 1, 2, 3, 4, 5, 6, 7,
                                                  DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                                  DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                                  DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    std::vector<uint64_t> expectHcclTaskTimes{220, 240, 260, 340, 1050};
    // 110中多一条是ffts+大任务的标记
    std::vector<uint64_t> expectTaskTimes{50, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 205, 220,
                                          222, 240, 225, 260, 340, 430, 1050, 1300, 1320};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 1, 2, 3, 4, 5, 6, 7, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID};
    // 特殊场景校验分别代表AI_CORE，AI_CPU和HCCL_AICPU
    std::string expectHcclComputeTaskTaskTypeStr = "null null 0 0 0 0 1 1 1 1 null null 1 9 256 1 9 null ";
    const uint64_t expectbigOPsNum = 4;
    std::vector<uint64_t> expectbigOPsCount{422, 504, 660, 2080};
    std::vector<std::string> expectKfcConnectionIds{"82,83", "9223372036854775807", "9223372036854775807",
                                                    "9223372036854775807"};
    std::vector<std::string> expectbigOPsAlgType{"PAIRWISE-NB-HD-RING", "STAR-NHR-PIPELINE-NB", "MESH-NB-HD-STAR",
                                                 "NA"};
    std::string expectOpTypeStr = "null null 1 1 2 2 1 1 0 0 null null 6 7 8 8 8 null ";
    std::string expectHashIdStr = "null null 1 1 2 2 1 1 0 0 null null 6 7 8 8 8 null ";
    std::string expectTensorStr = "null null 125 125 126 126 127 127 128 128 null null null null null null null 144 ";
    // 先检查个数正确
    ASSERT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    ASSERT_EQ(bigOPs.size(), expectbigOPsNum);

    for (uint16_t i = 0; i < bigOPs.size(); i++) {
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->opInfoDesc->data.hcclopInfo.count, expectbigOPsCount[i]);
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->kfcConnectionIds, expectKfcConnectionIds[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario2L0()
{
    // 逻辑树如下，其中[n]标识每个context id event中带几个context id
    // 其中tk(110),tk(120),tk(168)为计算任务
    // Model :  [1,                          200]  [201,             400]  [401, 800]
    //                    f(125)                                               f(600)
    // Node  :  [2, 99]  [100, 150]   [160, 195]   [202,  300] [302, 399]
    //                                 c(171)[1]
    //                                               hop(205)   hop(305)
    // Hccl  :                                     [202,  300] [302, 399]
    //                                              c(230)[3]   c(310)[3]
    // Runtime:  tk(50)   tk(105)       tk(165)      tk(220)     tk(330)
    //                    tk(110)       tk(168)
    //                    tk(120)
    auto kernelEvents = GenKernelEventQueueWithFFtsComm();
    auto contextIdEvents = GenHcclCtxIdEventQueue();
    uint32_t nodeCtxDot = 171;
    FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, nodeCtxDot, {0, 0});
    contextIdEvents->Sort();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},   // Runtime OtherTask
        {105, 5},  // Runtime MEMCPY_ASYNC
        {110, 50}, // Runtime STARS_COMMON
        {120, 0},  // Runtime KERNEL_AICORE
        {165, 5},  // Runtime MEMCPY_ASYNC
        {168, 52}, // Runtime FFTS_PLUS
        {220, 52}, // Runtime FFTS_PLUS
        {330, 52}, // Runtime FFTS_PLUS
        {1300, 108},   // Runtime KERNEL_MIX_AIC 裸rts_track
        {1320, 109},   // Runtime KERNEL_MIX_AIV 裸rts_track
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);
    std::vector<std::vector<uint16_t>> algList{
        {AlgType::HCCL_ALG_PAIRWISE, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_RING},
        {AlgType::HCCL_ALG_STAR, AlgType::HCCL_ALG_NHR, AlgType::HCCL_ALG_PIPELINE, AlgType::HCCL_ALG_NB}
    };
    auto hcclOpInfoEvents = GenHcclOpInfoEventQueue({205, 305}, algList);
    auto fusionOpInfoEvents =  GenFusionOpEventQueue({125, 600});

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->hcclOpInfoEvents = hcclOpInfoEvents;
    cannWarehouse->fusionOpInfoEvents = fusionOpInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    HashData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景2 L0 : 计算类算子传统模式+hccl算子ffts+模式（Pytorch训练）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario2L0)
{
    auto ana = GetAnalyzerForScenario2L0();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();
    auto geFusionOpInfos = ana->GetGeFusionOpInfos();

    std::vector<uint64_t> expectComputeTaskTimes{110, 120, 168, 1300, 1320};
    std::vector<uint64_t> expectComputeTaskCtxIds{DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, 0, 0, 0};
    std::vector<uint64_t> expectHcclTaskTimes{220, 220, 220, 330, 330, 330};
    // 多出的一个220，一个330分别标识ffts+大任务, 多出的一个168标识MIX算子
    std::vector<uint64_t> expectTaskTimes{50, 105, 110, 120, 165, 168, 168, 220, 220, 220, 220,
                                          330, 330, 330, 330, 1300, 1320};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 0, 0, 1, 1, 2, 2, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    const uint64_t expectbigOPsNum = 2;
    std::vector<uint64_t> expectbigOPsCount{410, 610};
    std::vector<std::string> expectbigOPsAlgType{"PAIRWISE-NB-HD-RING", "STAR-NHR-PIPELINE-NB"};
    std::vector<uint64_t> expectGeFusionOpInfoTimes{125, 600};
    // 先检查个数正确
    ASSERT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    ASSERT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    ASSERT_EQ(bigOPs.size(), expectbigOPsNum);
    ASSERT_EQ(tasks.size(), expectTaskTimes.size());
    ASSERT_EQ(geFusionOpInfos.size(), expectGeFusionOpInfoTimes.size());
    ASSERT_EQ(tasks.size(), expectTaskCtxIds.size());
    // 再匹配内容正确
    uint64_t modelId1 = 200;
    uint64_t modelId2 = 800;
    EXPECT_EQ(geFusionOpInfos[0]->modelId, modelId1);
    EXPECT_EQ(geFusionOpInfos[1]->modelId, modelId2);
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }

    for (uint16_t i = 0; i < bigOPs.size(); i++) {
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->opInfoDesc->data.hcclopInfo.count, expectbigOPsCount[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario2L2()
{
    // 逻辑树如下，其中[n]标识每个event中带几个context id
    // 其中tk(110),tk(120),tk(168)为计算任务
    // Acl:     [1,                                                               800] [801, 900]
    // Model :  [1,                          200]  [201,              400]  [401, 800]
    //                    f(125)                                               f(600)
    // Node  :  [2, 99]  [100, 150]   [160, 195]   [202,  300] [302,  399]
    //                   n-a-t(125)   c-n-a-t(171)   n-a(250)    n-a(310)
    //                                               hop(205)    hop(305)
    // Hccl  :                                     [202,  300] [302,  399]
    //                                               c(230)[3]  c(310)[3]
    //                                            hInfo(230)[3] hInfo(310)[3]
    // Runtime:  tk(50)   tk(105)       tk(165)       tk(220)     tk(330)                tk(850)
    //                    tk(110)       tk(168)
    //                    tk(120)
    auto kernelEvents = GenKernelEventQueueWithFFtsComm();
    auto hcclInfoEvents = GenHcclInfoEventQueue({230, 230, 230, 310, 310, 310}, {0, 1, 2, 0, 1, 2});
    auto nodeBasicInfoEvents = GenNodeBasicInfoEventQueue({{125, 1, 1}, {171, 3, 2}, {250, 333, 9}, {310, 334, 9}});
    auto nodeAttrInfoEvents = GenNodeAttrInfoEventQueue({{125, 1}, {171, 3}, {250, 333}, {310, 334}});
    auto tensorInfoEvents = GenTensorInfoEventQueue({125, 171});
    auto contextIdEvents = GenHcclCtxIdEventQueue();
    auto fusionOpInfoEvents =  GenFusionOpEventQueue({125, 600});
    uint32_t nodeCtxDot = 171;
    FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, nodeCtxDot, {0, 0});
    contextIdEvents->Sort();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},   // Runtime OtherTask
        {105, 5},  // Runtime MEMCPY_ASYNC
        {110, 50}, // Runtime STARS_COMMON
        {120, 0},  // Runtime KERNEL_AICORE
        {165, 5},  // Runtime MEMCPY_ASYNC
        {168, 52}, // Runtime FFTS_PLUS
        {220, 52}, // Runtime FFTS_PLUS
        {330, 52}, // Runtime FFTS_PLUS
        {850, 5},  // Runtime MEMCPY_ASYNC
        {1300, 108},   // Runtime KERNEL_MIX_AIC 裸rts_track
        {1320, 109},   // Runtime KERNEL_MIX_AIV 裸rts_track
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);
    std::vector<std::vector<uint16_t>> algList{
        {AlgType::HCCL_ALG_PAIRWISE, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_RING},
        {AlgType::HCCL_ALG_STAR, AlgType::HCCL_ALG_NHR, AlgType::HCCL_ALG_PIPELINE, AlgType::HCCL_ALG_NB}
    };
    auto hcclOpInfoEvents = GenHcclOpInfoEventQueue({205, 305}, algList);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->nodeAttrInfoEvents = nodeAttrInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;
    cannWarehouse->hcclOpInfoEvents = hcclOpInfoEvents;
    cannWarehouse->fusionOpInfoEvents = fusionOpInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    HashData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景2 L2 : 计算类算子传统模式+hccl算子ffts+模式（Pytorch训练）
// 相比L1只增加了nodeAttrInfo，其余信息不变，所以L2场景的用例可以覆盖L1
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario2L2)
{
    auto ana = GetAnalyzerForScenario2L2();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();
    auto geFusionOpInfos = ana->GetGeFusionOpInfos();

    std::vector<uint64_t> expectComputeTaskTimes{110, 120, 168, 1300, 1320};
    std::vector<uint64_t> expectComputeTaskCtxIds{0, 0, 0, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    std::vector<uint64_t> expectHcclTaskTimes{220, 220, 220, 330, 330, 330};
    std::vector<uint64_t> expectHcclTaskCtxIds{0, 0, 1, 1, 2, 2};
    // 多出的一个220，一个330分别标识ffts+大任务, 多出的一个168标识MIX算子
    std::vector<uint64_t> expectTaskTimes{50, 105, 110, 120, 165, 168, 168, 220, 220, 220, 220, 330, 330, 330, 330,
                                          850, 1300, 1320};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 0, 0, 1, 1, 2, 2,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    const uint64_t expectbigOPsNum = 2;
    std::vector<uint64_t> expectbigOPsCount{410, 610};
    std::vector<std::string> expectbigOPsAlgType{"PAIRWISE-NB-HD-RING", "STAR-NHR-PIPELINE-NB"};
    std::string expectComputeTaskOpTypesStr = "1 1 3 null null ";
    std::string expectComputeTaskHashIdsStr = "1 1 3 null null ";
    std::string expectComputeTaskTensorTimesStr = "125 125 171 null null ";
    std::vector<uint64_t> expectGeFusionOpInfoTimes{125, 600};

    // 先检查个数正确
    ASSERT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    ASSERT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    ASSERT_EQ(bigOPs.size(), expectbigOPsNum);
    ASSERT_EQ(tasks.size(), expectTaskTimes.size());
    ASSERT_EQ(geFusionOpInfos.size(), expectGeFusionOpInfoTimes.size());
    ASSERT_EQ(tasks.size(), expectTaskCtxIds.size());

    // 再匹配内容正确
    uint64_t modelId1 = 200;
    uint64_t modelId2 = 800;
    EXPECT_EQ(geFusionOpInfos[0]->modelId, modelId1);
    EXPECT_EQ(geFusionOpInfos[1]->modelId, modelId2);
    EXPECT_TRUE(geFusionOpInfos[0]->fusionOpInfo != nullptr);
    EXPECT_TRUE(geFusionOpInfos[1]->fusionOpInfo != nullptr);

    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }

    std::string opTypeStr;
    std::string hashIdStr;
    std::string tensorStr;
    for (auto task : computeTasks) {
        if (task->op && task->op->opDesc && task->op->opDesc->nodeDesc) {
            opTypeStr += std::to_string(task->op->opDesc->nodeDesc->data.nodeBasicInfo.opType) + " ";
        } else {
            opTypeStr += "null ";
        }

        if (task->op && task->op->opDesc && task->op->opDesc->nodeAttr) {
            hashIdStr += std::to_string(task->op->opDesc->nodeDesc->data.nodeAttrInfo.hashId) + " ";
        } else {
            hashIdStr += "null ";
        }

        if (task->op && task->op->opDesc && task->op->opDesc->tensorDesc) {
            tensorStr += std::to_string(task->op->opDesc->tensorDesc->timeStamp) + " ";
        } else {
            tensorStr += "null ";
        }
    }

    EXPECT_EQ(opTypeStr, expectComputeTaskOpTypesStr);
    EXPECT_EQ(hashIdStr, expectComputeTaskHashIdsStr);
    EXPECT_EQ(tensorStr, expectComputeTaskTensorTimesStr);

    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }

    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }

    // 由于unordered_map的特性，context_id不保序，先排序
    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }

    // 由于unordered_map的特性，context_id不保序，先排序
    std::sort(hcclTasks.begin(), hcclTasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->contextId, expectHcclTaskCtxIds[i]);
    }

    for (uint16_t i = 0; i < bigOPs.size(); i++) {
        EXPECT_EQ(bigOPs[i]->hcclBigOpDesc->opInfoDesc->data.hcclopInfo.count, expectbigOPsCount[i]);
    }
}
std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenarioEmptyPath()
{
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    ana->path_ = std::map<uint16_t, std::shared_ptr<TreeNode>>{};
    ana->path_[MSPROF_REPORT_NODE_LEVEL] = nullptr;
    return ana;
}

TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerGenComputeHostTasks)
{
    auto ana = GetAnalyzerForScenarioEmptyPath();
    std::unordered_map<std::string, std::shared_ptr<Operator>> ops{};
    
    HostTasks ht = ana -> GenComputeHostTasks(ops, nullptr, 0);
    EXPECT_EQ(ht, HostTasks{});
}

