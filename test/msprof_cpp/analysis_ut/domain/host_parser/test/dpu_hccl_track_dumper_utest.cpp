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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"
#include "analysis/csrc/domain/services/persistence/host/dpu_hccl_track_dumper.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/utils/common_constant.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "analysis/csrc/infrastructure/utils/parser_struct.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

using namespace Analysis;
using namespace Analysis::Domain;
using namespace Analysis::Infra;
using namespace Analysis::Utils;
using Analysis::Domain::Host::Cann::HashData;

namespace
{
const std::string ROOT_DIR = "./dpu_hccl_track_dumper_utest";
const std::string SQLITE_DIR = ROOT_DIR + "/sqlite";
constexpr uint16_t LEVEL_COMMUNICATION = 5500;

std::shared_ptr<ParserAdditionalInfo> MakeInfo()
{
    auto info = std::make_shared<ParserAdditionalInfo>();
    info->magicNumber = 0x5A5A;
    info->level = LEVEL_COMMUNICATION;
    info->type = 91;
    info->threadId = 270722;
    info->dataLen = 232;
    info->timeStamp = 75838889645892ULL;
    auto &payload = info->dpuHcclInfo;
    payload.itemId = 666666;
    payload.cclTag = 777777;
    payload.groupName = 123456;
    payload.localRank = 0;
    payload.remoteRank = 15;
    payload.rankSize = 16;
    payload.stage = 0;
    payload.notifyId = 111111;
    payload.startTime = 75838889643892ULL;
    payload.durationEstimated = 0;
    payload.srcAddr = 444555;
    payload.dstAddr = 777888;
    payload.dataSize = 1000;
    payload.taskId = 10;
    payload.aicpuTaskId = 5000;
    payload.streamId = 1;
    payload.planeId = 1;
    payload.npuDeviceId = 0;
    payload.filed24 = 0x1005;
    payload.opType = 0;
    payload.dataType = 0;
    payload.linkType = 0;
    payload.transportType = 0;
    payload.rdmaType = 0;
    payload.role = 0;
    payload.workFlowMode = 0;
    return info;
}

std::string DbPath()
{
    DPUDB dpuDb;
    return File::PathJoin({SQLITE_DIR, dpuDb.GetDBName()});
}

bool Dump(const std::vector<std::shared_ptr<ParserAdditionalInfo>> &records)
{
    DpuHcclTrackDumper dumper(ROOT_DIR);
    return dumper.DumpData(records);
}
}  // namespace

class DpuHcclTrackDumperUTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        ASSERT_TRUE(File::CreateDir(ROOT_DIR));
        ASSERT_TRUE(File::CreateDir(SQLITE_DIR));
    }

    void TearDown() override
    {
        Host::Cann::HashData::GetInstance().Clear();
        ASSERT_TRUE(File::RemoveDir(ROOT_DIR, 0));
    }
};

TEST_F(DpuHcclTrackDumperUTest, RecordKeepsMaskHashMappedNamesAndRawTimes)
{
    HashData::GetInstance().GetAll()[666666] = "hccl_dpu";
    HashData::GetInstance().GetAll()[123456] = "group_a";
    auto info = MakeInfo();
    info->dpuHcclInfo.startTime = 1234567890123ULL;
    info->timeStamp = 999ULL;
    ASSERT_TRUE(Dump({info}));
    DBRunner runner(DbPath());
    auto cols = runner.GetTableColumns("DPUHcclTrack");
    ASSERT_EQ(29U, cols.size());
    for (const auto &col : cols)
    {
        EXPECT_NE("is_dpu", col.name);
        EXPECT_NE("dev_type", col.name);
    }
    DpuHcclTrackRows dbRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM DPUHcclTrack", dbRows));
    ASSERT_EQ(1U, dbRows.size());
    const auto &row = dbRows[0];
    EXPECT_EQ(5U, std::get<1>(row));
    EXPECT_EQ(1234567890123ULL, std::get<3>(row));
    EXPECT_EQ(999ULL, std::get<4>(row));
    EXPECT_EQ("hccl_dpu", std::get<5>(row));
    EXPECT_EQ("group_a", std::get<6>(row));
    EXPECT_EQ("123456", std::get<7>(row));
    EXPECT_EQ("444555", std::get<12>(row));
    EXPECT_EQ("777888", std::get<13>(row));
    EXPECT_EQ("SUM", std::get<19>(row));
    EXPECT_EQ("INT8", std::get<20>(row));
    EXPECT_EQ("ON_CHIP", std::get<21>(row));
    EXPECT_EQ("SDMA", std::get<22>(row));
    EXPECT_EQ("RDMA_SEND_NOTIFY", std::get<23>(row));
    EXPECT_EQ("DST", std::get<24>(row));
}

TEST_F(DpuHcclTrackDumperUTest, HashMissAndUnknownEnumKeepOriginal)
{
    auto info = MakeInfo();
    info->dpuHcclInfo.opType = 99;
    info->dpuHcclInfo.dataType = 13;
    info->dpuHcclInfo.linkType = 42;
    info->dpuHcclInfo.transportType = 7;
    info->dpuHcclInfo.rdmaType = 3;
    info->dpuHcclInfo.role = 8;
    ASSERT_TRUE(Dump({info}));
    DBRunner runner(DbPath());
    DpuHcclTrackRows dbRows;
    ASSERT_TRUE(runner.QueryData("SELECT * FROM DPUHcclTrack", dbRows));
    ASSERT_EQ(1U, dbRows.size());
    const auto &row = dbRows[0];
    EXPECT_EQ("666666", std::get<5>(row));
    EXPECT_EQ(NA, std::get<6>(row));
    EXPECT_EQ("123456", std::get<7>(row));
    EXPECT_EQ("99", std::get<19>(row));
    EXPECT_EQ("13", std::get<20>(row));
    EXPECT_EQ("42", std::get<21>(row));
    EXPECT_EQ("7", std::get<22>(row));
    EXPECT_EQ("3", std::get<23>(row));
    EXPECT_EQ("8", std::get<24>(row));
}

TEST_F(DpuHcclTrackDumperUTest, DurationEstimatedColumnIsNumericAndStoresInteger)
{
    auto info = MakeInfo();
    info->dpuHcclInfo.durationEstimated = 40;
    ASSERT_TRUE(Dump({info}));
    DBRunner runner(DbPath());
    std::string declaredType;
    for (const auto &col : runner.GetTableColumns("DPUHcclTrack"))
    {
        if (col.name == "duration_estimated")
        {
            declaredType = col.type;
        }
    }
    EXPECT_EQ("NUMERIC", declaredType);
    std::vector<std::tuple<std::string, int64_t>> stored;
    ASSERT_TRUE(runner.QueryData("SELECT typeof(duration_estimated), duration_estimated FROM DPUHcclTrack", stored));
    ASSERT_EQ(1U, stored.size());
    EXPECT_EQ("integer", std::get<0>(stored[0]));
    EXPECT_EQ(40, std::get<1>(stored[0]));
}

TEST_F(DpuHcclTrackDumperUTest, EmptyRecordsDoNotCreateDb)
{
    ASSERT_TRUE(Dump(std::vector<std::shared_ptr<ParserAdditionalInfo>>{}));
    EXPECT_FALSE(File::Exist(DbPath()));
}
