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
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Test;
using HashMap = std::unordered_map<std::string, std::string>;
namespace {
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t STRING_NUM = 18;
const std::string COMPUTE_TASK_PATH = "./compute_task_path";
const std::string HOST_SUFFIX = "host";
const std::string DB_SUFFIX = "ge_info.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({COMPUTE_TASK_PATH,
                                                "PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string TABLE_NAME = "TaskInfo";

using GeInfoFormat = std::vector<std::tuple<uint32_t, std::string, int32_t, int32_t, uint32_t, uint32_t, std::string,
        std::string, std::string, int32_t, uint32_t, double, uint32_t, uint32_t,
        std::string, std::string, std::string, std::string, std::string,
        std::string, int32_t, uint32_t, std::string, std::string, std::string, std::string>>;

GeInfoFormat DATA_A{{4294967295, "aclnnMm_MatMulCommon_MatMulV2", 2, 1, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                            3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                            "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"1000,1000\"", 0, 0, "NO", "N/A", "N/A", "N/A"},
                    {4294967295, "trans_TransData_0", 2, 2, 35, 0, "1", "AI_CORE", "TransData", -1,
                            250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                            "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO", "N/A", "N/A", "N/A"}};
}

class ComputeTaskInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(COMPUTE_TASK_PATH)) {
            File::RemoveDir(COMPUTE_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMPUTE_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX})));
        CreateTaskInfo(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(COMPUTE_TASK_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateTaskInfo(const std::string& dbPath, GeInfoFormat data)
    {
        std::shared_ptr<GEInfoDB> database;
        MAKE_SHARED0_RETURN_VOID(database, GEInfoDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

static void InitHashMap(DataInventory& dataInventory)
{
    HashMap hashMap;
    hashMap.emplace("111", "TEST");
    std::shared_ptr<std::unordered_map<std::string, std::string>> res;
    MAKE_SHARED_RETURN_VOID(res, HashMap, std::move(hashMap));
    dataInventory.Inject(res);
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    InitHashMap(dataInventory);
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    DataInventory dataInventory;
    InitHashMap(dataInventory);
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenGeHashNotExists)
{
    DataInventory dataInventory;
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();

    std::vector<TaskInfoData> oriData;
    MOCKER_CPP(&ComputeTaskInfoProcessor::LoadData)
    .stubs()
    .will(returnValue(oriData));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
    MOCKER_CPP(&ComputeTaskInfoProcessor::LoadData).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<TaskInfoData>)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMPUTE_TASK_INFO));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<TaskInfoData>).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestProcessShouldReturnFalseWhenLoadDataFailed)
{
    auto processor = ComputeTaskInfoProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    StubReserveFailureForVector<std::vector<TaskInfoData>>();
    EXPECT_FALSE(processor.Process(dataInventory));
    ResetReserveFailureForVector<std::vector<TaskInfoData>>();

    DBInfo geInfoDB("ge_info.db", "TaskInfo");
    geInfoDB.dbRunner = nullptr;
    EXPECT_FALSE(processor.Process(dataInventory));
}
