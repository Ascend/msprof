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
#include "analysis/csrc/domain/entities/tree/include/event.h"
#include "analysis/csrc/domain/services/persistence/host/memcpy_info_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using ApiData = std::vector<std::shared_ptr<Event>>;
using HostTaskData = std::vector<
        std::tuple<
            uint32_t,
            int32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            uint32_t,
            std::string,
            std::string,
            uint32_t,
            uint64_t,
            int64_t,
            uint32_t>>;
const std::string MEMCPY_INFO_DIR = "./memcpy_info";
const std::string PROF0 = File::PathJoin({MEMCPY_INFO_DIR, "PROF_0"});
const std::string HOST_PATH = File::PathJoin({PROF0, "host"});
const std::string SQLITE_PATH = File::PathJoin({HOST_PATH, "sqlite"});
const std::string HOST_TASK_TABLE_NAME = "HostTask";
const uint32_t RUNTIME_LEVEL_NUMBER = 5000;
const HostTaskData DATA_A{{4294967295, -1, 30, 4, 4294967295, 0, "MEMCPY_ASYNC", "aclnn", 0, 52851917869135, 2000, 1},
                          {4294967295, -1, 30, 3, 4294967295, 0, "MEMCPY_ASYNC", "aclnn", 0, 52851917869135, 2000, 1},
                          {4294967295, -1, 30, 10, 4294967295, 0, "MEMCPY_ASYNC", "aclnn", 0, 52851917869135, 2000, 2}
};
class MemcpyInfoDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(MEMCPY_INFO_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(HOST_PATH));
        EXPECT_TRUE(File::CreateDir(SQLITE_PATH));
        EXPECT_TRUE(CreateRuntimeDB(SQLITE_PATH, DATA_A));
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(MEMCPY_INFO_DIR, 0);
    }

    static bool CreateRuntimeDB(const std::string &sqlitePath, const HostTaskData &data)
    {
        std::shared_ptr<RuntimeDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, RuntimeDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        auto cols = database->GetTableCols(HOST_TASK_TABLE_NAME);
        EXPECT_TRUE(dbRunner->CreateTable(HOST_TASK_TABLE_NAME, cols));
        EXPECT_TRUE(dbRunner->InsertData(HOST_TASK_TABLE_NAME, data));
        return true;
    }
};

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldInsertDataCorrectly)
{
    MemcpyInfoDumper memcpyInfoDumper(HOST_PATH);
    auto info1 = std::make_shared<ParserCompactInfo>();
    info1->level = RUNTIME_LEVEL_NUMBER;
    info1->threadId = 1; // threadId 1
    info1->timeStamp = 52851917869135;  // 时间戳 52851917869135
    info1->dataLen = 24;  // dataLen 24
    info1->data.memcpyInfo.dataSize = 83886080;  // dataSize 83886080
    info1->data.memcpyInfo.memcpyDirection = 0;
    info1->data.memcpyInfo.maxSize = 67108864;  // maxSize 67108864
    auto info2 = std::make_shared<ParserCompactInfo>();
    info2->level = RUNTIME_LEVEL_NUMBER;
    info2->threadId = 2;  // threadId 2
    info2->timeStamp = 52851917869135;  // 时间戳 52851917869135
    info2->dataLen = 24;  // dataLen 24
    info2->data.memcpyInfo.dataSize = 22282240;  // dataSize 22282240
    info2->data.memcpyInfo.memcpyDirection = 0;
    info2->data.memcpyInfo.maxSize = 67108864;  // maxSize 67108864
    std::vector<std::shared_ptr<ParserCompactInfo>> memcpyInfos;
    memcpyInfos.push_back(info1);
    memcpyInfos.push_back(info2);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    EXPECT_TRUE(res);
    RuntimeDB runtimeDB;
    std::string runtimeDBPath = Utils::File::PathJoin({SQLITE_PATH, runtimeDB.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    MemcpyInfoData data;
    runtimeDBRunner.QueryData("select * from MemcpyInfo", data);
    ASSERT_EQ(data.size(), 3);  // 表里面生成数据的数量 3
    EXPECT_EQ(std::get<0>(data[0]), 30);  // streamId: 30
    EXPECT_EQ(std::get<1>(data[0]), 0);  // 第1列
    EXPECT_EQ(std::get<2>(data[0]), 3);  // 第2列, taskid是3
    EXPECT_EQ(std::get<4>(data[0]), 0);  // 第4列
    EXPECT_EQ(std::get<5>(data[0]), 67108864);  // 第5列, 67108864
    EXPECT_EQ(std::get<6>(data[0]), 0);  // 第6列
    EXPECT_EQ(std::get<2>(data[1]), 4);  // 第2列, taskid是4
    EXPECT_EQ(std::get<5>(data[1]), 16777216);  // 第5列, 16777216
    EXPECT_EQ(std::get<2>(data[2]), 10);  // taskid是10
    EXPECT_EQ(std::get<5>(data[2]), 22282240);  // 第5列, 22282240
}

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    MemcpyInfoDumper memcpyInfoDumper(HOST_PATH);
    auto info = std::make_shared<ParserCompactInfo>();
    std::vector<std::shared_ptr<ParserCompactInfo>> memcpyInfos;
    memcpyInfos.push_back(info);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    ASSERT_FALSE(res);
}

TEST_F(MemcpyInfoDumperUtest, TestMemcpyInfoDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    MemcpyInfoDumper memcpyInfoDumper(HOST_PATH);
    std::vector<std::shared_ptr<ParserCompactInfo>> memcpyInfos;
    auto info = std::make_shared<ParserCompactInfo>();
    memcpyInfos.push_back(info);
    auto res = memcpyInfoDumper.DumpData(memcpyInfos);
    ASSERT_FALSE(res);
}