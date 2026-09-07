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

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/persistence/host/static_op_mem_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using HashData = Analysis::Domain::Host::Cann::HashData;

namespace
{
const std::string TEST_DB_DIR = "./sqlite";

std::shared_ptr<ParserAdditionalInfo> MakeStaticOpMemInfo(int64_t size, uint64_t opName, uint64_t dynOpName,
                                                         uint64_t lifeEnd)
{
    auto info = std::make_shared<ParserAdditionalInfo>();
    info->staticOpMem.size = size;
    info->staticOpMem.opName = opName;
    info->staticOpMem.lifeStart = 7;
    info->staticOpMem.lifeEnd = lifeEnd;
    info->staticOpMem.totalAllocateMemory = 8192;
    info->staticOpMem.dynOpName = dynOpName;
    info->staticOpMem.graphId = 3;
    return info;
}
}

class StaticOpMemDBDumperUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        File::CreateDir(TEST_DB_DIR);
        HashData::GetInstance().Clear();
    }

    void TearDown() override
    {
        HashData::GetInstance().Clear();
        File::RemoveDir(TEST_DB_DIR, 0);
    }
};

TEST_F(StaticOpMemDBDumperUTest, GenerateDataShouldApplyHashSpecialValueAndUnitConversions)
{
    const uint64_t opHash = 101;
    const uint64_t dynHash = 102;
    HashData::GetInstance().GetAll()[opHash] = "Add";
    HashData::GetInstance().GetAll()[dynHash] = "Root";
    StaticOpMemInfos infos = {
        MakeStaticOpMemInfo(1536, opHash, dynHash, 4294967294ULL),
        MakeStaticOpMemInfo(-1024, 0, 0, 8),
        MakeStaticOpMemInfo(64, 999, 998, 9),
    };

    StaticOpMemDBDumper dumper(".");
    auto data = dumper.GenerateData(infos);

    ASSERT_EQ(data.size(), 3U);
    EXPECT_EQ(std::get<0>(data[0]), "Add");
    EXPECT_EQ(std::get<1>(data[0]), "Root");
    EXPECT_EQ(std::get<2>(data[0]), 3U);
    EXPECT_EQ(std::get<3>(data[0]), 7U);
    EXPECT_EQ(std::get<4>(data[0]), 4294967295ULL);
    EXPECT_DOUBLE_EQ(std::get<5>(data[0]), 1.5);
    EXPECT_EQ(std::get<0>(data[1]), "TOTAL");
    EXPECT_EQ(std::get<1>(data[1]), "0");
    EXPECT_DOUBLE_EQ(std::get<5>(data[1]), -1.0);
    EXPECT_EQ(std::get<0>(data[2]), "999");
    EXPECT_EQ(std::get<1>(data[2]), "998");
    EXPECT_DOUBLE_EQ(std::get<5>(data[2]), 0.0625);
}

TEST_F(StaticOpMemDBDumperUTest, DumpDataShouldCreatePythonCompatibleTable)
{
    StaticOpMemInfos infos = {MakeStaticOpMemInfo(2048, 0, 0, 8)};
    StaticOpMemDBDumper dumper(".");

    ASSERT_TRUE(dumper.DumpData(infos));
    StaticOpMemDB database;
    DBRunner runner(File::PathJoin({".", "sqlite", database.GetDBName()}));
    StaticOpMemDumpData data;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM StaticOpMem", data));
    ASSERT_EQ(data.size(), 1U);
    EXPECT_EQ(std::get<0>(data[0]), "TOTAL");
    EXPECT_EQ(std::get<1>(data[0]), "0");
    EXPECT_DOUBLE_EQ(std::get<5>(data[0]), 2.0);
}
