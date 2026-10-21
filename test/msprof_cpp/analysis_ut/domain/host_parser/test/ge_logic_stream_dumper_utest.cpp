/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of the Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include <string>
#include <vector>

#include "analysis/csrc/domain/services/persistence/host/ge_logic_stream_dumper.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

namespace
{
const std::string ROOT_DIR = "./ge_logic_stream_dumper_utest";
const std::string SQLITE_DIR = ROOT_DIR + "/sqlite";

std::string DbPath()
{
    GeLogicStreamDB db;
    return File::PathJoin({SQLITE_DIR, db.GetDBName()});
}
}  // namespace

class GeLogicStreamDumperUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        ASSERT_TRUE(File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override { ASSERT_TRUE(File::RemoveDir(ROOT_DIR, 0)); }
};

TEST_F(GeLogicStreamDumperUTest, DumpsRows)
{
    const GeLogicStreamRows rows{{100U, 99U}, {101U, 99U}, {102U, 99U}};
    ASSERT_TRUE(GeLogicStreamDumper(ROOT_DIR).DumpData(rows));
    DBRunner runner(DbPath());
    GeLogicStreamRows dbRows;
    ASSERT_TRUE(runner.QueryData("SELECT physic_stream, logic_stream FROM GeLogicStreamInfo", dbRows));
    EXPECT_EQ(rows, dbRows);
}

TEST_F(GeLogicStreamDumperUTest, EmptyRowsDoNotCreateDb)
{
    ASSERT_TRUE(GeLogicStreamDumper(ROOT_DIR).DumpData(GeLogicStreamRows{}));
    EXPECT_FALSE(File::Exist(DbPath()));
}
