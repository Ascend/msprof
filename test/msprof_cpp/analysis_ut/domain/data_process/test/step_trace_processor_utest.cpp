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
#include "analysis/csrc/domain/data_process/ai_task/step_trace_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./step";
const std::string DEVICE = "device_0";
const std::string DB_NAME = "trace.db";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
}
using ReduceDataType = std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t>>;
using GetNextDataType = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t>>;
using TraceDataType = std::vector<std::tuple<uint16_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                             uint64_t, uint64_t, uint64_t>>;
ReduceDataType reduce{{0, 2, 1, 313056236532, 313055200207, 313055459640},
                      {0, 2, 2, 318016202377, 318015356897, 318015374534}};
GetNextDataType next{{2, 1, 313055463171, 313055464468}, {2, 2, 318015397631, 318015399357}};
TraceDataType trace{{0, 1, 1, 306258517346, 306258521644, 306258522140, 292028, 4298, 496, 0},
                    {0, 2, 1, 313055197979, 313056220356, 313056236532, 1044232, 1022377, 16176, 6796675839},
                    {0, 3, 1, 0, 317098454510, 317099294827, 75569326, 0, 840317, 0}};

class StepTraceProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE})));
        EXPECT_TRUE(CreateReduceData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), reduce, "all_reduce"));
        EXPECT_TRUE(CreateNextData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), next, "get_next"));
        EXPECT_TRUE(CreateTraceData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), trace, "training_trace"));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }

    static bool CreateReduceData(const std::string &dbPath, ReduceDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
    static bool CreateNextData(const std::string &dbPath, GetNextDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
    static bool CreateTraceData(const std::string &dbPath, TraceDataType &data, const std::string &&tableName)
    {
        std::shared_ptr<TraceDB> database;
        MAKE_SHARED_RETURN_VALUE(database, TraceDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        auto cols = database->GetTableCols(tableName);
        dbRunner->CreateTable(tableName, cols);
        dbRunner->InsertData(tableName, data);
        return true;
    }
};

TEST_F(StepTraceProcessorUTest, ShouldReturnOKWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = StepTraceProcessor(PROF_PATH_A);
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "0"},
        {"pid", "10"},
        {"hostCntvct", "65177261204177"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"hostMonotonic", "651599377155020"},
    };
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    auto resReduce = dataInventory.GetPtr<std::vector<AllReduceData>>();
    auto resNext = dataInventory.GetPtr<std::vector<GetNextData>>();
    auto resTrace = dataInventory.GetPtr<std::vector<TrainTraceData>>();
    EXPECT_EQ(2ul, resReduce->size());
    EXPECT_EQ(2ul, resNext->size());
    EXPECT_EQ(3ul, resTrace->size());
}

TEST_F(StepTraceProcessorUTest, ShouldReturnFalseWhenReserveException)
{
    DataInventory dataInventory;
    auto processor = StepTraceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    Analysis::Test::StubReserveFailureForVector<std::vector<AllReduceData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    Analysis::Test::ResetReserveFailureForVector<std::vector<AllReduceData>>();
    Analysis::Test::StubReserveFailureForVector<std::vector<GetNextData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    Analysis::Test::ResetReserveFailureForVector<std::vector<GetNextData>>();
    Analysis::Test::StubReserveFailureForVector<std::vector<TrainTraceData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    Analysis::Test::ResetReserveFailureForVector<std::vector<TrainTraceData>>();
}

TEST_F(StepTraceProcessorUTest, ShouldReturnFalseWhenCheckFailed)
{
    DataInventory dataInventory;
    auto processor = StepTraceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(HOST_ID)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Utils::FileReader::Check).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_STEP_TRACE));
}
