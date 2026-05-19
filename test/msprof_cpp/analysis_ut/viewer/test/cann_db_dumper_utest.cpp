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


#include <iostream>
#include <dirent.h>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/domain/services/persistence/host/cann_trace_db_dumper.h"


using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Viewer::Database;
using namespace  Analysis::Domain;
using namespace Analysis::Domain::Cann;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t INPUT_DATA_TYPE_POSITION = 15;
const uint32_t GROUP_NAME_POSITION = 3;
const uint32_t THREAD_ID = 1;

class CannDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
        File::CreateDir(TEST_DB_FILE_PATH + "/sqlite");
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }

    static void MockGetHCCLTasks()
    {
        HostTask hcclHostTask;
        auto pMiniOpDesc = std::make_shared<HcclSmallOpDesc>(0, 0, nullptr);
        pMiniOpDesc->ctxId = 0;
        auto hcclTrace = MsprofHcclInfo{};
        hcclTrace.opType = 1;
        auto hcclInfo = MsprofAdditionalInfo();
        std::memcpy(hcclInfo.data, &hcclTrace, sizeof(hcclTrace));
        pMiniOpDesc->hcclInfo = std::make_shared<MsprofAdditionalInfo>(hcclInfo);
        auto hcclTaskPp = std::make_shared<Operator>(pMiniOpDesc, 0, OpType::OPTYPE_COMPUTE);
        hcclHostTask.op = hcclTaskPp;
        std::shared_ptr<HostTask> hostTaskPointer = std::make_shared<HostTask>(hcclHostTask);
        auto hcclTasks = std::make_shared<HostTasks>();
        hcclTasks->push_back(hostTaskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetHCCLTasks).stubs().will(returnValue(*hcclTasks));
    }

    static void MockGetComputeTasks()
    {
        auto kernelTask = std::make_shared<HostTask>();
        MsprofNodeBasicInfo msprofNodeBasicInfo{};
        MsprofAttrInfo msprofNodeAttrInfo{};
        auto ctx = std::make_shared<MsprofAdditionalInfo>();
        ctx->threadId = 0;
        auto tensorDesc = std::make_shared<ConcatTensorInfo>();
        tensorDesc->tensorNum = 1;
        auto kernelDesc = std::make_shared<OpDesc>();
        kernelDesc->tensorDesc = std::make_shared<ConcatTensorInfo>();
        kernelDesc->nodeDesc = std::make_shared<MsprofCompactInfo>();
        kernelDesc->nodeDesc->data.nodeBasicInfo = msprofNodeBasicInfo;
        kernelDesc->nodeDesc->data.nodeAttrInfo = msprofNodeAttrInfo;
        kernelDesc->ctxId = std::make_shared<MsprofAdditionalInfo>();
        auto kernelOp = std::make_shared<Operator>(kernelDesc, 0, OpType::OPTYPE_COMPUTE);
        kernelTask->op = kernelOp;
        std::shared_ptr<HostTask> computeTaskPointer = kernelTask;
        auto kernelTasks = std::make_shared<HostTasks>();
        kernelTasks->push_back(computeTaskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetComputeTasks).stubs().will(returnValue(*kernelTasks));
    }

    static void MockGetL0ComputeTasks()
    {
        auto kernelTask = std::make_shared<HostTask>();
        MsprofNodeBasicInfo msprofNodeBasicInfo{};
        MsprofAttrInfo msprofNodeAttrInfo{};
        auto ctx = std::make_shared<MsprofAdditionalInfo>();
        ctx->threadId = 0;
        auto tensorDesc = std::make_shared<ConcatTensorInfo>();
        tensorDesc->tensorNum = 1;
        auto kernelDesc = std::make_shared<OpDesc>();
        auto kernelOp = std::make_shared<Operator>(kernelDesc, 0, OpType::OPTYPE_COMPUTE);
        kernelTask->op = kernelOp;
        std::shared_ptr<HostTask> computeTaskPointer = kernelTask;
        auto kernelTasks = std::make_shared<HostTasks>();
        kernelTasks->push_back(computeTaskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetComputeTasks).stubs().will(returnValue(*kernelTasks));
    }

    static void MockGetTasks()
    {
        auto pMiniOpDesc = std::make_shared<HcclSmallOpDesc>(0, 0, nullptr);
        std::shared_ptr<Operator> pointer = std::make_shared<Operator>(pMiniOpDesc, 0, OpType::OPTYPE_HCCL_SMALL);
        HostTask task;
        task.op = pointer;
        std::shared_ptr<HostTask> taskPointer = std::make_shared<HostTask>(task);
        auto tasks = std::make_shared<HostTasks>();
        tasks->push_back(taskPointer);
        MOCKER_CPP(&TreeAnalyzer::GetTasks).stubs().will(returnValue(*tasks));
    }

    void MockGetHcclBigOps() const
    {
        auto trace = new MsprofCompactInfo;
        auto nodeBasicInfo = new MsprofNodeBasicInfo;
        auto nodeAttrInfo = new MsprofAttrInfo;
        auto runtime = new MsprofRuntimeTrack;
        
        trace->data.nodeBasicInfo = *nodeBasicInfo;
        trace->data.nodeAttrInfo = *nodeAttrInfo;
        trace->data.runtimeTrack = *runtime;

        auto hcclOp = new MsprofHcclOPInfo;
        auto hcclOpCompactInfo = new MsprofCompactInfo;
        hcclOpCompactInfo->data.hcclopInfo = *hcclOp;

        auto desc = std::make_shared<HcclBigOpDesc>(1, 1, 1, 1, 1, 1, 1, std::shared_ptr<MsprofCompactInfo>(trace),
                                                    std::shared_ptr<MsprofCompactInfo>(hcclOpCompactInfo), -1);
        auto op = std::make_shared<Operator>(desc, 0, OpType::OPTYPE_HCCL_BIG);
        auto hcclBigOps = std::make_shared<HCCLBigOpDescs>();
        hcclBigOps->emplace_back(op);
        MOCKER_CPP(&TreeAnalyzer::GetHcclBigOps).stubs().will(returnValue(*hcclBigOps));
    }
};

TEST_F(CannDBDumperUtest,
            TestCANNDumperShouldReturnTrueWhenAllDataIsCorrectAndDBFunctionsNormalThenQueryDBShouldReturnRecords)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();

    CANNTraceDBDumper cannTraceDbDumper(".");
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            uint32_t, std::string, std::string, uint32_t, std::string>> hcclOpData;
    hcclOpDBRunner.QueryData("select * from HCCLOP", hcclOpData);
    EXPECT_EQ(hcclOpData.size(), 1);

    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    std::vector<std::tuple<uint32_t,
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, std::string, uint32_t,
            std::string, std::string>> hostTaskData;
    hostTaskDBRunner.QueryData("select * from HostTask", hostTaskData);
    EXPECT_EQ(hostTaskData.size(), 1);

    GEInfoDB geInfoDB;
    std::string opDescDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, geInfoDB.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string,
            std::string, std::string, std::string, std::string, std::string, uint32_t,
            uint32_t, uint32_t>> TaskInfoData;
    opDescDBRunner.QueryData("select * from TaskInfo", TaskInfoData);
    EXPECT_EQ(TaskInfoData.size(), 1);
    EXPECT_EQ(std::get<INPUT_DATA_TYPE_POSITION>(TaskInfoData[0]), "N/A");

    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, std::string,
            double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            std::string, double, std::string, std::string, uint64_t, std::string, uint32_t>> HCCLTaskData;
    hcclOpDBRunner.QueryData("select * from HCCLTask", HCCLTaskData);
    EXPECT_EQ(HCCLTaskData.size(), 1);
    EXPECT_EQ(std::get<GROUP_NAME_POSITION>(HCCLTaskData[0]), "0");
}

TEST_F(CannDBDumperUtest, TestCANNDumperShouldReturnTrueWhenComputeTaskDataIsL0ThenQueryDBShouldReturnRecords)
{
    MockGetHCCLTasks();
    MockGetL0ComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();

    CANNTraceDBDumper cannTraceDbDumper(".");
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);
    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            uint32_t, std::string, std::string, uint32_t, std::string>> hcclOpData;
    hcclOpDBRunner.QueryData("select * from HCCLOP", hcclOpData);
    EXPECT_EQ(hcclOpData.size(), 1);

    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    std::vector<std::tuple<uint32_t,
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, std::string, uint32_t,
            std::string, std::string>> hostTaskData;
    hostTaskDBRunner.QueryData("select * from HostTask", hostTaskData);
    EXPECT_EQ(hostTaskData.size(), 1);

    GEInfoDB geInfoDB;
    std::string opDescDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, geInfoDB.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string,
            std::string, std::string, std::string, std::string, std::string, uint32_t,
            uint32_t, uint32_t>> TaskInfoData;
    opDescDBRunner.QueryData("select * from TaskInfo", TaskInfoData);
    EXPECT_EQ(TaskInfoData.size(), 1);
    EXPECT_EQ(std::get<INPUT_DATA_TYPE_POSITION>(TaskInfoData[0]), "N/A");

    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, std::string,
            double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            std::string, double, std::string, std::string, uint64_t, std::string, int32_t>> HCCLTaskData;
    hcclOpDBRunner.QueryData("select * from HCCLTask", HCCLTaskData);
    EXPECT_EQ(HCCLTaskData.size(), 1);
    EXPECT_EQ(std::get<GROUP_NAME_POSITION>(HCCLTaskData[0]), "0");
}


TEST_F(CannDBDumperUtest,
            TestCANNDumperShouldReturnTrueWhenAllDataIsL0AndDBFunctionsNormalThenQueryDBShouldReturnRecords)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();

    CANNTraceDBDumper cannTraceDbDumper(".");
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    auto computeTasks = treeAnalyzer.GetComputeTasks();
    computeTasks[0]->op->opDesc->tensorDesc = nullptr;
    auto hcclTasks = treeAnalyzer.GetHCCLTasks();
    hcclTasks[0]->op->hcclSmallOpDesc->hcclInfo = nullptr;
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);

    HCCLDB hcclDB;
    std::string hcclTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hcclDB.GetDBName()});
    DBRunner hcclOpDBRunner(hcclTaskDBPath);
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            uint32_t, std::string, std::string, uint32_t, std::string>> hcclOpData;
    hcclOpDBRunner.QueryData("select * from HCCLOP", hcclOpData);
    EXPECT_EQ(hcclOpData.size(), 1);

    RuntimeDB runtimeDB;
    std::string hostTaskDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, runtimeDB.GetDBName()});
    DBRunner hostTaskDBRunner(hostTaskDBPath);
    std::vector<std::tuple<uint32_t,
            uint32_t, uint32_t, uint32_t, std::string, uint32_t, std::string, uint32_t,
            std::string, std::string>> hostTaskData;
    hostTaskDBRunner.QueryData("select * from HostTask", hostTaskData);
    EXPECT_EQ(hostTaskData.size(), 1);

    GEInfoDB geInfoDB;
    std::string opDescDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, geInfoDB.GetDBName()});
    DBRunner opDescDBRunner(opDescDBPath);
    std::vector<std::tuple<uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t, std::string, uint32_t,
            std::string, uint32_t, uint32_t, double, uint32_t, uint32_t, std::string,
            std::string, std::string, std::string, std::string, std::string, uint32_t,
            uint32_t, uint32_t>> TaskInfoData;
    opDescDBRunner.QueryData("select * from TaskInfo", TaskInfoData);
    EXPECT_EQ(TaskInfoData.size(), 1);
    EXPECT_EQ(std::get<INPUT_DATA_TYPE_POSITION>(TaskInfoData[0]), "N/A");

    std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string, uint32_t, std::string,
            double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
            std::string, double, std::string, std::string, uint64_t, std::string, int32_t>> HCCLTaskData;
    hcclOpDBRunner.QueryData("select * from HCCLTask", HCCLTaskData);
    EXPECT_EQ(HCCLTaskData.size(), 1);
    EXPECT_EQ(std::get<GROUP_NAME_POSITION>(HCCLTaskData[0]), "N/A");
}

TEST_F(CannDBDumperUtest, TestCANNDumperShouldReturnFalseWhenInsertDataToDBFailed)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_FALSE(ret);
}


TEST_F(CannDBDumperUtest, TestCANNDumperShouldRetrunTrueWhenDataIsEmpty)
{
    MOCKER_CPP(&TreeAnalyzer::GetHCCLTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetComputeTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetTasks).stubs().will(returnValue(*std::make_shared<HostTasks>()));
    MOCKER_CPP(&TreeAnalyzer::GetHcclBigOps).stubs().will(returnValue(*std::make_shared<HCCLBigOpDescs>()));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_TRUE(ret);
}

TEST_F(CannDBDumperUtest, TestCANNDumperShouldReturnFalseWhenCreateTableFailed)
{
    MockGetHCCLTasks();
    MockGetComputeTasks();
    MockGetTasks();
    MockGetHcclBigOps();
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    TreeAnalyzer treeAnalyzer(treeNode, THREAD_ID);
    bool ret = cannTraceDbDumper.DumpData(treeAnalyzer);
    EXPECT_FALSE(ret);
}

TEST_F(CannDBDumperUtest, TestAddTensorShapeInfoSuccess)
{
    CANNTraceDBDumper cannTraceDbDumper(".");
    std::shared_ptr<ConcatTensorInfo> tensorDescPtr = std::make_shared<ConcatTensorInfo>();
    tensorDescPtr->tensorNum = 2; // 2ul
    MsrofTensorData tensorInput = {
        0, 1, 1, {1, 2, 3, 0, 4}
    };
    MsrofTensorData tensorOutput = {
        1, 1, 1, {5, 6, 7, 0, 4}
    };
    tensorDescPtr->tensorData = {tensorInput, tensorOutput};
    MsprofNodeBasicInfo nodeBasicInfo;
    CANNTraceDBDumper::TaskInfoData data;
    std::shared_ptr<HostTask> hostTaskPtr = std::make_shared<HostTask>();
    auto kernelDesc = std::make_shared<OpDesc>();
    auto kernelOp = std::make_shared<Operator>(kernelDesc, 0, OpType::OPTYPE_COMPUTE);
    hostTaskPtr->op = kernelOp;
    cannTraceDbDumper.AddTensorShapeInfo(tensorDescPtr, nodeBasicInfo, data, hostTaskPtr);
    EXPECT_EQ(std::get<13>(data[0]), 2); // 13 2
    EXPECT_EQ(std::get<16>(data[0]), "\"1,2,3\"");
    EXPECT_EQ(std::get<19>(data[0]), "\"5,6,7\"");
}

TEST_F(CannDBDumperUtest, TestGetFormat_UINT32_MAX)
{
    uint32_t oriFormat = UINT32_MAX;
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    std::string result = cannTraceDbDumper.GetFormat(oriFormat);
    EXPECT_EQ(result, "UNDEFINED");
}

TEST_F(CannDBDumperUtest, TestGetFormat_SubFormat_Zero)
{
    uint32_t oriFormat = 0x00000000;
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    std::string result = cannTraceDbDumper.GetFormat(oriFormat);
    EXPECT_EQ(result, "NCHW");
}

TEST_F(CannDBDumperUtest, TestGetFormat_SubFormat_One)
{
    uint32_t oriFormat = 0x000100;
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    std::string result = cannTraceDbDumper.GetFormat(oriFormat);
    EXPECT_EQ(result, "NCHW:1");
}

TEST_F(CannDBDumperUtest, TestAddTaskInfoOpIsNullWhenProfLevel0)
{
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto hostTaskPtr = std::make_shared<HostTask>();
    hostTaskPtr->op = nullptr;
    hostTaskPtr->kernelName = 0;
    CANNTraceDBDumper::TaskInfoData taskInfoData;
    bool isLevel0 = true;
    cannTraceDbDumper.AddTaskInfo(hostTaskPtr, taskInfoData, isLevel0);
    EXPECT_EQ(taskInfoData.size(), 0ul);
    hostTaskPtr->kernelName = 1;
    cannTraceDbDumper.AddTaskInfo(hostTaskPtr, taskInfoData, isLevel0);
    EXPECT_EQ(taskInfoData.size(), 1ul);
}

TEST_F(CannDBDumperUtest, TestAddTaskInfoOpIsNullWhenProfLevel1)
{
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto hostTaskPtr = std::make_shared<HostTask>();
    hostTaskPtr->op = nullptr;
    hostTaskPtr->kernelName = 0;
    CANNTraceDBDumper::TaskInfoData taskInfoData;
    bool isLevel0 = false;
    cannTraceDbDumper.AddTaskInfo(hostTaskPtr, taskInfoData, isLevel0);
    EXPECT_EQ(taskInfoData.size(), 0ul);
    hostTaskPtr->kernelName = 1;
    cannTraceDbDumper.AddTaskInfo(hostTaskPtr, taskInfoData, isLevel0);
    EXPECT_EQ(taskInfoData.size(), 1ul);
}

TEST_F(CannDBDumperUtest, TestDumpGeFusionOps)
{
    CANNTraceDBDumper cannTraceDbDumper(TEST_DB_FILE_PATH);
    auto profFusionOpInfoPtr = std::make_shared<ProfFusionOpInfo>(ProfFusionOpInfo{
        .opName = 12345,
        .fusionOpNum = 2,
        .inputMemsize = 1024,
        .outputMemsize = 2048,
        .weightMemSize = 4096,
        .workspaceMemSize = 8192,
        .totalMemSize = 16384,
        .fusionOpId = {1, 2}
    });
    std::shared_ptr<GeFusionOpInfo> info = std::make_shared<GeFusionOpInfo>(0, profFusionOpInfoPtr);
    GeFusionOpInfos geFusionOpInfos = {info};
    cannTraceDbDumper.DumpGeFusionOps(geFusionOpInfos);
    EXPECT_TRUE(cannTraceDbDumper.result_);
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    cannTraceDbDumper.result_ = true;
    cannTraceDbDumper.DumpGeFusionOps(geFusionOpInfos);
    EXPECT_FALSE(cannTraceDbDumper.result_);
    MOCKER_CPP(&DBRunner::CreateTable).reset();
    MOCKER_CPP(&DBRunner::InsertData<>).stubs().will(returnValue(false));
    cannTraceDbDumper.DumpGeFusionOps(geFusionOpInfos);
    EXPECT_FALSE(cannTraceDbDumper.result_);
    MOCKER_CPP(&DBRunner::InsertData<>).reset();
}