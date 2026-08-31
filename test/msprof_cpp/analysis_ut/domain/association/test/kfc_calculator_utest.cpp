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
#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#define private public
#include "analysis/csrc/domain/services/association/calculator/hccl/include/kfc_calculator.h"
#undef private

#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using Analysis::Common::NA;

namespace
{
DeviceHcclTask MakeTask(uint32_t streamId, uint32_t taskId, uint32_t contextId, uint32_t batchId, double timestamp,
                        double duration, double size = 0.0, int64_t opId = -1)
{
    DeviceHcclTask task;
    task.streamId = streamId;
    task.taskId = taskId;
    task.contextId = contextId;
    task.batchId = batchId;
    task.timestamp = timestamp;
    task.duration = duration;
    task.size = size;
    task.opId = opId;
    return task;
}

DeviceKfcOp MakeKernel(uint32_t streamId, uint32_t taskId, uint32_t contextId, uint32_t batchId, double start,
                       double end, int64_t kfcConnectionId = 0, const std::string& opName = "")
{
    DeviceKfcOp kernel;
    kernel.streamId = streamId;
    kernel.taskId = taskId;
    kernel.contextId = contextId;
    kernel.batchId = batchId;
    kernel.start = start;
    kernel.end = end;
    kernel.kfcConnectionId = kfcConnectionId;
    kernel.opName = opName;
    return kernel;
}

Mc2CommInfo MakeCommInfo(const std::string& groupName, int64_t rankSize, uint32_t aicpuKfcStreamId,
                         std::vector<uint32_t> commStreamIds)
{
    Mc2CommInfo info;
    info.groupName = groupName;
    info.rankSize = rankSize;
    info.aicpuKfcStreamId = aicpuKfcStreamId;
    info.commStreamIds = std::move(commStreamIds);
    return info;
}
}  // namespace

class KfcCalculatorUTest : public testing::Test {
protected:
    void SetUp() override
    {
        calculator_ = std::make_shared<KfcCalculator>();
    }

    void TearDown() override
    {
        calculator_.reset();
    }

protected:
    std::shared_ptr<KfcCalculator> calculator_;
};

// GetKfcHostHcclOp：一行 HCCL_OP 的 kfc_connection_ids（逗号分隔）映射到多个 kfc_connection_id
TEST_F(KfcCalculatorUTest, TestGetKfcHostHcclOp)
{
    std::vector<DeviceHcclHostOp> hostOps;
    DeviceHcclHostOp op1;
    op1.connectionId = 1001;
    op1.kfcConnectionIds = "500,501";
    op1.opName = "hcom_allReduce_";
    op1.groupName = "g1";
    DeviceHcclHostOp op2;
    op2.connectionId = 1002;
    op2.kfcConnectionIds = "502";
    op2.opName = "hcom_broadcast_";
    op2.groupName = "g2";
    hostOps.emplace_back(op1);
    hostOps.emplace_back(op2);

    auto map = calculator_->GetKfcHostHcclOp(hostOps);
    ASSERT_EQ(3u, map.size());
    EXPECT_EQ(1001, map.at(500)->connectionId);
    EXPECT_EQ(1001, map.at(501)->connectionId);
    EXPECT_EQ(1002, map.at(502)->connectionId);
    EXPECT_EQ("g1", map.at(501)->groupName);
    EXPECT_EQ("g2", map.at(502)->groupName);
}

// GetHcclAndMc2Op：匹配到 HCCL_OP 的 kernel 归为 HCCL，未匹配的归为 MC2；iter_id 按四元组出现次数递增
TEST_F(KfcCalculatorUTest, TestGetHcclAndMc2Op)
{
    std::vector<DeviceKfcOp> aicpuKernels;
    aicpuKernels.emplace_back(MakeKernel(1, 100, 0, 0, 1000, 1200, 500, "kfc_hccl_op"));
    aicpuKernels.emplace_back(MakeKernel(2, 110, 0, 0, 2000, 2200, 900, "kfc_mc2_op"));
    aicpuKernels.emplace_back(MakeKernel(2, 110, 0, 0, 3000, 3200, 900, "kfc_mc2_op_again"));

    std::vector<DeviceHcclHostOp> hostOps;
    DeviceHcclHostOp op;
    op.connectionId = 1001;
    op.kfcConnectionIds = "500";
    op.opName = "hcom_allReduce_";
    op.groupName = "g1";
    hostOps.emplace_back(op);

    std::vector<DeviceKfcOp> hcclKernels;
    std::vector<DeviceKfcOp> mc2Kernels;
    calculator_->GetHcclAndMc2Op(aicpuKernels, hostOps, hcclKernels, mc2Kernels);

    ASSERT_EQ(1u, hcclKernels.size());
    EXPECT_EQ(1001, hcclKernels[0].connectionId);
    EXPECT_EQ("hcom_allReduce_", hcclKernels[0].opName);
    EXPECT_EQ("g1", hcclKernels[0].groupName);
    EXPECT_EQ(1u, hcclKernels[0].iterId);

    ASSERT_EQ(2u, mc2Kernels.size());
    EXPECT_EQ(900, mc2Kernels[0].connectionId);
    EXPECT_EQ(1u, mc2Kernels[0].iterId);
    EXPECT_EQ(900, mc2Kernels[1].connectionId);
    EXPECT_EQ(2u, mc2Kernels[1].iterId);
}

// FilterMatchedTasks：只保留已匹配到 op 时间窗（op_id != -1）的 task
TEST_F(KfcCalculatorUTest, TestFilterMatchedTasks)
{
    std::vector<DeviceHcclTask> tasks;
    tasks.emplace_back(MakeTask(1, 1, 0, 0, 100, 10));           // opId 默认 -1 → 过滤
    tasks.emplace_back(MakeTask(1, 2, 0, 0, 200, 10, 0, 7));     // opId=7 → 保留
    tasks.emplace_back(MakeTask(1, 3, 0, 0, 300, 10, 0, -1));    // 过滤

    auto matched = calculator_->FilterMatchedTasks(tasks);
    ASSERT_EQ(1u, matched.size());
    EXPECT_EQ(2u, matched[0].taskId);
}

// GroupTasksByCommStream：按 comm_stream_id 展开分组，非 comm 流的 task 被排除
TEST_F(KfcCalculatorUTest, TestGroupTasksByCommStream)
{
    std::vector<DeviceHcclTask> tasks;
    tasks.emplace_back(MakeTask(10, 1, 0, 0, 100, 10));
    tasks.emplace_back(MakeTask(99, 2, 0, 0, 200, 10));  // 非 comm 流 → 排除
    tasks.emplace_back(MakeTask(11, 3, 0, 0, 300, 10));

    std::unordered_map<uint32_t, std::set<std::string>> table;
    table[10].insert("g1");
    table[11].insert("g1");
    table[11].insert("g2");

    auto grouped = calculator_->GroupTasksByCommStream(tasks, table);
    ASSERT_EQ(2u, grouped.size());
    EXPECT_EQ(2u, grouped["g1"].size());
    ASSERT_EQ(1u, grouped["g2"].size());
    EXPECT_EQ(3u, grouped["g2"][0].taskId);
}

// GroupKernelsByGroupName：按 group_name 分组，组内按 start 升序
TEST_F(KfcCalculatorUTest, TestGroupKernelsByGroupName)
{
    std::vector<DeviceKfcOp> kernels;
    DeviceKfcOp k1 = MakeKernel(1, 1, 0, 0, 2000, 2200);
    k1.groupName = "g2";
    DeviceKfcOp k2 = MakeKernel(1, 2, 0, 0, 1000, 1200);
    k2.groupName = "g1";
    DeviceKfcOp k3 = MakeKernel(1, 3, 0, 0, 1500, 1700);
    k3.groupName = "g2";
    kernels.emplace_back(k1);
    kernels.emplace_back(k2);
    kernels.emplace_back(k3);

    auto grouped = calculator_->GroupKernelsByGroupName(kernels);
    ASSERT_EQ(2u, grouped.size());
    ASSERT_EQ(1u, grouped["g1"].size());
    ASSERT_EQ(2u, grouped["g2"].size());
    EXPECT_DOUBLE_EQ(1500, grouped["g2"][0].start);
    EXPECT_DOUBLE_EQ(2000, grouped["g2"][1].start);
}

// GetMc2CommInfoData：构建 {aicpu_stream_id: (group_name, rank_size)} 与 {comm_stream_id: group_names}
TEST_F(KfcCalculatorUTest, TestGetMc2CommInfoData)
{
    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {10, 11}));
    commInfos.emplace_back(MakeCommInfo("g2", 4, 2, {11, 12}));

    auto result = calculator_->GetMc2CommInfoData(commInfos);
    const auto& aicpuInfo = result.first;
    const auto& commTable = result.second;
    EXPECT_EQ("g1", aicpuInfo.at(1).first);
    EXPECT_EQ(8, aicpuInfo.at(1).second);
    EXPECT_EQ("g2", aicpuInfo.at(2).first);
    EXPECT_EQ(4, aicpuInfo.at(2).second);
    EXPECT_EQ(1u, commTable.at(10).size());
    EXPECT_EQ(2u, commTable.at(11).size());
    EXPECT_EQ(1u, commTable.at(12).size());
    EXPECT_EQ("g1", *commTable.at(10).begin());
}

// AssignOpAndPlane：时间窗内的 task 标记主流并回填 op 信息，时间窗外 is_master=0；
// 同一 stream 分配连续 plane_id；返回 kernel_times（首尾时间窗）
TEST_F(KfcCalculatorUTest, TestAssignOpAndPlane)
{
    std::map<std::string, std::vector<DeviceHcclTask>> groupedTasks;
    groupedTasks["g1"].emplace_back(MakeTask(1, 1, 0, 0, 100, 10));  // 在 op 时间窗内
    groupedTasks["g1"].emplace_back(MakeTask(2, 2, 0, 0, 500, 10));  // 在 op 时间窗外

    std::map<std::string, std::vector<DeviceKfcOp>> kernelByGroup;
    DeviceKfcOp kernel = MakeKernel(1, 1, 0, 0, 90, 200, 777, "op1");
    kernel.connectionId = 777;
    kernel.groupName = "g1";
    kernel.modelId = 42;
    kernel.indexId = -5;
    kernel.iterId = 3;
    kernelByGroup["g1"].emplace_back(kernel);

    auto kernelTimes = calculator_->AssignOpAndPlane(groupedTasks, kernelByGroup, 1);

    const auto& inWindow = groupedTasks["g1"][0];
    EXPECT_EQ(1u, inWindow.isMaster);
    EXPECT_EQ(777, inWindow.opId);
    EXPECT_EQ("g1", inWindow.groupName);
    EXPECT_EQ(42u, inWindow.modelId);
    EXPECT_EQ(-5, inWindow.indexId);
    EXPECT_EQ(3u, inWindow.iterId);
    EXPECT_EQ("op1", inWindow.opName);
    EXPECT_EQ(1, inWindow.planeId);

    const auto& outWindow = groupedTasks["g1"][1];
    EXPECT_EQ(0u, outWindow.isMaster);
    EXPECT_EQ(2, outWindow.planeId);

    auto key = std::make_tuple(1u, 1u, 0u, 0u, 3u);
    auto it = kernelTimes.find(key);
    ASSERT_NE(it, kernelTimes.end());
    EXPECT_DOUBLE_EQ(100, it->second.first);
    EXPECT_DOUBLE_EQ(110, it->second.second);
}

// RefineKernelTimesWithMasterStream：FIRST/LAST 主流任务修正 aicpu kernel 的 start/end
TEST_F(KfcCalculatorUTest, TestRefineKernelTimesWithMasterStream)
{
    std::vector<DeviceMasterStreamHcclTask> masterTasks;
    DeviceMasterStreamHcclTask first;
    first.taskType = KfcCalculator::FIRST_TASK_TYPE;
    first.streamId = 5;
    first.taskId = 50;
    first.batchId = 0;
    first.aicpuStreamId = 1;
    first.aicpuTaskId = 10;
    first.aicpuBatchId = 0;
    masterTasks.emplace_back(first);

    DeviceMasterStreamHcclTask last;
    last.taskType = KfcCalculator::LAST_TASK_TYPE;
    last.streamId = 6;
    last.taskId = 60;
    last.batchId = 0;
    last.aicpuStreamId = 1;
    last.aicpuTaskId = 10;
    last.aicpuBatchId = 0;
    masterTasks.emplace_back(last);

    std::vector<DeviceHcclTask> kfcTasks;
    kfcTasks.emplace_back(MakeTask(5, 50, UINT32_MAX, 0, 1000, 100));  // FIRST 对应 small task → 首=1000
    kfcTasks.emplace_back(MakeTask(6, 60, UINT32_MAX, 0, 1200, 50));   // LAST 对应 small task → 尾=1250

    std::vector<DeviceKfcOp> hcclKernels;
    hcclKernels.emplace_back(MakeKernel(1, 10, UINT32_MAX, 0, 0, 0));
    hcclKernels[0].iterId = 1;

    calculator_->RefineKernelTimesWithMasterStream(masterTasks, kfcTasks, hcclKernels);
    ASSERT_EQ(1u, hcclKernels.size());
    EXPECT_DOUBLE_EQ(1000, hcclKernels[0].start);
    EXPECT_DOUBLE_EQ(1250, hcclKernels[0].end);
}

// RefineKernelTimesWithMasterStream：同一 uid 多次执行（重复执行）时，各轮 kernel 取对应轮次
// 的小 task 时间，不能被最后一轮覆盖
TEST_F(KfcCalculatorUTest, TestRefineKernelTimesWithRepeatedExecution)
{
    // 两轮执行：FIRST 起始 task(5,50)、LAST 结束 task(6,60)，同一 uid 重复出现
    std::vector<DeviceMasterStreamHcclTask> masterTasks;
    DeviceMasterStreamHcclTask first;
    first.taskType = KfcCalculator::FIRST_TASK_TYPE;
    first.streamId = 5;
    first.taskId = 50;
    first.batchId = 0;
    first.aicpuStreamId = 1;
    first.aicpuTaskId = 10;
    first.aicpuBatchId = 0;

    DeviceMasterStreamHcclTask last;
    last.taskType = KfcCalculator::LAST_TASK_TYPE;
    last.streamId = 6;
    last.taskId = 60;
    last.batchId = 0;
    last.aicpuStreamId = 1;
    last.aicpuTaskId = 10;
    last.aicpuBatchId = 0;
    // 按 timestamp 顺序：第一轮 F/L、第二轮 F/L
    masterTasks.emplace_back(first);
    masterTasks.emplace_back(last);
    masterTasks.emplace_back(first);
    masterTasks.emplace_back(last);

    // kfc 小 task：同一 uid 各两次执行，时间不同（故意乱序，验证按 timestamp 排序）
    std::vector<DeviceHcclTask> kfcTasks;
    kfcTasks.emplace_back(MakeTask(5, 50, UINT32_MAX, 0, 1000, 100));  // FIRST uid 第 1 次
    kfcTasks.emplace_back(MakeTask(6, 60, UINT32_MAX, 0, 3000, 100));  // LAST uid 第 1 次
    kfcTasks.emplace_back(MakeTask(5, 50, UINT32_MAX, 0, 2000, 100));  // FIRST uid 第 2 次
    kfcTasks.emplace_back(MakeTask(6, 60, UINT32_MAX, 0, 4000, 100));  // LAST uid 第 2 次

    std::vector<DeviceKfcOp> hcclKernels;
    DeviceKfcOp kernel1 = MakeKernel(1, 10, UINT32_MAX, 0, 0, 0);
    kernel1.iterId = 1;
    hcclKernels.emplace_back(kernel1);
    DeviceKfcOp kernel2 = MakeKernel(1, 10, UINT32_MAX, 0, 0, 0);
    kernel2.iterId = 2;
    hcclKernels.emplace_back(kernel2);

    calculator_->RefineKernelTimesWithMasterStream(masterTasks, kfcTasks, hcclKernels);
    ASSERT_EQ(2u, hcclKernels.size());
    // 第一轮：start=FIRST uid 第 1 次(1000)，end=LAST uid 第 1 次(3000+100=3100)
    EXPECT_DOUBLE_EQ(1000, hcclKernels[0].start);
    EXPECT_DOUBLE_EQ(3100, hcclKernels[0].end);
    // 第二轮：start=FIRST uid 第 2 次(2000)，end=LAST uid 第 2 次(4000+100=4100)
    EXPECT_DOUBLE_EQ(2000, hcclKernels[1].start);
    EXPECT_DOUBLE_EQ(4100, hcclKernels[1].end);
}

// SerializeKfcTask：25 列字段映射 + source
TEST_F(KfcCalculatorUTest, TestSerializeKfcTask)
{
    DeviceHcclTask task;
    task.modelId = 42;
    task.indexId = -1;
    task.hcclName = "KfcAllReduce";
    task.groupName = "g1";
    task.planeId = 3;
    task.timestamp = 1000;
    task.duration = 200;
    task.opId = 7;
    task.isMaster = 1;
    task.streamId = 1;
    task.taskId = 2;
    task.contextId = 3;
    task.batchId = 4;
    task.size = 1024;
    task.bandwidth = 5.12;
    task.localRank = 1;
    task.remoteRank = 2;
    task.rankSize = 8;
    task.transportType = "SDMA";
    task.dataType = "FP16";
    task.linkType = "ON_CHIP";
    task.rdmaType = "INVALID_TYPE";
    task.notifyId = "100";
    task.iterId = 5;

    auto record = calculator_->SerializeKfcTask(task, HcclType::HCCL);
    EXPECT_EQ(42u, record.modelId);
    EXPECT_EQ(-1, record.indexId);
    EXPECT_EQ("KfcAllReduce", record.hcclName);
    EXPECT_EQ("g1", record.groupName);
    EXPECT_EQ(3, record.planeId);
    EXPECT_DOUBLE_EQ(1000, record.timestamp);
    EXPECT_DOUBLE_EQ(200, record.duration);
    EXPECT_EQ(7, record.opId);
    EXPECT_EQ(1u, record.isMaster);
    EXPECT_EQ(1u, record.streamId);
    EXPECT_EQ(2u, record.taskId);
    EXPECT_EQ(3u, record.contextId);
    EXPECT_EQ(4u, record.batchId);
    EXPECT_DOUBLE_EQ(1024, record.size);
    EXPECT_DOUBLE_EQ(5.12, record.bandwidth);
    EXPECT_EQ(1, record.localRank);
    EXPECT_EQ(2, record.remoteRank);
    EXPECT_EQ(8, record.rankSize);
    EXPECT_EQ("SDMA", record.transportType);
    EXPECT_EQ("FP16", record.dataType);
    EXPECT_EQ("ON_CHIP", record.linkType);
    EXPECT_EQ("INVALID_TYPE", record.rdmaType);
    EXPECT_EQ("100", record.notifyId);
    EXPECT_EQ(5u, record.iterId);
    EXPECT_EQ(HcclType::HCCL, record.source);
}

// SerializeKfcOp：16 列字段映射 + source
TEST_F(KfcCalculatorUTest, TestSerializeKfcOp)
{
    DeviceKfcOp kernel = MakeKernel(1, 2, 3, 4, 1000, 1200, 900, "op1");
    kernel.modelId = 42;
    kernel.indexId = -1;
    kernel.connectionId = 900;
    kernel.groupName = "g1";
    kernel.opType = "op1";
    kernel.relay = 1;
    kernel.retry = 2;
    kernel.dataType = "FP16";
    kernel.algType = "HD";
    kernel.count = 100;
    kernel.rankSize = 8;
    kernel.iterId = 5;

    auto record = calculator_->SerializeKfcOp(kernel, HcclType::MC2);
    EXPECT_EQ(42u, record.modelId);
    EXPECT_EQ(-1, record.indexId);
    EXPECT_EQ("op1", record.opName);
    EXPECT_DOUBLE_EQ(1000, record.start);
    EXPECT_DOUBLE_EQ(1200, record.end);
    EXPECT_EQ("g1", record.groupName);
    EXPECT_EQ(900, record.connectionId);
    EXPECT_EQ("op1", record.opType);
    EXPECT_EQ(1, record.relay);
    EXPECT_EQ(2, record.retry);
    EXPECT_EQ("FP16", record.dataType);
    EXPECT_EQ("HD", record.algType);
    EXPECT_EQ(100u, record.count);
    EXPECT_EQ(8, record.rankSize);
    EXPECT_EQ(5u, record.iterId);
    EXPECT_EQ(HcclType::MC2, record.source);
}

// BuildAicpuKernels：ASCEND_TASK 与 op_name 内存 join，过滤 INVALID_TIME，按 start 升序
TEST_F(KfcCalculatorUTest, TestBuildAicpuKernels)
{
    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 100, 0, 1, 0, -1, "device", "host", 4294967295, 500, 1000, 1200));
    ascendTasks.emplace_back(TopDownTask(false, 101, 0, 1, 0, -1, "device", "host", 4294967295, 600, -1, -1));

    AicpuOpNameMap opMap;
    opMap[TaskId(1u, 0u, 100u, 0u)] = "kfc_op_a";

    std::vector<DeviceKfcOp> kernels;
    calculator_->BuildAicpuKernels(ascendTasks, opMap, kernels);
    ASSERT_EQ(1u, kernels.size());
    EXPECT_EQ(1u, kernels[0].streamId);
    EXPECT_EQ(100u, kernels[0].taskId);
    EXPECT_EQ(0u, kernels[0].contextId);
    EXPECT_DOUBLE_EQ(1000, kernels[0].start);
    EXPECT_DOUBLE_EQ(1200, kernels[0].end);
    EXPECT_EQ(500, kernels[0].kfcConnectionId);
    EXPECT_EQ("kfc_op_a", kernels[0].opName);
    EXPECT_EQ(4294967295u, kernels[0].modelId);
}

// BuildKfcTasks（level0）：仅 ASCEND_TASK + kfc 默认值
TEST_F(KfcCalculatorUTest, TestBuildKfcTasksLevel0)
{
    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 10, 0, 1, 3, -1, "device", "host", 4294967295, 0, 1000, 1100));

    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {1}));

    std::vector<KfcInfoData> kfcInfos;  // level0 分支不参与 join
    std::vector<DeviceHcclTask> kfcTasks;
    calculator_->BuildKfcTasks(true, kfcInfos, ascendTasks, commInfos, kfcTasks);

    ASSERT_EQ(1u, kfcTasks.size());
    EXPECT_EQ(NA, kfcTasks[0].hcclName);
    EXPECT_EQ(-1, kfcTasks[0].localRank);
    EXPECT_EQ(-1, kfcTasks[0].remoteRank);
    EXPECT_EQ(-1, kfcTasks[0].rankSize);
    EXPECT_EQ(0, kfcTasks[0].planeId);
    EXPECT_EQ(3u, kfcTasks[0].contextId);
    EXPECT_EQ(NA, kfcTasks[0].notifyId);
    EXPECT_DOUBLE_EQ(-1, kfcTasks[0].size);
    EXPECT_EQ(1u, kfcTasks[0].streamId);
    EXPECT_EQ(10u, kfcTasks[0].taskId);
    EXPECT_EQ(0u, kfcTasks[0].batchId);
    EXPECT_DOUBLE_EQ(1000, kfcTasks[0].timestamp);
    EXPECT_DOUBLE_EQ(100, kfcTasks[0].duration);
    EXPECT_DOUBLE_EQ(0, kfcTasks[0].bandwidth);
    EXPECT_EQ(-1, kfcTasks[0].opId);
}

// BuildKfcTasks（非 level0）：KFC_INFO 与 ASCEND_TASK 按四元组 join，回填 kfc 字段
TEST_F(KfcCalculatorUTest, TestBuildKfcTasksNonLevel0)
{
    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 10, 0, 1, 3, -1, "device", "host", 4294967295, 0, 1000, 1100));

    std::vector<KfcInfoData> kfcInfos;
    KfcInfoData info;
    info.streamId = 1;
    info.taskId = 10;
    info.contextId = 3;
    info.batchId = 0;
    info.hcclName = "KfcAllReduce";
    info.localRank = 1;
    info.remoteRank = 2;
    info.rankSize = 8;
    info.planeId = 5;
    info.notifyId = "100";
    info.size = 1024;
    info.dataType = "FP16";
    info.linkType = "ON_CHIP";
    info.transportType = "SDMA";
    info.rdmaType = "INVALID_TYPE";
    kfcInfos.emplace_back(info);

    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {1}));

    std::vector<DeviceHcclTask> kfcTasks;
    calculator_->BuildKfcTasks(false, kfcInfos, ascendTasks, commInfos, kfcTasks);

    ASSERT_EQ(1u, kfcTasks.size());
    EXPECT_EQ("KfcAllReduce", kfcTasks[0].hcclName);
    EXPECT_EQ(1, kfcTasks[0].localRank);
    EXPECT_EQ(2, kfcTasks[0].remoteRank);
    EXPECT_EQ(8, kfcTasks[0].rankSize);
    EXPECT_EQ(5, kfcTasks[0].planeId);
    EXPECT_EQ(3u, kfcTasks[0].contextId);
    EXPECT_EQ("100", kfcTasks[0].notifyId);
    EXPECT_DOUBLE_EQ(1024, kfcTasks[0].size);
    EXPECT_EQ("FP16", kfcTasks[0].dataType);
    EXPECT_EQ("ON_CHIP", kfcTasks[0].linkType);
    EXPECT_EQ("SDMA", kfcTasks[0].transportType);
    EXPECT_EQ("INVALID_TYPE", kfcTasks[0].rdmaType);
    EXPECT_EQ(1u, kfcTasks[0].streamId);
    EXPECT_EQ(10u, kfcTasks[0].taskId);
    EXPECT_DOUBLE_EQ(1000, kfcTasks[0].timestamp);
    EXPECT_DOUBLE_EQ(100, kfcTasks[0].duration);
    EXPECT_EQ(-1, kfcTasks[0].opId);
}

// BuildKfcTasks（非 level0）：同一四元组在 ASCEND_TASK 多次出现时 JOIN 展开为多行，全部保留
TEST_F(KfcCalculatorUTest, TestBuildKfcTasksExpandDuplicateAscendTask)
{
    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 10, 0, 1, 3, -1, "device", "host", 4294967295, 0, 1000, 1100));
    ascendTasks.emplace_back(TopDownTask(false, 10, 0, 1, 3, -1, "device", "host", 4294967295, 0, 1500, 1600));

    std::vector<KfcInfoData> kfcInfos;
    KfcInfoData info;
    info.streamId = 1;
    info.taskId = 10;
    info.contextId = 3;
    info.batchId = 0;
    info.hcclName = "KfcAllReduce";
    kfcInfos.emplace_back(info);

    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {1}));

    std::vector<DeviceHcclTask> kfcTasks;
    calculator_->BuildKfcTasks(false, kfcInfos, ascendTasks, commInfos, kfcTasks);

    ASSERT_EQ(2u, kfcTasks.size());
    EXPECT_DOUBLE_EQ(1000, kfcTasks[0].timestamp);
    EXPECT_DOUBLE_EQ(1500, kfcTasks[1].timestamp);
}

// Calculate：HCCL/MC2 双分支全流程，验证 kfcTask/kfcOp/kfcOpReport 输出
TEST_F(KfcCalculatorUTest, TestCalculateWithHcclAndMc2)
{
    KfcUpstreamData upstream;
    // aicpu kernels（已按 start 升序）
    upstream.aicpuKernels.emplace_back(MakeKernel(1, 100, 0, 0, 1000, 1200, 500, "kfc_hccl_op"));
    upstream.aicpuKernels.emplace_back(MakeKernel(2, 110, 0, 0, 2000, 2200, 900, "kfc_mc2_op"));
    // kfc_connection_id=500 匹配 HCCL_OP → HCCL；900 未匹配 → MC2
    DeviceHcclHostOp hostOp;
    hostOp.connectionId = 1001;
    hostOp.kfcConnectionIds = "500";
    hostOp.opName = "hcom_allReduce_";
    hostOp.groupName = "g1";
    upstream.hcclHostOps.emplace_back(hostOp);
    // kfc tasks：与两个 kernel 分别落在各自时间窗内
    upstream.kfcTasks.emplace_back(MakeTask(1, 200, 0, 0, 1010, 10, 1024));
    upstream.kfcTasks.emplace_back(MakeTask(2, 210, 0, 0, 2010, 10, 2048));
    upstream.mc2CommInfo.emplace_back(MakeCommInfo("g1", 8, 1, {1, 2}));
    upstream.mc2CommInfo.emplace_back(MakeCommInfo("g2", 4, 2, {1, 2}));
    upstream.startTimeRawTimestamp = 0;
    upstream.isLevel0 = false;

    calculator_->Calculate(upstream);

    const auto& taskData = calculator_->GetKfcTaskData();
    ASSERT_EQ(2u, taskData.size());
    // taskA → HCCL
    EXPECT_EQ(HcclType::HCCL, taskData[0].source);
    EXPECT_EQ(1001, taskData[0].opId);
    EXPECT_EQ("g1", taskData[0].groupName);
    EXPECT_EQ(1, taskData[0].planeId);
    EXPECT_EQ(1u, taskData[0].isMaster);
    EXPECT_EQ(1u, taskData[0].iterId);
    EXPECT_DOUBLE_EQ(95.367431640625, taskData[0].bandwidth);
    // taskB → MC2
    EXPECT_EQ(HcclType::MC2, taskData[1].source);
    EXPECT_EQ(900, taskData[1].opId);
    EXPECT_EQ("g2", taskData[1].groupName);
    EXPECT_EQ(1, taskData[1].planeId);
    EXPECT_EQ(1u, taskData[1].isMaster);
    EXPECT_EQ(1u, taskData[1].iterId);
    EXPECT_DOUBLE_EQ(190.73486328125, taskData[1].bandwidth);

    const auto& opData = calculator_->GetKfcOpData();
    ASSERT_EQ(1u, opData.size());
    EXPECT_EQ("kfc_mc2_op_g2_0_1", opData[0].opName);
    EXPECT_EQ(900, opData[0].connectionId);
    EXPECT_EQ("g2", opData[0].groupName);
    EXPECT_EQ(HcclType::MC2, opData[0].source);
    EXPECT_DOUBLE_EQ(2010, opData[0].start);
    EXPECT_DOUBLE_EQ(2020, opData[0].end);

    const auto& reportData = calculator_->GetMc2OpReportData();
    ASSERT_EQ(1u, reportData.size());
    EXPECT_EQ("kfc_mc2_op", reportData[0].opType);
    EXPECT_EQ(1u, reportData[0].count);
    EXPECT_DOUBLE_EQ(10, reportData[0].totalTime);
    EXPECT_DOUBLE_EQ(10, reportData[0].min);
    EXPECT_DOUBLE_EQ(10, reportData[0].max);
    EXPECT_DOUBLE_EQ(10, reportData[0].avg);
}

// Calculate（level0）：不生成报表数据
TEST_F(KfcCalculatorUTest, TestCalculateWhenLevel0SkipsReport)
{
    KfcUpstreamData upstream;
    upstream.aicpuKernels.emplace_back(MakeKernel(2, 110, 0, 0, 2000, 2200, 900, "kfc_mc2_op"));
    upstream.kfcTasks.emplace_back(MakeTask(2, 210, 0, 0, 2010, 10, 2048));
    upstream.mc2CommInfo.emplace_back(MakeCommInfo("g2", 4, 2, {2}));
    upstream.startTimeRawTimestamp = 0;
    upstream.isLevel0 = true;

    calculator_->Calculate(upstream);
    EXPECT_TRUE(calculator_->GetMc2OpReportData().empty());
    // 非 level0 分支的 op 输出不受影响
    EXPECT_EQ(1u, calculator_->GetKfcOpData().size());
}

// ProcessEntry：无 mc2 通信域 → 优雅跳过，返回 ANALYSIS_OK，不注入数据
TEST_F(KfcCalculatorUTest, TestProcessEntryWhenMc2CommInfoMissingThenSkipOk)
{
    Analysis::Infra::DataInventory dataInventory;

    std::vector<HcclOp> hcclOps;
    HcclOp op;
    op.connectionId = 1001;
    op.kfcConnectionIds = "500";
    op.opName = "hcom_allReduce_";
    op.groupName = "g1";
    hcclOps.emplace_back(op);
    dataInventory.Inject(std::make_shared<std::vector<HcclOp>>(hcclOps));

    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 100, 0, 1, 0, -1, "device", "host", 4294967295, 500, 1000, 1200));
    dataInventory.Inject(std::make_shared<std::vector<TopDownTask>>(ascendTasks));

    KfcCalculator calculator;
    Analysis::Domain::DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, calculator.Run(dataInventory, context));
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcTaskRecord>>(), nullptr);
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcOpRecord>>(), nullptr);
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcOpStatistics>>(), nullptr);
}

// ProcessEntry：无 aicpu kernel（op_name 缺失）→ 优雅跳过，返回 ANALYSIS_OK
TEST_F(KfcCalculatorUTest, TestProcessEntryWhenNoAicpuKernelThenSkipOk)
{
    Analysis::Infra::DataInventory dataInventory;

    std::vector<HcclOp> hcclOps;
    HcclOp op;
    op.connectionId = 1001;
    op.kfcConnectionIds = "500";
    op.opName = "hcom_allReduce_";
    op.groupName = "g1";
    hcclOps.emplace_back(op);
    dataInventory.Inject(std::make_shared<std::vector<HcclOp>>(hcclOps));

    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 100, 0, 1, 0, -1, "device", "host", 4294967295, 500, 1000, 1200));
    dataInventory.Inject(std::make_shared<std::vector<TopDownTask>>(ascendTasks));

    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {1}));
    dataInventory.Inject(std::make_shared<std::vector<Mc2CommInfo>>(commInfos));

    KfcCalculator calculator;
    Analysis::Domain::DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, calculator.Run(dataInventory, context));
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcTaskRecord>>(), nullptr);
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcOpRecord>>(), nullptr);
    EXPECT_EQ(dataInventory.GetPtr<std::vector<KfcOpStatistics>>(), nullptr);
}

// ProcessEntry：上游数据缺失（无 ASCEND_TASK / HCCL_OP）→ 返回 ANALYSIS_ERROR
TEST_F(KfcCalculatorUTest, TestProcessEntryWhenDataMissingThenError)
{
    Analysis::Infra::DataInventory dataInventory;
    KfcCalculator calculator;
    Analysis::Domain::DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_ERROR, calculator.Run(dataInventory, context));
}

// ProcessEntry：全链路注入上游数据 → HCCL/MC2 任务、MC2 算子与报表均注入 DataInventory
TEST_F(KfcCalculatorUTest, TestProcessEntryWhenSuccessThenInjectData)
{
    Analysis::Infra::DataInventory dataInventory;

    std::vector<HcclOp> hcclOps;
    HcclOp op;
    op.modelId = 4294967295;
    op.connectionId = 1001;
    op.indexId = -1;
    op.threadId = 1;
    op.kfcConnectionIds = "500";
    op.opName = "hcom_allReduce_";
    op.taskType = "HCCL";
    op.opType = "hcom_allReduce_";
    op.groupName = "g1";
    hcclOps.emplace_back(op);
    dataInventory.Inject(std::make_shared<std::vector<HcclOp>>(hcclOps));

    std::vector<TopDownTask> ascendTasks;
    ascendTasks.emplace_back(TopDownTask(false, 100, 0, 1, 0, -1, "device", "host", 4294967295, 500, 1000, 1200));
    ascendTasks.emplace_back(TopDownTask(false, 110, 0, 2, 0, -1, "device", "host", 4294967295, 900, 2000, 2200));
    dataInventory.Inject(std::make_shared<std::vector<TopDownTask>>(ascendTasks));

    std::vector<Mc2CommInfo> commInfos;
    commInfos.emplace_back(MakeCommInfo("g1", 8, 1, {1, 2}));
    commInfos.emplace_back(MakeCommInfo("g2", 4, 2, {1, 2}));
    dataInventory.Inject(std::make_shared<std::vector<Mc2CommInfo>>(commInfos));

    AicpuOpNameMap opMap;
    opMap[TaskId(1u, 0u, 100u, 0u)] = "kfc_hccl_op";
    opMap[TaskId(2u, 0u, 110u, 0u)] = "kfc_mc2_op";
    dataInventory.Inject(std::make_shared<AicpuOpNameMap>(opMap));

    std::vector<KfcInfoData> kfcInfos;
    KfcInfoData info1;
    info1.streamId = 1;
    info1.taskId = 100;
    info1.contextId = 0;
    info1.batchId = 0;
    info1.hcclName = "KfcHcclOp";
    info1.localRank = 1;
    info1.remoteRank = 2;
    info1.rankSize = 8;
    info1.size = 1024;
    kfcInfos.emplace_back(info1);
    KfcInfoData info2;
    info2.streamId = 2;
    info2.taskId = 110;
    info2.contextId = 0;
    info2.batchId = 0;
    info2.hcclName = "KfcMc2Op";
    info2.localRank = 1;
    info2.remoteRank = 2;
    info2.rankSize = 4;
    info2.size = 2048;
    kfcInfos.emplace_back(info2);
    dataInventory.Inject(std::make_shared<std::vector<KfcInfoData>>(kfcInfos));

    KfcCalculator calculator;
    Analysis::Domain::DeviceContext context;
    ASSERT_EQ(Analysis::ANALYSIS_OK, calculator.Run(dataInventory, context));

    auto taskData = dataInventory.GetPtr<std::vector<KfcTaskRecord>>();
    ASSERT_NE(taskData, nullptr);
    ASSERT_EQ(2u, taskData->size());
    // 先 HCCL 后 MC2（GenerateHcclKernels 先于 GenerateMc2Kernels）
    EXPECT_EQ(HcclType::HCCL, (*taskData)[0].source);
    EXPECT_EQ(1001, (*taskData)[0].opId);
    EXPECT_EQ("g1", (*taskData)[0].groupName);
    EXPECT_EQ(1, (*taskData)[0].planeId);
    EXPECT_EQ(1u, (*taskData)[0].isMaster);
    EXPECT_EQ("KfcHcclOp", (*taskData)[0].hcclName);
    EXPECT_DOUBLE_EQ(4.76837158203125, (*taskData)[0].bandwidth);
    EXPECT_EQ(HcclType::MC2, (*taskData)[1].source);
    EXPECT_EQ(900, (*taskData)[1].opId);
    EXPECT_EQ("g2", (*taskData)[1].groupName);
    EXPECT_EQ(1, (*taskData)[1].planeId);
    EXPECT_EQ(1u, (*taskData)[1].isMaster);
    EXPECT_EQ("KfcMc2Op", (*taskData)[1].hcclName);
    EXPECT_DOUBLE_EQ(9.5367431640625, (*taskData)[1].bandwidth);

    auto opData = dataInventory.GetPtr<std::vector<KfcOpRecord>>();
    ASSERT_NE(opData, nullptr);
    ASSERT_EQ(1u, opData->size());
    EXPECT_EQ("kfc_mc2_op_g2_0_1", (*opData)[0].opName);
    EXPECT_EQ(900, (*opData)[0].connectionId);
    EXPECT_EQ("g2", (*opData)[0].groupName);
    EXPECT_EQ(HcclType::MC2, (*opData)[0].source);
    EXPECT_DOUBLE_EQ(2000, (*opData)[0].start);
    EXPECT_DOUBLE_EQ(2200, (*opData)[0].end);

    auto reportData = dataInventory.GetPtr<std::vector<KfcOpStatistics>>();
    ASSERT_NE(reportData, nullptr);
    ASSERT_EQ(1u, reportData->size());
    EXPECT_EQ("kfc_mc2_op", (*reportData)[0].opType);
    EXPECT_EQ(1u, (*reportData)[0].count);
    EXPECT_DOUBLE_EQ(200, (*reportData)[0].totalTime);
    EXPECT_DOUBLE_EQ(200, (*reportData)[0].min);
    EXPECT_DOUBLE_EQ(200, (*reportData)[0].max);
    EXPECT_DOUBLE_EQ(200, (*reportData)[0].avg);
}
