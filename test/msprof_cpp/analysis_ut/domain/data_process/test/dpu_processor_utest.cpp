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
#include "analysis/csrc/domain/data_process/ai_task/dpu_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Test;

const std::string DPU_DIR = "./dpu_data";
const std::string PROF0 = File::PathJoin({DPU_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({DPU_DIR, "PROF_1"});
const std::string PROF2 = File::PathJoin({DPU_DIR, "PROF_2"});
const std::string PROF3 = File::PathJoin({DPU_DIR, "PROF_3"});
const std::string TRACK_TABLE_NAME = "DPUTaskTrack";
const std::string HCCL_TRACK_TABLE_NAME = "DPUHcclTrack";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1, PROF2, PROF3};

// dpu_device_id, thread_id, start_time, end_time, task_type, stream_id, task_id, kernel_name(op_name)
using Track = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, uint64_t, std::string,
        uint16_t, uint32_t, std::string>>;
// npu_device_id, dpu_device_id, thread_id, start_time, end_time, op_name, group_name, group_name_id
// local_rank, remote_rank, rank_size, duration_estimated, src_addr, dst_addr, data_size
// stream_id, task_id, aicpu_task_id, plane_id, op_type, data_type, link_type, transport_type
// rdma_type, role, ccl_tag, notify_id, work_flow_mode, stage
using HcclTrack = std::vector<std::tuple<uint16_t, uint16_t, uint32_t, uint64_t, uint64_t,
        std::string, std::string, std::string, uint16_t, uint16_t, uint32_t, double, std::string, std::string, uint64_t,
        uint16_t, uint32_t, uint32_t, uint16_t, std::string, std::string, std::string, std::string,
        std::string, std::string, std::string, std::string, std::string, std::string>>;

const Track TRACK_DATA = {
        {0, 116, 65177262396323, 65177262397323, "KERNEL_AICORE", 1, 100, "Conv2D"},
        {0, 117, 65177262398000, 65177262399500, "KERNEL_AICORE", 2, 101, "MatMul"},
        {1, 118, 65177262400000, 65177262401200, "KERNEL_AICORE", 3, 102, "Relu"},
};

const HcclTrack HCCL_TRACK_DATA = {
        {0, 0, 116, 65177264896891, 65177264928192, "Dpu_Inline_Write", "hccl_group", "123456789", 0, 2, 4, 0.0, "1111", "2222",
         1024, 1, 200, 300, 0, "MAX", "FP32", "ROCE", "UB", "RESERVED", "DST", "123456", "0", "1", "0"},
        {0, 1, 117, 65177264940493, 65177264975485, "Dpu_Notify_Wait", "hccl_group", "123456789", 0, 2, 4, 0.0,
         "1111", "2222", 2048, 1, 201, 300, 0, "MAX", "FP32", "ON_CHIP", "UB", "RESERVED", "DST", "123456", "0", "1", "0"},
};

class DPUProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        if (File::Check(DPU_DIR)) {
            File::RemoveDir(DPU_DIR, 0);
        }
        EXPECT_TRUE(File::CreateDir(DPU_DIR));

        for (const auto& profPath : PROF_PATHS) {
            EXPECT_TRUE(File::CreateDir(profPath));
            EXPECT_TRUE(File::CreateDir(File::PathJoin({profPath, HOST})));
            if (profPath == PROF3) {
                EXPECT_TRUE(File::CreateDir(File::PathJoin({profPath, HOST, SQLITE})));
            } else {
                EXPECT_TRUE(CreateDPUDb(File::PathJoin({profPath, HOST, SQLITE})));
            }
        }
    }

    static bool CreateDPUDb(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<DPUDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, DPUDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable(TRACK_TABLE_NAME, database->GetTableCols(TRACK_TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TRACK_TABLE_NAME, TRACK_DATA));

        EXPECT_TRUE(dbRunner->CreateTable(HCCL_TRACK_TABLE_NAME, database->GetTableCols(HCCL_TRACK_TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(HCCL_TRACK_TABLE_NAME, HCCL_TRACK_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(DPU_DIR, 0));
        GlobalMockObject::verify();
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

void CheckDPUDataValid(const std::vector<DPUData> &checkData)
{
    size_t trackIndex = 0;
    size_t hcclIndex = 0;

    for (const auto &data : checkData) {
        if (!data.isHccl) {
            EXPECT_EQ(data.dpuDeviceId, std::get<0>(TRACK_DATA[trackIndex]));
            EXPECT_EQ(data.threadId, std::get<1>(TRACK_DATA[trackIndex]));
            EXPECT_EQ(data.taskType, std::get<4>(TRACK_DATA[trackIndex]));
            EXPECT_EQ(data.streamId, std::get<5>(TRACK_DATA[trackIndex]));
            EXPECT_EQ(data.taskId, std::get<6>(TRACK_DATA[trackIndex]));
            EXPECT_EQ(data.opName, std::get<7>(TRACK_DATA[trackIndex]));
            ++trackIndex;
        } else {
            EXPECT_EQ(data.npuDeviceId, std::get<0>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.dpuDeviceId, std::get<1>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.threadId, std::get<2>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.opName, std::get<5>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.groupName, std::get<6>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.groupNameId, std::get<7>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.localRank, std::get<8>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.remoteRank, std::get<9>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.rankSize, std::get<10>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.dataSize, std::get<14>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.streamId, std::get<15>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.taskId, std::get<16>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.aicpuTaskId, std::get<17>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.planeId, std::get<18>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.opType, std::get<19>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.dataType, std::get<20>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.linkType, std::get<21>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.transportType, std::get<22>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.rdmaType, std::get<23>(HCCL_TRACK_DATA[hcclIndex]));
            EXPECT_EQ(data.notifyId, std::get<26>(HCCL_TRACK_DATA[hcclIndex]));
            ++hcclIndex;
        }
    }

    EXPECT_EQ(trackIndex, TRACK_DATA.size());
    EXPECT_EQ(hcclIndex, HCCL_TRACK_DATA.size());
}

TEST_F(DPUProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::vector<DataInventory> res(PROF_PATHS.size());
    size_t i = 0;
    for (const auto& profPath : PROF_PATHS) {
        auto processor = DPUProcessor(profPath);
        EXPECT_TRUE(processor.Run(res[i], PROCESSOR_NAME_DPU));
        ++i;
    }

    // 最后一份PROF3未生成db数据
    for (size_t i = 0; i < res.size() - 1; ++i) {
        auto& node = res[i];
        auto checkData = node.GetPtr<std::vector<DPUData>>();
        ASSERT_NE(checkData, nullptr);
        EXPECT_EQ(TRACK_DATA.size() + HCCL_TRACK_DATA.size(), checkData->size());
        CheckDPUDataValid(*checkData);
        node.RemoveRestData({});
    }
}

TEST_F(DPUProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = DPUProcessor(PROF0);
    DataInventory dataInventory;

    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DPU));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DPU));
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DPUData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DPU));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DPUData>).reset();
}

TEST_F(DPUProcessorUTest, TestRunShouldReturnFalseWhenReserveFail)
{
    auto processor = DPUProcessor(PROF0);
    DataInventory dataInventory;
    StubReserveFailureForVector<std::vector<DPUData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DPU));
    ResetReserveFailureForVector<std::vector<DPUData>>();
}

TEST_F(DPUProcessorUTest, TestLoadTrackDataShouldReturnEmptyWhenDBNotExist)
{
    auto processor = DPUProcessor(PROF3);
    DPUProcessor::TrackFormat trackData = processor.LoadTrackData();
    EXPECT_EQ(trackData.size(), 0);
}

TEST_F(DPUProcessorUTest, TestLoadHcclTrackDataShouldReturnEmptyWhenDBNotExist)
{
    auto processor = DPUProcessor(PROF3);
    DPUProcessor::HcclTrackFormat hcclTrackData = processor.LoadHcclTrackData();
    EXPECT_EQ(hcclTrackData.size(), 0);
}

TEST_F(DPUProcessorUTest, TestLoadTrackDataShouldGetTaskSize)
{
    auto processor = DPUProcessor(PROF0);
    DPUProcessor::TrackFormat trackData = processor.LoadTrackData();
    EXPECT_EQ(trackData.size(), TRACK_DATA.size());
}

TEST_F(DPUProcessorUTest, TestLoadHcclTrackDataShouldGetHcclSize)
{
    auto processor = DPUProcessor(PROF0);
    DPUProcessor::HcclTrackFormat hcclTrackData = processor.LoadHcclTrackData();
    EXPECT_EQ(hcclTrackData.size(), HCCL_TRACK_DATA.size());
}

TEST_F(DPUProcessorUTest, TestProcessWithEmptyData)
{
    auto processor = DPUProcessor(PROF3);
    DataInventory dataInventory;

    MOCKER_CPP(&DPUProcessor::LoadTrackData).stubs().will(returnValue(DPUProcessor::TrackFormat{}));
    MOCKER_CPP(&DPUProcessor::LoadHcclTrackData).stubs().will(returnValue(DPUProcessor::HcclTrackFormat{}));

    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_DPU));

    MOCKER_CPP(&DPUProcessor::LoadTrackData).reset();
    MOCKER_CPP(&DPUProcessor::LoadHcclTrackData).reset();
}
