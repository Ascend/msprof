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
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <tuple>
#include <utility>
#include "gtest/gtest.h"

#include "analysis/csrc/domain/services/association/cann/include/tree_builder.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/entities/tree/include/tree.h"
#include "analysis/csrc/domain/services/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"
#include "analysis/csrc/infrastructure/utils/prof_struct.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"

using namespace Analysis::Domain::Host::Cann;
using namespace Analysis::Domain::Cann;
using namespace Analysis::Domain;

class TreeBuilderUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

std::shared_ptr<EventQueue> GenNodeBasicInfoEvents()
{
    auto nodeBasicInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9    10
        {"Node", {105, 110, 115, 115, 130, 135, 150, 155, 160, 170, 1000}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        FakeEventGenerator::AddNodeBasicEvent(nodeBasicInfoEvents, dot);
        cnt++;
    }
    nodeBasicInfoEvents->Sort();
    return nodeBasicInfoEvents;
}

std::shared_ptr<EventQueue> GenNodeAttrInfoEvents()
{
    auto nodeAttrInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9    10
        {"Node", {105, 110, 115, 115, 130, 135, 150, 155, 160, 170, 1000}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        FakeEventGenerator::AddNodeAttrEvent(nodeAttrInfoEvents, dot);
        cnt++;
    }
    nodeAttrInfoEvents->Sort();
    return nodeAttrInfoEvents;
}

std::shared_ptr<EventQueue> GenTensorInfoEvents()
{
    auto tensorInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9
        {"Node", {109, 110, 113, 130, 134, 150, 151, 152, 170, 175}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        FakeEventGenerator::AddTensorInfoEvent(tensorInfoEvents, dot);
        cnt++;
    }
    tensorInfoEvents->Sort();
    return tensorInfoEvents;
}

std::shared_ptr<EventQueue> GenCtxIdEvents()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 20);
    std::unordered_map<std::string, std::vector<uint64_t>> nodelLevelEvents{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7
        {"Node", {109, 110, 130, 134, 150, 151, 170, 172}}
    };

    std::unordered_map<std::string, std::vector<uint64_t>> hcclLevelEvents{
        // HcclLevel: {"Hccl", {{115, 125}, {155, 165}}},
        //  Index      0     1    2    3    4
        {"Communication", {115, 117, 125, 130, 160}}
    };

    int nodeCnt = 0;
    for (auto dot: nodelLevelEvents["Node"]) {
        FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, dot, {dot, dot});
        nodeCnt++;
    }

    int hcclCnt = 0;
    for (auto dot: hcclLevelEvents["Communication"]) {
        FakeEventGenerator::AddHcclCtxIdEvent(contextIdEvents, dot, dot, dot);
        hcclCnt++;
    }
    contextIdEvents->Sort();
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenFusionOpInfoEvents()
{
    auto fusioOpInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        //  Model   {"Model", {{110, 200}}},
        //  Index       0     1    2    3    4
        {"Model", {104, 110, 116, 200, 220}}
    };
    int cnt = 0;
    for (auto dot: Events["Model"]) {
        FakeEventGenerator::AddFusionOpInfoEvent(fusioOpInfoEvents, dot);
        cnt++;
    }
    fusioOpInfoEvents->Sort();
    return fusioOpInfoEvents;
}

std::shared_ptr<EventQueue> GenHcclInfoEvents()
{
    auto hcclInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // HcclLevel: {"Hccl", {{115, 125}, {155, 165}}},
        //  Index       0     1    2    3    4   5     6    7    8   9
        {"Communication", {103, 110, 115, 120, 125, 130, 155, 160, 165, 170}}
    };
    int cnt = 0;
    for (auto dot: Events["Communication"]) {
        FakeEventGenerator::AddHcclInfoEvent(hcclInfoEvents, dot, cnt);
        cnt++;
    }
    hcclInfoEvents->Sort();
    return hcclInfoEvents;
}
/*
 EventLevel      此Level包含的EventType
|- ACL
|- Model    [graph_id_map, fusion_op_info]
|- Node     [node_basic_info, node_attr_info, tensor_info, context_id, hccl_op_info]
|- HCCL     [hccl_info, context_id]
|- Runtime  [task_track, mem_cpy]
*/
std::shared_ptr<EventQueue> GenKernelEvents()
{
    auto kernelEvents = std::make_shared<EventQueue>(1, 100);
    std::vector<std::string> levels{"Acl", "Model", "Node", "Communication"};
    std::unordered_map<std::string, uint16_t> levelNum{
        {"Acl", MSPROF_REPORT_ACL_LEVEL},
        {"Model", MSPROF_REPORT_MODEL_LEVEL},
        {"Node", MSPROF_REPORT_NODE_LEVEL},
        {"Communication", MSPROF_REPORT_HCCL_NODE_LEVEL},
    };
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Acl", {{10, 50}, {100, 215}}},
        {"Model", {{110, 200}}},
        {"Node", {{110, 130}, {150, 170}, {175, 180}}},
        {"Communication", {{115, 125}, {155, 165}}},
    };

    for (const auto &le: levels) {
        int cnt = 0;
        for (auto range: apiEvents[le]) {
            FakeEventGenerator::AddApiEvent(kernelEvents, levelNum[le], range.first, range.second);
            cnt++;
        }
    }
    kernelEvents->Sort();
    return kernelEvents;
}

std::shared_ptr<EventQueue> GenTaskTrackEvents()
{
    auto taskTrackEvents = std::make_shared<EventQueue>(1, 100);
    std::unordered_map<std::string, std::vector<uint64_t>> ttEvents{
        /*    Index        0    1    2    3    4    5    6    7    8    9    10   11  12 */
        {"Runtime", {11, 114, 115, 120, 125, 130, 155, 160, 165, 170, 175, 180, 190, 210}},
    };
    int cnt = 0;
    for (auto dot: ttEvents["Runtime"]) {
        FakeEventGenerator::AddTaskTrackEvent(taskTrackEvents, dot, 0);
        cnt++;
    }
    taskTrackEvents->Sort();
    return taskTrackEvents;
}

std::shared_ptr<EventQueue> GenHcclOpInfoEvents()
{
    auto hcclOpInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9    10
        {"Node", {105, 110, 115, 115, 130, 135, 150, 155, 160, 170, 1000}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        FakeEventGenerator::AddHcclOpEvent(
            hcclOpInfoEvents, dot,
            {AlgType::HCCL_ALG_HD, AlgType::HCCL_ALG_NB, AlgType::HCCL_ALG_MESH, AlgType::HCCL_ALG_NHR});
        cnt++;
    }
    hcclOpInfoEvents->Sort();
    return hcclOpInfoEvents;
}

TEST_F(TreeBuilderUTest, TestAddLevelEventsShouldReturnFalseWhenInputNullptrOrEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    /* 测试events为nullptr，levelNodes不为空的情况 */
    std::vector<std::shared_ptr<TreeNode>> oneNodes{std::shared_ptr<TreeNode>{}};
    auto nullEventQueuePtr = std::shared_ptr<EventQueue>{};
    auto checkNullPtr = treeBuilder->AddLevelEvents(nullEventQueuePtr, oneNodes,
                                                    EventType::EVENT_TYPE_NODE_BASIC_INFO);
    EXPECT_EQ(false, checkNullPtr);

    /* 测试events不为nullptr，levelNodes为空的情况 */
    auto eventQueuePtr = std::make_shared<EventQueue>(1, 5);
    std::vector<std::shared_ptr<TreeNode>> emptyNodes{};
    auto checkEmptyNodes = treeBuilder->AddLevelEvents(eventQueuePtr, emptyNodes,
                                                       EventType::EVENT_TYPE_NODE_BASIC_INFO);
    EXPECT_EQ(false, checkEmptyNodes);

    /* 测试events为nullptr，levelNodes为空的情况 */
    auto checkAllEmpty = treeBuilder->AddLevelEvents(nullEventQueuePtr, emptyNodes,
                                                     EventType::EVENT_TYPE_NODE_BASIC_INFO);
    EXPECT_EQ(false, checkEmptyNodes);
}

TEST_F(TreeBuilderUTest, TestAddLevelEventsShouldMatchCorrectNodeWhenInputNotEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    /* 构造数据：TreeNodes：     2---3        5---------8    10-----12          */
    /* 构造数据：Events：   1 1  2       4 4     6 6  7 8  9     11 12 13 13    */
    std::vector<std::shared_ptr<TreeNode>> levelNodes{};
    std::vector<std::pair<uint64_t, uint64_t>> nodesTimes{{2, 3}, {5, 8}, {10, 12}};
    for (auto range: nodesTimes) {
        EventInfo info{EventType::EVENT_TYPE_API, 0, range.first, range.second};
        auto event = std::make_shared<Event>(std::shared_ptr<ParserApi>{},
                                             info);
        levelNodes.emplace_back(std::make_shared<TreeNode>(event));
    }

    std::vector<uint64_t> eventDots{1, 1, 2, 4, 4, 6, 6, 7, 8, 9, 11, 12, 12, 13, 14};
    std::vector<uint64_t> ans{6, 6, 7, 8, 11, 12, 12};
    auto eventQueue = std::make_shared<EventQueue>(1, eventDots.size());
    for (auto dot: eventDots) {
        EventInfo info{EventType::EVENT_TYPE_NODE_BASIC_INFO, 0, dot, dot};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<ParserCompactInfo>{},
                                                info);
        eventQueue->Push(eventPtr);
    }

    auto checkFalse = treeBuilder->AddLevelEvents(eventQueue, levelNodes, EventType::EVENT_TYPE_NODE_BASIC_INFO);
    /* 因为存在没有匹配上的元素，范围值应为false */
    EXPECT_EQ(false, checkFalse);

    std::vector<uint64_t> valid;
    for (auto node: levelNodes) {
        if (!node->records.empty()) {
            for (const auto &e: node->records) {
                valid.emplace_back(e->info.start);
            }
        }
    }

    /* 首先需要保证size相等，然后检查每个元素是否相等 */
    EXPECT_EQ(ans.size(), valid.size());
    for (size_t i = 0; i < valid.size(); ++i) {
        EXPECT_EQ(ans[i], valid[i]);
    }
}

TEST_F(TreeBuilderUTest, TestBuildTreeShouldReturnRootNodeWhenCANNWarehouseEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    /* 建树 */
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto tree = treeBuilder->Build();
    EXPECT_EQ(nullptr, tree);
}

TEST_F(TreeBuilderUTest, TestBuildTreeShouldBuildCorrectTreeWhenCANNWarehouseNotEmpty)
{
    /* 生成cannWarehouse各个type的数据 */
    auto kernelEvents = GenKernelEvents();
    auto nodeBasicInfoEvents = GenNodeBasicInfoEvents();
    auto nodeAttrInfoEvents = GenNodeAttrInfoEvents();
    auto tensorInfoEvents = GenTensorInfoEvents();
    auto contextIdEvents = GenCtxIdEvents();
    auto fusionOpInfoEvents = GenFusionOpInfoEvents();
    auto hcclInfoEvents = GenHcclInfoEvents();
    auto taskTrackEvents = GenTaskTrackEvents();
    auto hcclOpInfoEvents = GenHcclOpInfoEvents();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->nodeAttrInfoEvents = nodeAttrInfoEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->fusionOpInfoEvents = fusionOpInfoEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->hcclOpInfoEvents = hcclOpInfoEvents;

    /* 期望生成的Tree每一层的字符串
     * Event格式: 类型 + start + _ + end, []标识其为additional类型Event，挂在左边最近的节点上
     * eg: Node层一个api节点(start = 1, end = 10)上挂了GraphIdMap(start = 2, end = 2)和
     * GraphIdMap(start = 5, end = 5)可表示为: Api1_10 [GraphIdMap2_2] [GraphIdMap5_5]
     * */
    // 核心event：
    // Acl,   [10, 50], [100, 215]
    // Model, [110, 200],
    // Node,  [110, 130], [150, 170], [175, 180],
    // Hccl,  [115, 125], [155, 165],
    // 逻辑树如下，其中[n]标识每个context id event中带几个context id
    // Acl : [10, 50]  [100,                                                                               215]
    // Model :  [110,                                                                                    200]
    // g(105)g(110)             g(115) g(200)                                                                   g(210)
    // f(104)f(110)             f(116) f(200)                                                                   f(220)
    // 其中n-a-t-c表示nodeBasicInfo, nodeAttrInfo, tensorInfo, ctxId
    // Node  :  [110,                              130]        [150,                      170]     [175, 180]
    // n(105) n(110)    n(115) n(115) n(130)         n(135) n(150)   n(155) n(160)      n(170)                  n(1000)
    // a(105) a(110)    a(115) a(115) a(130)         a(135) a(150)   a(155) a(160)      a(170)                  a(1000)
    // t(109) t(110)        t(113) t(130)            t(134) t(150)   t(151)             t(170)t(175)
    //                                                               t(152)
    // c(109) c(110)             c(130)              c(134) c(150)   c(151)             c(170)c(172)
    //
    //
    // Hccl  :          [115,                125]                 [155,               165]
    // h(103) h(110) h(115) h(120)  h(125)       h(130)  h(155)       h(160) h(165)        h(170)
    //  c(115)             c(117) c(125)         c(130)               c(160)
    // Runtime: tk(114) tk(115) tk(120) tk(125) tk(130)       tk(155) tk(160) tk(165)tk(170)tk(175) tk(180) tk(190,210)
    //  tk(11)
    std::vector<std::string> ans{
        "Api0_216 ", // 第0层
        "Api10_50 Api100_215 ", // 第1层
        "Dummy11_11 [TaskTrack11_11] Api110_200 [FusionOpInfo116_116] "
        "[FusionOpInfo200_200] Dummy210_210 [TaskTrack210_210] ",  // 第二层
        "Api110_130 [NodeBasicInfo115_115] [NodeBasicInfo115_115] [NodeBasicInfo130_130] "
        "[NodeAttrInfo115_115] [NodeAttrInfo115_115] [NodeAttrInfo130_130] "
        "[TensorInfo113_113] [TensorInfo130_130] "
        "[HcclOpInfo115_115] [HcclOpInfo115_115] [HcclOpInfo130_130] Api150_170 [NodeBasicInfo155_155] "
        "[NodeBasicInfo160_160] [NodeBasicInfo170_170] "
        "[NodeAttrInfo155_155] [NodeAttrInfo160_160] [NodeAttrInfo170_170] "
        "[TensorInfo151_151] [TensorInfo152_152] "
        "[TensorInfo170_170] "
        "[HcclOpInfo155_155] [HcclOpInfo160_160] [HcclOpInfo170_170] Api175_180 Dummy175_175 "
        "[TaskTrack175_175] Dummy190_190 [TaskTrack190_190] ", // 第三层
        "Api115_125 [ContextId117_117] [ContextId125_125] [HcclInfo120_120] "
        "[HcclInfo125_125] Dummy114_114 [TaskTrack114_114] Dummy115_115 [TaskTrack115_115] "
        "Dummy130_130 [TaskTrack130_130] Api155_165 [ContextId160_160] [HcclInfo160_160] [HcclInfo165_165] "
        "Dummy155_155 [TaskTrack155_155] Dummy170_170 [TaskTrack170_170] Dummy180_180 [TaskTrack180_180] ", // 第四层
        "Dummy120_120 [TaskTrack120_120] Dummy125_125 [TaskTrack125_125] "
        "Dummy160_160 [TaskTrack160_160] Dummy165_165 [TaskTrack165_165] " // 第五层
    };

    /* 建树 */
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();
    auto tree = std::make_shared<Tree>(treeNode);
    /* 层序遍历建完的树，将树的每一层保存为一个字符串 */
    auto levelStr = tree->Show();

    /* 首先需要保证size相等，然后检查每个元素是否相等 */
    EXPECT_EQ(ans.size(), levelStr.size());
    for (size_t i = 0; i < ans.size(); ++i) {
        EXPECT_EQ(ans[i], levelStr[i]);
    }
}

// 构造带parent链的node列表: nodes[0]为外层, 其余按parentIdx指定父节点
std::vector<std::shared_ptr<TreeNode>> GenLevelNodesWithParent(
    const std::vector<std::tuple<uint64_t, uint64_t, int>> &ranges)
{
    std::shared_ptr<ParserApi> apiEvent;
    uint16_t level = 3;
    std::vector<std::shared_ptr<TreeNode>> levelNodes;
    for (const auto &item : ranges) {
        EventInfo info(EventType::EVENT_TYPE_API, level, std::get<0>(item), std::get<1>(item));
        levelNodes.emplace_back(std::make_shared<TreeNode>(std::make_shared<Event>(apiEvent, info)));
    }
    for (size_t i = 0; i < ranges.size(); ++i) {
        int parentIdx = std::get<2>(ranges[i]);
        if (parentIdx >= 0 && static_cast<size_t>(parentIdx) < levelNodes.size()) {
            levelNodes[i]->parent = levelNodes[parentIdx];
            levelNodes[parentIdx]->children.emplace_back(levelNodes[i]);
        }
    }
    return levelNodes;
}

std::shared_ptr<Event> GenAdditionalEvent(EventType eventType, uint64_t timestamp)
{
    EventInfo info(eventType, 3, timestamp, timestamp);
    if (eventType == EventType::EVENT_TYPE_TENSOR_INFO) {
        std::shared_ptr<ParserConcatTensorInfo> tensorEvent;
        return std::make_shared<Event>(tensorEvent, info);
    }
    std::shared_ptr<ParserCompactInfo> compactEvent;
    return std::make_shared<Event>(compactEvent, info);
}

TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldReturnOffset2WhenNodeNested)
{
    // hccl aicpu下发场景, 会出现2层node嵌套, 需要计算offset, 关联至准确的node api
    // [=======================Node(hcom_allReduce_)==============================]
    //   [===Node(hcomAicpuInit)==]     [===Node(allreduceAicpuKernel)==]
    // 外层(10,200], 内层兄弟A(30,40]、B(80,120], 时间戳100落在B内
    auto levelNodes = GenLevelNodesWithParent({{10, 200, -1}, {30, 40, 0}, {80, 120, 0}});
    auto event = GenAdditionalEvent(EventType::EVENT_TYPE_NODE_BASIC_INFO, 100);

    EXPECT_EQ(2U, TreeBuilder::GetEventOffset(0, event, levelNodes));
}

// 双层nodeLaunch下, tensor/attr 只有单个时间戳, 落在内层node时间片内时应归属内层node,
// 否则内层node取不到自己的 tensor/attr 描述信息
TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldDrillDownForTensorAndAttr)
{
    // Node(外层, 老nodeLaunch) : (10, 200]
    //   Node(内层, Refer_to_kernelName) : (80, 120]
    // 附加信息时间戳 100 落在内层区间内
    auto levelNodes = GenLevelNodesWithParent({{10, 200, -1}, {80, 120, 0}});

    // NodeBasicInfo/NodeAttrInfo/TensorInfo 语义一致, 均应归属内层node
    for (auto eventType : {EventType::EVENT_TYPE_TENSOR_INFO, EventType::EVENT_TYPE_NODE_ATTR_INFO,
                           EventType::EVENT_TYPE_NODE_BASIC_INFO}) {
        auto event = GenAdditionalEvent(eventType, 100);
        EXPECT_EQ(1U, TreeBuilder::GetEventOffset(0, event, levelNodes))
            << "eventType=" << static_cast<int>(eventType);
    }
}

// 多层嵌套时取最内层命中的node
TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldReturnInnermostWhenMultiLevelNested)
{
    // Node (10, 500]
    //   Node (20, 400]
    //     Node (30, 300]
    //       Node (40, 100]
    // 附加信息时间戳 50 同时落在全部四层内, 应归属最内层 offset=3
    auto levelNodes = GenLevelNodesWithParent({{10, 500, -1}, {20, 400, 0}, {30, 300, 1}, {40, 100, 2}});
    auto event = GenAdditionalEvent(EventType::EVENT_TYPE_TENSOR_INFO, 50);

    EXPECT_EQ(3U, TreeBuilder::GetEventOffset(0, event, levelNodes));
}

// 附加信息落在内层node之外(仍在外层内)时不下钻
TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldStayOuterWhenEventOutsideInner)
{
    // Node(外层) (10, 200]; Node(内层) (80, 120]; 附加信息时间戳 150 只落在外层
    auto levelNodes = GenLevelNodesWithParent({{10, 200, -1}, {80, 120, 0}});
    auto event = GenAdditionalEvent(EventType::EVENT_TYPE_TENSOR_INFO, 150);

    EXPECT_EQ(0U, TreeBuilder::GetEventOffset(0, event, levelNodes));
}

// 相邻内层兄弟节点共用时间边界时, 时间戳等于后一节点起点应归属前一节点(左开右闭),
// 与AddLevelEvents/AddTaskTrackEvents的区间语义保持一致
TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldUseLeftOpenIntervalForSharedBoundary)
{
    // Node(外层) (10, 200]
    //   Node(A) (20, 80]
    //   Node(B) (80, 120]
    // 时间戳 80 恰好等于B的起点, 按左开右闭应归属A
    auto levelNodes = GenLevelNodesWithParent({{10, 200, -1}, {20, 80, 0}, {80, 120, 0}});
    auto event = GenAdditionalEvent(EventType::EVENT_TYPE_TENSOR_INFO, 80);

    EXPECT_EQ(1U, TreeBuilder::GetEventOffset(0, event, levelNodes));
}

// 兄弟节点时间重叠且非后代时, 不应挂到非后代节点上
TEST_F(TreeBuilderUTest, TestGetEventOffsetShouldNotDrillDownToNonDescendant)
{
    // Node(外层) (10, 200]
    //   Node(X) (30, 150]  <- levelNodes[1], 是外层后代
    //   Node(Y) (40, 120]  <- levelNodes[2], 与外层同级(非后代), 但时间覆盖时间戳60
    // 时间戳 60 应归属X(后代), 不能归属Y
    auto levelNodes = GenLevelNodesWithParent({{10, 200, -1}, {30, 150, 0}, {40, 120, -1}});
    auto event = GenAdditionalEvent(EventType::EVENT_TYPE_TENSOR_INFO, 60);

    EXPECT_EQ(1U, TreeBuilder::GetEventOffset(0, event, levelNodes));
}

TEST_F(TreeBuilderUTest, TestBuildShouldSetRecordEventKeyFromTaskTrackEventInfo)
{
    const uint32_t recordEventType = 196625;
    const uint64_t expectKey = 123456789;
    TypeData::GetInstance().Clear();
    TypeData::GetInstance().GetAll()[MSPROF_REPORT_ACL_LEVEL][recordEventType] = "aclrtRecordEvent";

    auto kernelEvents = std::make_shared<EventQueue>(1, 1);
    auto api = std::make_shared<ParserApi>();
    api->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    api->level = MSPROF_REPORT_ACL_LEVEL;
    api->type = recordEventType;
    api->threadId = 1;
    api->beginTime = 10;
    api->endTime = 20;
    EventInfo apiInfo{EventType::EVENT_TYPE_API, MSPROF_REPORT_ACL_LEVEL, api->beginTime, api->endTime};
    auto apiEvent = std::make_shared<Event>(api, apiInfo);
    kernelEvents->Push(apiEvent);
    kernelEvents->Sort();

    auto taskTrackEvents = std::make_shared<EventQueue>(1, 1);
    auto taskTrack = std::make_shared<ParserCompactInfo>();
    taskTrack->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    taskTrack->level = MSPROF_REPORT_RUNTIME_LEVEL;
    taskTrack->type = static_cast<uint32_t>(EventType::EVENT_TYPE_TASK_TRACK);
    taskTrack->threadId = 1;
    taskTrack->timeStamp = 15;
    taskTrack->data.runtimeTrack.eventInfo.key = expectKey;
    EventInfo taskTrackInfo{
        EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_RUNTIME_LEVEL, taskTrack->timeStamp, taskTrack->timeStamp};
    auto taskTrackEvent = std::make_shared<Event>(taskTrack, taskTrackInfo);
    taskTrackEvents->Push(taskTrackEvent);
    taskTrackEvents->Sort();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    auto tree = treeBuilder->Build();

    EXPECT_NE(nullptr, tree);
    EXPECT_EQ(expectKey, apiEvent->key);
    TypeData::GetInstance().Clear();
}

TEST_F(TreeBuilderUTest, TestBuildShouldSetWaitEventKeyFromTaskTrackNotifyInfo)
{
    const uint32_t waitEventType = 196626;
    const uint64_t expectKey = 987654321;
    TypeData::GetInstance().Clear();
    TypeData::GetInstance().GetAll()[MSPROF_REPORT_ACL_LEVEL][waitEventType] = "aclrtStreamWaitEvent";

    auto kernelEvents = std::make_shared<EventQueue>(1, 1);
    auto api = std::make_shared<ParserApi>();
    api->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    api->level = MSPROF_REPORT_ACL_LEVEL;
    api->type = waitEventType;
    api->threadId = 1;
    api->beginTime = 10;
    api->endTime = 20;
    EventInfo apiInfo{EventType::EVENT_TYPE_API, MSPROF_REPORT_ACL_LEVEL, api->beginTime, api->endTime};
    auto apiEvent = std::make_shared<Event>(api, apiInfo);
    kernelEvents->Push(apiEvent);
    kernelEvents->Sort();

    auto taskTrackEvents = std::make_shared<EventQueue>(1, 1);
    auto taskTrack = std::make_shared<ParserCompactInfo>();
    taskTrack->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    taskTrack->level = MSPROF_REPORT_RUNTIME_LEVEL;
    taskTrack->type = static_cast<uint32_t>(EventType::EVENT_TYPE_TASK_TRACK);
    taskTrack->threadId = 1;
    taskTrack->timeStamp = 15;
    taskTrack->data.runtimeTrack.notifyInfo.key = expectKey;
    EventInfo taskTrackInfo{
        EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_RUNTIME_LEVEL, taskTrack->timeStamp, taskTrack->timeStamp};
    auto taskTrackEvent = std::make_shared<Event>(taskTrack, taskTrackInfo);
    taskTrackEvents->Push(taskTrackEvent);
    taskTrackEvents->Sort();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    auto tree = treeBuilder->Build();

    EXPECT_NE(nullptr, tree);
    EXPECT_EQ(expectKey, apiEvent->key);
    TypeData::GetInstance().Clear();
}
