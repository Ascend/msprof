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
#include "nlohmann/json.hpp"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/timeline/json_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/application/include/export_mode_enum.h"


using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Application;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Domain;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./export_test";
const std::string DEVICE = "device_0";
const std::string DB_NAME = "trace.db";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, Analysis::Common::OUTPUT_PATH});
const std::string REPORTS_JSON = "reports.json";
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

class ExportManagerUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE, SQLITE})));
        EXPECT_TRUE(CreateReduceData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), reduce, "all_reduce"));
        EXPECT_TRUE(CreateNextData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), next, "get_next"));
        EXPECT_TRUE(CreateTraceData(File::PathJoin({PROF_PATH, DEVICE, SQLITE, DB_NAME}), trace, "training_trace"));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
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
    static void CreateReportsJson(nlohmann::json &reports)
    {
        FileWriter reportsWriter(File::PathJoin({BASE_PATH, REPORTS_JSON}));
        reportsWriter.WriteText(reports.dump());
    }
protected:
    DataInventory dataInventory_;
};

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenPathIsInvalid)
{
    std::string path = "/home/prof_0";
    ExportManager manager(path);
    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenContextInitFail)
{
    ExportManager manager(PROF_PATH);
    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessFail)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessSuccessWithDB)
{
    ExportManager manager(PROF_PATH);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::DB}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenProcessSuccessWithReportJson)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", true},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonValueError)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", "true"},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonProcessNotExist)
{
    nlohmann::json reports = {
        {"json_processxxxx", {}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenReportJsonPathNotExist)
{
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenProcessDataGetDataProcessByNameFailed)
{
    ExportManager manager(PROF_PATH, REPORTS_JSON);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    std::shared_ptr<DataProcessor> processor = nullptr;
    MOCKER_CPP(&DataProcessorFactory::GetDataProcessByName).stubs().will(returnValue(processor));
    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
    MOCKER_CPP(&DataProcessorFactory::GetDataProcessByName).reset();
}

TEST_F(ExportManagerUTest, ShouldReturnFalseWhenCheckOutputPathFailed)
{
    ExportManager manager(PROF_PATH, REPORTS_JSON);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));

    MOCKER_CPP(&File::Exist).
    stubs()
    .will(returnValue(false));

    MOCKER_CPP(&File::CreateDir)
    .stubs()
    .will(returnValue(false));

    EXPECT_FALSE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
    MOCKER_CPP(&File::Exist).reset();
    MOCKER_CPP(&File::CreateDir).reset();
}

TEST_F(ExportManagerUTest, ShouldReturnTrueWhenAnalysisReportJsonFailed)
{
    nlohmann::json reports = {
        {"json_process", {
            {"cann", true},
            {"ascend", true},
            {"freq", true},
            {"hbm", false}}
        }};
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    const int analysisError = 1;

    MOCKER_CPP(&FileReader::ReadJson)
    .stubs()
    .will(returnValue(analysisError));

    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));

    MOCKER_CPP(&FileReader::ReadJson).reset();
}

TEST_F(ExportManagerUTest, ShouldAddLowPowerWhenFreqIsTrue)
{
    nlohmann::json reports = {
        {"json_process", {
            {"freq", true}
        }}
    };
    CreateReportsJson(reports);
    auto jsonPath = File::PathJoin({BASE_PATH, REPORTS_JSON});
    ExportManager manager(PROF_PATH, jsonPath);
    
    // mock Init()
    MOCKER_CPP(&Context::Load).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::IsChipV6).stubs().will(returnValue(false));
    
    EXPECT_TRUE(manager.Run({Analysis::Application::ExportMode::TIMELINE}));
}