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

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/include/data_processor_factory.h"
#include "analysis/csrc/domain/data_process/system/page_fault_processor.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Application;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Test;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./page_fault_path";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string DEVICE = "device_0";
const std::string DB_NAME = "soc_pmu.db";
const std::string TABLE_NAME = "PageFault";
const std::string SQLITE_PATH = File::PathJoin({PROF_PATH, DEVICE, SQLITE});
const std::string DB_PATH = File::PathJoin({SQLITE_PATH, DB_NAME});
const std::string HOST_SQLITE_PATH = File::PathJoin({PROF_PATH, HOST, SQLITE});
const std::string TASK_INFO_DB_PATH = File::PathJoin({HOST_SQLITE_PATH, "ge_info.db"});
const std::string TASK_INFO_TABLE = "TaskInfo";

const TableColumns PAGE_FAULT_TABLE_COLS = {
    {"task_type", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"event_lists", SQL_TEXT_TYPE},
};

OriPageFaultData PAGE_FAULT_DATA{
    {7, 15, 31, "3,0"},
    {9, 17, 63, "1"},
};

using TaskInfoDataFormat = std::vector<std::tuple<std::string, uint32_t, uint32_t, uint16_t>>;
const TableColumns TASK_INFO_TABLE_COLS = {
    {"op_name", SQL_TEXT_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"device_id", SQL_INTEGER_TYPE},
};

TaskInfoDataFormat TASK_INFO_DATA{
    {"Conv2D", 15, 31, 0},
    {"Add", 17, 63, 0},
};
}

class PageFaultProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE})));
        EXPECT_TRUE(File::CreateDir(SQLITE_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, HOST})));
        EXPECT_TRUE(File::CreateDir(HOST_SQLITE_PATH));
        CreatePageFaultData(DB_PATH, PAGE_FAULT_DATA);
        CreateTaskInfoData(TASK_INFO_DB_PATH, TASK_INFO_DATA);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
    }

    static void CreatePageFaultData(const std::string& dbPath, const OriPageFaultData& data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        dbRunner->CreateTable(TABLE_NAME, PAGE_FAULT_TABLE_COLS);
        dbRunner->InsertData(TABLE_NAME, data);
    }

    static void CreateTaskInfoData(const std::string& dbPath, const TaskInfoDataFormat& data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        dbRunner->CreateTable(TASK_INFO_TABLE, TASK_INFO_TABLE_COLS);
        dbRunner->InsertData(TASK_INFO_TABLE, data);
    }
};

TEST_F(PageFaultProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = PageFaultProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    auto res = dataInventory.GetPtr<std::vector<PageFaultData>>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(2ul, res->size());
    EXPECT_EQ(0u, res->at(0).deviceId);
    EXPECT_EQ(7u, res->at(0).taskType);
    EXPECT_EQ(15u, res->at(0).streamId);
    EXPECT_EQ(31u, res->at(0).taskId);
    EXPECT_EQ(3u, res->at(0).pageFaultNum);
    EXPECT_EQ("Conv2D", res->at(0).opName);
}

TEST_F(PageFaultProcessorUTest, ShouldReturnTrueWhenSourceTableNotExist)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, DB_PATH);
    dbRunner->DropTable(TABLE_NAME);

    DataInventory dataInventory;
    auto processor = PageFaultProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<PageFaultData>>());

    CreatePageFaultData(DB_PATH, PAGE_FAULT_DATA);
}

TEST_F(PageFaultProcessorUTest, ShouldFillNAWhenTaskInfoNotMatched)
{
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, TASK_INFO_DB_PATH);
    dbRunner->DropTable(TASK_INFO_TABLE);
    TaskInfoDataFormat taskInfoData{
        {"Conv2D", 15, 31, 0},
    };
    CreateTaskInfoData(TASK_INFO_DB_PATH, taskInfoData);

    DataInventory dataInventory;
    auto processor = PageFaultProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));

    auto res = dataInventory.GetPtr<std::vector<PageFaultData>>();
    ASSERT_NE(nullptr, res);
    ASSERT_EQ(2ul, res->size());
    EXPECT_EQ("Conv2D", res->at(0).opName);
    EXPECT_EQ(NA, res->at(1).opName);

    dbRunner->DropTable(TASK_INFO_TABLE);
    CreateTaskInfoData(TASK_INFO_DB_PATH, TASK_INFO_DATA);
}

TEST_F(PageFaultProcessorUTest, ShouldReturnTrueWhenNoDbExists)
{
    const std::string testProfPath = File::PathJoin({BASE_PATH, "test"});
    const std::string testSqlitePath = File::PathJoin({testProfPath, "device_1", SQLITE});
    EXPECT_TRUE(File::CreateDir(testProfPath));
    EXPECT_TRUE(File::CreateDir(File::PathJoin({testProfPath, "device_1"})));
    EXPECT_TRUE(File::CreateDir(testSqlitePath));

    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));

    auto processor = PageFaultProcessor(testProfPath);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<PageFaultData>>());

    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(PageFaultProcessorUTest, ShouldReturnFalseWhenProcessFailed)
{
    DataInventory dataInventory;
    auto processor = PageFaultProcessor(PROF_PATH);

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PageFaultData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<PageFaultData>).reset();

    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(PageFaultProcessorUTest, ShouldReturnEmptyWhenDbRunnerIsNull)
{
    auto processor = PageFaultProcessor(PROF_PATH);
    DBInfo socPmuDB(DB_NAME, TABLE_NAME);
    socPmuDB.dbRunner = nullptr;
    EXPECT_EQ(0ul, processor.LoadData(socPmuDB, "").size());
}

TEST_F(PageFaultProcessorUTest, ShouldReturnTrueWhenReserveFailed)
{
    DataInventory dataInventory;
    auto processor = PageFaultProcessor(PROF_PATH);
    StubReserveFailureForVector<std::vector<PageFaultData>>();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_PAGE_FAULT));
    EXPECT_EQ(nullptr, dataInventory.GetPtr<std::vector<PageFaultData>>());
    ResetReserveFailureForVector<std::vector<PageFaultData>>();
}

TEST_F(PageFaultProcessorUTest, ShouldCreateProcessorFromFactory)
{
    auto processor = DataProcessorFactory::GetDataProcessByName(PROF_PATH, PROCESSOR_NAME_PAGE_FAULT);
    EXPECT_NE(nullptr, processor);
}
