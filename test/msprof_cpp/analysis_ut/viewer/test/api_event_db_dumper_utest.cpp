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
#include "analysis/csrc/domain/services/persistence/host/api_event_db_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Application;
using namespace Analysis::Domain;
using ApiData = std::vector<std::shared_ptr<Event>>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const uint32_t ACL_LEVEL_NUMBER = 20000;
const uint32_t PYTORCH_LEVEL_NUMBER = 30000;
const uint32_t TWO_BYTES = 16;
class ApiEventDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_FILE_PATH);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_FILE_PATH, 0);
    }
};

TEST_F(ApiEventDBDumperUtest, TestApiEventDBDumperShouldInsertDataCorrectly)
{
    ApiEventDBDumper apiEventDbDumper(".");
    auto apiPtr1 = std::make_shared<MsprofApi>();
    uint64_t endTime1 = 4;
    apiPtr1->type = 1 << TWO_BYTES;
    apiPtr1->level = ACL_LEVEL_NUMBER;
    apiPtr1->threadId = 1;
    apiPtr1->beginTime = 1;
    apiPtr1->endTime = endTime1;
    apiPtr1->itemId = 0;

    EventInfo info1(EventType::EVENT_TYPE_API, MSPROF_REPORT_ACL_LEVEL, 1, endTime1);
    auto even1 = std::make_shared<Event>(apiPtr1, info1);

    ApiData apiData = {even1};

    auto res = apiEventDbDumper.DumpData(apiData);
    EXPECT_TRUE(res);

    ApiEventDB apiEventDB;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, apiEventDB.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string, std::string, uint32_t, std::string, uint64_t, uint64_t>> data;
    runtimeDBRunner.QueryData("select * from ApiData", data);
    // 如果level不是acl，则会直接把id置为0
    const int col2 = 2;
    const int col3 = 3;
    const int col4 = 4;
    const int col5 = 5;
    const int col6 = 6;
    EXPECT_EQ(std::get<0>(data[0]), "ACL_OP");
    EXPECT_EQ(std::get<1>(data[0]), "65536");
    EXPECT_EQ(std::get<col2>(data[0]), "acl");
    EXPECT_EQ(std::get<col3>(data[0]), 1);
    EXPECT_EQ(std::get<col4>(data[0]), "0");
    EXPECT_EQ(std::get<col5>(data[0]), 1);
    EXPECT_EQ(std::get<col6>(data[0]), endTime1);
}

TEST_F(ApiEventDBDumperUtest, TestApiEventDBDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    ApiEventDBDumper apiEventDbDumper(".");
    auto apiPtr1 = std::make_shared<MsprofApi>();
    apiPtr1->type = 1 << TWO_BYTES;
    apiPtr1->level = ACL_LEVEL_NUMBER;
    EventInfo info1(EventType::EVENT_TYPE_API, MSPROF_REPORT_ACL_LEVEL, 1, 1);
    auto even1 = std::make_shared<Event>(apiPtr1, info1);

    ApiData apiData{even1};
    auto res = apiEventDbDumper.DumpData(apiData);
    ASSERT_FALSE(res);
}

TEST_F(ApiEventDBDumperUtest, TestApiEventDBDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    ApiEventDBDumper apiEventDbDumper(".");
    auto apiPtr1 = std::make_shared<MsprofApi>();
    apiPtr1->type = 1 << TWO_BYTES;
    apiPtr1->level = ACL_LEVEL_NUMBER;

    EventInfo info1(EventType::EVENT_TYPE_API, MSPROF_REPORT_ACL_LEVEL, 1, 1);
    auto even1 = std::make_shared<Event>(apiPtr1, info1);

    ApiData apiData{even1};
    auto res = apiEventDbDumper.DumpData(apiData);
    ASSERT_FALSE(res);
}