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
#include "analysis/csrc/domain/services/persistence/host/dpu_task_track_db_dumper.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"

using namespace Analysis::Utils;
using namespace Analysis::Infra;
using namespace Analysis::Domain;

struct DpuTrackRow {
    uint32_t dpuDeviceId;
    uint32_t threadId;
    uint64_t startTime;
    uint64_t endTime;
    std::string taskType;
    uint32_t streamId;
    uint32_t taskId;
    std::string kernelName;
};

static DpuTrackRow ToRow(const DpuTaskTrackData::value_type &t)
{
    return {std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t),
            std::get<4>(t), std::get<5>(t), std::get<6>(t), std::get<7>(t)};
}

const std::string TEST_DB_DIR = "./sqlite";

class DpuTaskTrackDBDumperUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(TEST_DB_DIR);
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
        File::RemoveDir(TEST_DB_DIR, 0);
    }
};

std::shared_ptr<MsprofCompactInfo> MakeDpuTrackInfo(uint32_t deviceId, uint32_t threadId,
    uint64_t startTime, uint64_t endTime, uint32_t streamId, uint32_t taskId)
{
    auto info = std::make_shared<MsprofCompactInfo>();
    info->threadId = threadId;
    info->timeStamp = endTime;
    info->level = MSPROF_REPORT_NODE_LEVEL;
    info->type = static_cast<uint32_t>(EventType::EVENT_TYPE_TASK_TRACK);
    info->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    MsprofDpuTrack track;
    track.deviceId = static_cast<uint16_t>(deviceId);
    track.streamId = static_cast<uint16_t>(streamId);
    track.taskId = taskId;
    track.taskType = 1;
    track.startTime = startTime;
    info->data.dpuTrack = track;
    return info;
}

TEST_F(DpuTaskTrackDBDumperUtest, TestGenerateDataShouldReturnCorrectlyWhenKernelNameMapSet)
{
    DpuTaskTrackDBDumper dumper(".");
    std::unordered_map<uint64_t, uint64_t> kernelMap;
    const uint64_t kernelHash1 = 836640106292564866;
    const uint64_t kernelHash2 = 7419384796023234053;
    kernelMap[(static_cast<uint64_t>(0x1001) << 32) | 10] = kernelHash1;
    kernelMap[(static_cast<uint64_t>(0x1002) << 32) | 20] = kernelHash2;
    dumper.SetKernelNameMap(kernelMap);

    std::vector<std::shared_ptr<MsprofCompactInfo>> trackInfos;
    trackInfos.push_back(MakeDpuTrackInfo(0x1001, 1, 1000, 2000, 5, 10));
    trackInfos.push_back(MakeDpuTrackInfo(0x1002, 2, 3000, 4000, 6, 20));
    trackInfos.push_back(MakeDpuTrackInfo(0x1003, 3, 5000, 6000, 7, 30));

    auto result = dumper.GenerateData(trackInfos);
    ASSERT_EQ(result.size(), 3);

    DpuTrackRow row0 = ToRow(result[0]);
    EXPECT_EQ(row0.dpuDeviceId, 0x001);
    EXPECT_EQ(row0.threadId, 1);
    EXPECT_EQ(row0.startTime, 1000);
    EXPECT_EQ(row0.endTime, 2000);
    EXPECT_EQ(row0.taskType, "AI_CPU");
    EXPECT_EQ(row0.streamId, 5);
    EXPECT_EQ(row0.taskId, 10);
    EXPECT_EQ(row0.kernelName, std::to_string(kernelHash1));

    DpuTrackRow row1 = ToRow(result[1]);
    EXPECT_EQ(row1.dpuDeviceId, 0x002);
    EXPECT_EQ(row1.threadId, 2);
    EXPECT_EQ(row1.startTime, 3000);
    EXPECT_EQ(row1.endTime, 4000);
    EXPECT_EQ(row1.taskType, "AI_CPU");
    EXPECT_EQ(row1.streamId, 6);
    EXPECT_EQ(row1.taskId, 20);
    EXPECT_EQ(row1.kernelName, std::to_string(kernelHash2));

    DpuTrackRow row2 = ToRow(result[2]);
    EXPECT_EQ(row2.dpuDeviceId, 0x003);
    EXPECT_EQ(row2.threadId, 3);
    EXPECT_EQ(row2.startTime, 5000);
    EXPECT_EQ(row2.endTime, 6000);
    EXPECT_EQ(row2.taskType, "AI_CPU");
    EXPECT_EQ(row2.streamId, 7);
    EXPECT_EQ(row2.taskId, 30);
    EXPECT_EQ(row2.kernelName, "N/A");
}

TEST_F(DpuTaskTrackDBDumperUtest, TestGenerateDataShouldReturnEmptyWhenInputEmpty)
{
    DpuTaskTrackDBDumper dumper(".");
    std::vector<std::shared_ptr<MsprofCompactInfo>> emptyInfos;
    auto result = dumper.GenerateData(emptyInfos);
    EXPECT_TRUE(result.empty());
}

TEST_F(DpuTaskTrackDBDumperUtest, TestDumpDataShouldReturnTrueWhenDataInsertSuccess)
{
    DpuTaskTrackDBDumper dumper(".");
    std::vector<std::shared_ptr<MsprofCompactInfo>> trackInfos;
    trackInfos.push_back(MakeDpuTrackInfo(0x1001, 1, 1000, 2000, 5, 10));

    auto res = dumper.DumpData(trackInfos);
    EXPECT_TRUE(res);

    DPUDB dpuDb;
    std::string dbPath = Utils::File::PathJoin({".", "sqlite", dpuDb.GetDBName()});
    DBRunner dbRunner(dbPath);
    std::vector<std::tuple<std::string, std::string>> queryResult;
    dbRunner.QueryData("select * from DPUTaskTrack", queryResult);
    EXPECT_EQ(queryResult.size(), 1);
}

TEST_F(DpuTaskTrackDBDumperUtest, TestDumpDataShouldReturnFalseWhenDBNotCreated)
{
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    DpuTaskTrackDBDumper dumper(".");
    std::vector<std::shared_ptr<MsprofCompactInfo>> trackInfos;
    trackInfos.push_back(MakeDpuTrackInfo(0x1001, 1, 1000, 2000, 5, 10));

    ASSERT_FALSE(dumper.DumpData(trackInfos));
}