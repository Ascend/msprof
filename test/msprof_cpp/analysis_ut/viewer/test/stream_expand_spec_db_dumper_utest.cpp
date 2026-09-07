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

#include "analysis/csrc/domain/services/persistence/host/stream_expand_spec_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"

using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string TEST_DB_DIR = "./sqlite";

std::shared_ptr<ParserCompactInfo> MakeStreamExpandSpecInfo(uint16_t status)
{
    auto info = std::make_shared<ParserCompactInfo>();
    info->data.streamExpandSpec.expandStatus = status;
    return info;
}
}

class StreamExpandSpecDBDumperUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        File::CreateDir(TEST_DB_DIR);
    }

    void TearDown() override
    {
        File::RemoveDir(TEST_DB_DIR, 0);
    }
};

TEST_F(StreamExpandSpecDBDumperUTest, GenerateDataShouldKeepInputOrder)
{
    StreamExpandSpecInfos infos = {MakeStreamExpandSpecInfo(1), MakeStreamExpandSpecInfo(0)};
    StreamExpandSpecDBDumper dumper(".");

    auto data = dumper.GenerateData(infos);

    ASSERT_EQ(data.size(), 2U);
    EXPECT_EQ(std::get<0>(data[0]), 1U);
    EXPECT_EQ(std::get<0>(data[1]), 0U);
}

TEST_F(StreamExpandSpecDBDumperUTest, DumpDataShouldReuseStreamExpandSpecDatabase)
{
    StreamExpandSpecInfos infos = {MakeStreamExpandSpecInfo(1)};
    StreamExpandSpecDBDumper dumper(".");

    ASSERT_TRUE(dumper.DumpData(infos));
    StreamExpandSpecDB database;
    DBRunner runner(File::PathJoin({".", "sqlite", database.GetDBName()}));
    StreamExpandSpecDumpData data;
    ASSERT_TRUE(runner.QueryData("SELECT expand_status FROM StreamExpandSpec", data));
    ASSERT_EQ(data.size(), 1U);
    EXPECT_EQ(std::get<0>(data[0]), 1U);
}
