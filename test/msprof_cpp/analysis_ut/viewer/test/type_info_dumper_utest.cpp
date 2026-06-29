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

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/domain/services/persistence/host/type_info_db_dumper.h"

using namespace Analysis::Utils;
using namespace Analysis::Application;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using HashData = std::unordered_map<uint64_t, std::string>;
const std::string TEST_DB_FILE_PATH = "./sqlite";
const std::string TYPE_VALUE = "123";

class TypeInfoDumperUtest : public testing::Test {
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

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperWithCompletedHashDataShouldInsertSuccessAndCanBeQueried)
{
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_TRUE(res);

    HashDB hashDb;
    std::string runtimeDBPath = Utils::File::PathJoin({TEST_DB_FILE_PATH, hashDb.GetDBName()});
    DBRunner runtimeDBRunner(runtimeDBPath);
    std::vector<std::tuple<std::string, std::string>> data;
    runtimeDBRunner.QueryData("select * from TypeHashInfo", data);
    EXPECT_EQ(data.size(), ANALYSIS_ERROR);
}

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_FALSE(res);
}

TEST_F(TypeInfoDumperUtest, TestTypeInfoDumperShouldReturnFalseWhenCannotInsertTable)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(true));
    TypeInfoDBDumper TypeInfoDBDumper(".");
    TypeInfoData typeInfoData{
            {1, {{1, TYPE_VALUE}}}
    };
    auto res = TypeInfoDBDumper.DumpData(typeInfoData);
    EXPECT_FALSE(res);
}