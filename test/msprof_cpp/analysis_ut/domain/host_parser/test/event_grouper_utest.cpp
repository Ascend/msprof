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
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include "gtest/gtest.h"

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/domain/services/parser/host/cann/event_grouper.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/prof_common.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"
#include "analysis/csrc/domain/services/parser/host/cann/type_data.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain::Host::Cann;
const std::string TEST_FILE = "./unaging.additional.type_info_dic.slice_0";

class EventGrouperUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        if (File::Exist(TEST_FILE)) {
            File::DeleteFile(TEST_FILE);  // 删除test_file文件
        }
        // 创建test_file文件
        FileWriter fw(TEST_FILE);
        // 有效输入
        fw.WriteText("20000_655361:isGraphNeedRebuild\n");
        fw.WriteText("20000_196625:aclrtRecordEvent\n");
        fw.WriteText("5000_1125:CpuKernelLaunchExWithArgs_Big\n");
        fw.WriteText("5000_1077:MemCopy2DAsync\n");
        // node层(type=10000)仅launch(type=5)参与建树，其余类型（node_basic_info/ge_api等）不参与建树但仍落盘
        fw.WriteText("10000_5:launch\n");
        fw.WriteText("10000_0:node_basic_info\n");
        fw.WriteText("10000_65536:ge_api\n");
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile(TEST_FILE);  // 删除test_file文件
    }
};

void GenApiEventsBin(const std::string &fakeDataDir, const int num)
{
    std::vector<MsprofApi> apiTrace;
    const int n = 2;
    for (int i = n; i < num + n - 1; i++) {
        auto tmp = MsprofApi{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = MSPROF_REPORT_MODEL_LEVEL;
        tmp.type = static_cast<uint32_t>(EventType::EVENT_TYPE_API);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.reserve = static_cast<uint32_t>(i);
        tmp.beginTime = static_cast<uint64_t>(n * i);
        tmp.endTime = static_cast<uint64_t>(n * i + 1);
        tmp.itemId = static_cast<uint32_t>(i);

        apiTrace.emplace_back(tmp);
    }
    auto tmp = MsprofApi{};
    tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    tmp.level = MSPROF_REPORT_ACL_LEVEL;
    tmp.type = 196625;  // 196625为构造的aclrtRecordEvent对应的type值
    tmp.threadId = static_cast<uint32_t>(num + n - 1);
    tmp.reserve = static_cast<uint32_t>(num + n - 1);
    tmp.beginTime = static_cast<uint64_t>(n * (num + n - 1));
    tmp.endTime = static_cast<uint64_t>(n * (num + n - 1) + 1);
    tmp.itemId = static_cast<uint32_t>(num + n - 1);
    apiTrace.emplace_back(tmp);
    // 补充不正常的api
    for (int i = n; i < num + n - 1; i++) {
        auto tmp = MsprofApi{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = MSPROF_REPORT_MODEL_LEVEL;
        tmp.type = static_cast<uint32_t>(EventType::EVENT_TYPE_API);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.reserve = static_cast<uint32_t>(i);
        tmp.beginTime = static_cast<uint64_t>(n * i);
        tmp.endTime = static_cast<uint64_t>(n * i + 1);
        tmp.itemId = static_cast<uint32_t>(i);

        apiTrace.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofApi>(apiTrace, EventType::EVENT_TYPE_API, true, 0);
}

// 构造node层api事件：launch(type=5)与非launch(type=0/0x010000)混合，写bin
void GenNodeApiEventsBin(const std::string &fakeDataDir)
{
    std::vector<MsprofApi> apiTrace;
    auto addApi = [&](uint32_t type, uint64_t begin, uint64_t end) {
        auto tmp = MsprofApi{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = MSPROF_REPORT_NODE_LEVEL;
        tmp.type = type;
        tmp.threadId = 1;
        tmp.reserve = 1;
        tmp.beginTime = begin;
        tmp.endTime = end;
        tmp.itemId = 1;
        apiTrace.emplace_back(tmp);
    };
    // node层launch事件，应进入kernelEvents参与建树
    addApi(MSPROF_REPORT_NODE_LAUNCH_TYPE, 100, 200);
    addApi(MSPROF_REPORT_NODE_LAUNCH_TYPE, 300, 400);
    // node层非launch事件，不应进入kernelEvents（不参与建树），但api全量仍保留在apiTraces_中落盘
    addApi(MSPROF_REPORT_NODE_BASIC_INFO_TYPE, 150, 250);
    addApi(MSPROF_REPORT_NODE_GE_API_BASE_TYPE, 350, 450);

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofApi>(apiTrace, EventType::EVENT_TYPE_API, true, 0);
}

void GenAdditionalInfoEventsBin(const std::string &fakeDataDir, EventType type, uint16_t level, const int num)
{
    const int size = 8;
    const int n = 2;
    uint8_t fakeData[size];
    std::vector<MsprofAdditionalInfo> traces;
    for (int i = n; i < num + n; i++) {
        auto tmp = MsprofAdditionalInfo{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = level;
        tmp.type = static_cast<uint32_t>(type);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.dataLen = size;
        tmp.timeStamp = static_cast<uint64_t>(n * i + 1);
        memcpy(tmp.data, fakeData, size);

        traces.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofAdditionalInfo>(traces, type, true, 0);
}

void GenCompactInfoEventsBin(const std::string &fakeDataDir, EventType type, uint16_t level, const int num)
{
    const int size = 8;
    const int n = 2;
    std::vector<MsprofCompactInfo> traces;
    uint8_t fakeData[size];

    for (int i = n; i < num + n; i++) {
        auto tmp = MsprofCompactInfo{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = level;
        tmp.type = static_cast<uint32_t>(type);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.dataLen = size;
        tmp.timeStamp = static_cast<uint64_t>(n * i + 1);

        if (type == EventType::EVENT_TYPE_NODE_BASIC_INFO) {
            tmp.data.nodeBasicInfo = MsprofNodeBasicInfo{};
        } else if (type == EventType::EVENT_TYPE_TASK_TRACK) {
            tmp.data.runtimeTrack = MsprofRuntimeTrack{};
        } else {
            memcpy(tmp.data.info, fakeData, size);
        }
        traces.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofCompactInfo>(traces, type, true, 0);
}

uint64_t g_getCannEventsNum(std::set<uint32_t> &tids, CANNWarehouses &cwhs, const std::string &eventName)
{
    uint64_t cnt = 0;
    for (auto tid: tids) {
        if (eventName == "kernelEvents" && cwhs[tid].kernelEvents != nullptr) {
            cnt += cwhs[tid].kernelEvents->GetSize();
        } else if (eventName == "graphIdMapEvents" && cwhs[tid].graphIdMapEvents != nullptr) {
            cnt += cwhs[tid].graphIdMapEvents->GetSize();
        } else if (eventName == "fusionOpInfoEvents" && cwhs[tid].fusionOpInfoEvents != nullptr) {
            cnt += cwhs[tid].fusionOpInfoEvents->GetSize();
        } else if (eventName == "nodeBasicInfoEvents" && cwhs[tid].nodeBasicInfoEvents != nullptr) {
            cnt += cwhs[tid].nodeBasicInfoEvents->GetSize();
        } else if (eventName == "nodeAttrInfoEvents" && cwhs[tid].nodeAttrInfoEvents != nullptr) {
            cnt += cwhs[tid].nodeAttrInfoEvents->GetSize();
        } else if (eventName == "tensorInfoEvents" && cwhs[tid].tensorInfoEvents != nullptr) {
            cnt += cwhs[tid].tensorInfoEvents->GetSize();
        } else if (eventName == "contextIdEvents" && cwhs[tid].contextIdEvents != nullptr) {
            cnt += cwhs[tid].contextIdEvents->GetSize();
        } else if (eventName == "hcclInfoEvents" && cwhs[tid].hcclInfoEvents != nullptr) {
            cnt += cwhs[tid].hcclInfoEvents->GetSize();
        } else if (eventName == "taskTrackEvents" && cwhs[tid].taskTrackEvents != nullptr) {
            cnt += cwhs[tid].taskTrackEvents->GetSize();
        } else if (eventName == "hcclOpInfoEvents" && cwhs[tid].hcclOpInfoEvents != nullptr) {
            cnt += cwhs[tid].hcclOpInfoEvents->GetSize();
        }
    }
    return cnt;
}

// 测试输入空文件夹
TEST_F(EventGrouperUTest, TestGroupShouldReturnEmptyWhenDataDirEmpty)
{
    const std::string fakeDataDir = "./fakeDataEmpty";
    File::CreateDir(fakeDataDir);

    auto grouper = std::make_shared<EventGrouper>(fakeDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    EXPECT_EQ(0, tids.size());
    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}

// 测试文件夹中只有Api类型数据
TEST_F(EventGrouperUTest, TestGroupShouldReturnEmptyWhenDataOnlyHasApiBin)
{
    const std::string fakeDataDir = "./fakeDataApi";
    File::RemoveDir(fakeDataDir, 0);

    const std::string hostDataDir = fakeDataDir + "/host/data";
    const int MaxNum = 10000;
    TypeData::GetInstance().Load("./");
    GenApiEventsBin(fakeDataDir, MaxNum);
    auto grouper = std::make_shared<EventGrouper>(hostDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    auto res = grouper->GetGroupEvents();
    EXPECT_EQ(MaxNum, tids.size());
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "kernelEvents"));
    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}

// 测试输入所有类别二进制文件
TEST_F(EventGrouperUTest, TestGroupShouldGroupCorrespondingEventsWhenDataDirHasAllTypeBin)
{
    const std::string fakeDataDir = "./fakeData";
    File::RemoveDir(fakeDataDir, 0);
    const int MaxNum = 10000;
    const std::string hostDataDir = fakeDataDir + "/host/data";
    // 生成所有类型Fake数据
    GenApiEventsBin(fakeDataDir, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_GRAPH_ID_MAP,
                               MSPROF_REPORT_MODEL_LEVEL, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_FUSION_OP_INFO,
                               MSPROF_REPORT_MODEL_LEVEL, MaxNum);
    GenCompactInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_NODE_BASIC_INFO,
                            MSPROF_REPORT_NODE_LEVEL, MaxNum);
    GenCompactInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_NODE_ATTR_INFO,
                            MSPROF_REPORT_NODE_LEVEL, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_TENSOR_INFO,
                               MSPROF_REPORT_HCCL_NODE_LEVEL, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_CONTEXT_ID,
                               MSPROF_REPORT_NODE_LEVEL, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_HCCL_INFO,
                               MSPROF_REPORT_HCCL_NODE_LEVEL, MaxNum);
    GenCompactInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_TASK_TRACK,
                            MSPROF_REPORT_RUNTIME_LEVEL, MaxNum);
    GenCompactInfoEventsBin(fakeDataDir, EventType::EVENT_TYPE_HCCL_OP_INFO,
                            MSPROF_REPORT_NODE_LEVEL, MaxNum);

    // 多线程解析
    auto grouper = std::make_shared<EventGrouper>(hostDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    auto res = grouper->GetGroupEvents();
    // 检查每一个类型Event的数量是否符合预期
    EXPECT_EQ(MaxNum, tids.size());

    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "kernelEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "graphIdMapEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "fusionOpInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "nodeBasicInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "nodeAttrInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "tensorInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "contextIdEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "hcclInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "taskTrackEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "hcclOpInfoEvents"));

    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}

// 测试node层非launch的api事件不参与建树（不进kernelEvents），但api全量仍保存在apiTraces_中（落盘不受影响）
TEST_F(EventGrouperUTest, TestGroupShouldOnlyKeepNodeLaunchEventForBuildTree)
{
    const std::string fakeDataDir = "./fakeDataNodeFilter";
    File::RemoveDir(fakeDataDir, 0);
    const std::string hostDataDir = fakeDataDir + "/host/data";
    TypeData::GetInstance().Load("./");

    GenNodeApiEventsBin(fakeDataDir);
    auto grouper = std::make_shared<EventGrouper>(hostDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    auto res = grouper->GetGroupEvents();

    // 只有2条launch事件进入kernelEvents参与建树
    EXPECT_EQ(2, g_getCannEventsNum(tids, res, "kernelEvents"));
    // api全量（2条launch + 2条非launch）仍保留在apiTraces_中，落盘不受影响
    auto &apiTraces = grouper->GetApiTraces();
    EXPECT_EQ(4, apiTraces.size());
    bool hasNonLaunch = false;
    for (const auto &event : apiTraces)
    {
        if (event->apiPtr->type != MSPROF_REPORT_NODE_LAUNCH_TYPE)
        {
            hasNonLaunch = true;
        }
    }
    EXPECT_EQ(true, hasNonLaunch);
    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}
