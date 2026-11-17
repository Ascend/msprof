/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#include "analysis/csrc/application/database/db_constant.h"
#include "analysis/csrc/domain/data_process/system/hbm_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Test;

namespace
{
const std::string HBM_DIR = "./hbm";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hbm.db";
const std::string PROF_DIR = File::PathJoin({HBM_DIR, "./PROF_0"});
const std::string TABLE_NAME = "HBMbwData";
// device_id, timestamp, bandwidth, hbmid, event_type
using HbmDataFormat = std::vector<std::tuple<uint32_t, double, double, uint32_t, std::string>>;
using SummaryData = std::vector<std::tuple<uint32_t, std::string, double>>;
const HbmDataFormat HBM_DATA = {
    {2, 71735309797040, 20.82875495833125, 0, "read"},   {2, 71735309797040, 20.40716971570149, 1, "read"},
    {2, 71735309797040, 20.12305792175534, 2, "read"},   {2, 71735309797040, 20.75849075122629, 3, "read"},
    {2, 71735309797040, 111.44514239951948, 0, "write"}, {2, 71735309797040, 113.74247647529906, 1, "write"},
    {2, 71735309797040, 112.54798495451473, 2, "write"}, {2, 71735309797040, 114.35041461503329, 3, "write"},
};
}  // namespace
class HBMProcessorUTest : public testing::Test
{
   protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(HBM_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateHBMDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    static bool CreateHBMDB(const std::string& dbPath)
    {
        std::shared_ptr<HBMDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, HBMDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, HBM_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HBM_DIR, 0));
        GlobalMockObject::verify();
    }
};

TEST_F(HBMProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(summary->size(), 5UL);
    EXPECT_TRUE(summary->front().isAverage);
    for (size_t i = 1; i < summary->size(); ++i)
    {
        EXPECT_FALSE(summary->at(i).isAverage);
    }
    EXPECT_GT(summary->front().readBandwidth, 0);
    EXPECT_GT(summary->front().writeBandwidth, 0);
}

TEST_F(HBMProcessorUTest, AverageUsesPerHbmIdMeansWhenSampleCountsDiffer)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_uneven_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    HBMDB database;
    DBRunner runner(File::PathJoin({sqliteDir, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{
        {2, 71735309797040, 100, 0, "read"},
        {2, 71735309797041, 200, 0, "read"},
        {2, 71735309797042, 600, 1, "read"},
    }));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(3UL, summary->size());
    EXPECT_TRUE(summary->at(0).isAverage);
    EXPECT_DOUBLE_EQ(375, summary->at(0).readBandwidth);
    EXPECT_DOUBLE_EQ(0, summary->at(0).writeBandwidth);
    EXPECT_DOUBLE_EQ(150, summary->at(1).readBandwidth);
    EXPECT_DOUBLE_EQ(600, summary->at(2).readBandwidth);
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, ReadOnlySampleDefaultsWriteBandwidthToZero)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_read_only_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    const auto dbPath = File::PathJoin({sqliteDir, DB_SUFFIX});
    HBMDB database;
    DBRunner runner(dbPath);
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{{2, 71735309797040, 88, 0, "read"}}));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(summary->size(), 2UL);
    for (const auto &row : *summary)
    {
        EXPECT_DOUBLE_EQ(88, row.readBandwidth);
        EXPECT_DOUBLE_EQ(0, row.writeBandwidth);
    }
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, WriteOnlySampleDefaultsReadBandwidthToZero)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_write_only_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    HBMDB database;
    DBRunner runner(File::PathJoin({sqliteDir, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{{2, 71735309797040, 12, 1, "write"}}));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(summary->size(), 2UL);
    for (const auto &row : *summary)
    {
        EXPECT_DOUBLE_EQ(row.readBandwidth, 0);
        EXPECT_DOUBLE_EQ(row.writeBandwidth, 12);
    }
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, SampledEventsAreTrackedPerHbmId)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_mixed_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    const auto dbPath = File::PathJoin({sqliteDir, DB_SUFFIX});
    HBMDB database;
    DBRunner runner(dbPath);
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{
        {2, 71735309797040, 0, 0, "read"},
        {2, 71735309797040, 12, 1, "write"},
        {2, 71735309797040, 7, 2, "unknown"},
    }));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(summary->size(), 3UL);
    EXPECT_DOUBLE_EQ(summary->at(0).readBandwidth, 0);
    EXPECT_DOUBLE_EQ(summary->at(0).writeBandwidth, 12);
    for (size_t i = 1; i < summary->size(); ++i)
    {
        const auto &row = summary->at(i);
        if (row.hbmId == 0)
        {
            EXPECT_DOUBLE_EQ(row.readBandwidth, 0);
            EXPECT_DOUBLE_EQ(row.writeBandwidth, 0);
        }
        else
        {
            EXPECT_EQ(row.hbmId, 1);
            EXPECT_DOUBLE_EQ(row.readBandwidth, 0);
            EXPECT_DOUBLE_EQ(row.writeBandwidth, 12);
        }
    }
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, OutOfRangeHbmIdDoesNotCollideWithValidSummaryOrDetail)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_hbm_id_range"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    HBMDB database;
    DBRunner runner(File::PathJoin({sqliteDir, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{
        // The fixture's collection start is 1701069323851824000 ns; keep these samples after it.
        {2, 1701069323852824000.0, 10, 0, "read"},
        {2, 1701069323853824000.0, 20, 255, "read"},
        {2, 1701069323854824000.0, 30, 256, "read"},
    }));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    ASSERT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    const auto detail = dataInventory.GetPtr<std::vector<HbmData>>();
    const auto summary = dataInventory.GetPtr<std::vector<HbmSummaryData>>();
    ASSERT_NE(detail, nullptr);
    ASSERT_NE(summary, nullptr);
    ASSERT_EQ(detail->size(), 2UL);
    bool hasZero = false;
    bool has255 = false;
    for (const auto &item : *detail)
    {
        hasZero |= item.hbmId == 0;
        has255 |= item.hbmId == 255;
    }
    EXPECT_TRUE(hasZero);
    EXPECT_TRUE(has255);
    ASSERT_EQ(summary->size(), 3UL);
    EXPECT_TRUE(summary->at(0).isAverage);
    EXPECT_DOUBLE_EQ(summary->at(0).readBandwidth, 15);
    EXPECT_EQ(summary->at(1).hbmId, 0);
    EXPECT_EQ(summary->at(2).hbmId, 255);
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, UnknownEventsDoNotProduceEmptySummaryRows)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_unknown_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    HBMDB database;
    DBRunner runner(File::PathJoin({sqliteDir, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable(TABLE_NAME, database.GetTableCols(TABLE_NAME)));
    ASSERT_TRUE(runner.InsertData(TABLE_NAME, HbmDataFormat{{2, 71735309797040, 7, 2, "unknown"}}));

    HBMProcessor processor(profile);
    DataInventory dataInventory;
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    EXPECT_EQ(dataInventory.GetPtr<std::vector<HbmSummaryData>>(), nullptr);
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, MissingFormalTableAfterFailedParseFails)
{
    const auto profile = File::PathJoin({HBM_DIR, "PROF_missing_hbm"});
    const auto device = File::PathJoin({profile, DEVICE_SUFFIX});
    const auto sqliteDir = File::PathJoin({device, SQLITE});
    ASSERT_TRUE(File::CreateDir(profile));
    ASSERT_TRUE(File::CreateDir(device));
    ASSERT_TRUE(File::CreateDir(sqliteDir));
    DBRunner runner(File::PathJoin({sqliteDir, DB_SUFFIX}));
    ASSERT_TRUE(runner.CreateTable("HBMbwDataStage_interrupted", {{"value", "INTEGER"}}));
    DataInventory dataInventory;
    HBMProcessor processor(profile);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    EXPECT_TRUE(File::RemoveDir(profile, 0));
}

TEST_F(HBMProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HbmData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<HbmData>).reset();

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(static_cast<uint16_t>(UINT16_MAX)));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}

TEST_F(HBMProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    // LoadData failed
    MOCKER_CPP(&OriHbmData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&OriHbmData::empty).reset();
    // Reserve failed
    StubReserveFailureForVector<std::vector<HbmData>>();
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    ResetReserveFailureForVector<std::vector<HbmData>>();
}

TEST_F(HBMProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessSummaryDataFailed)
{
    auto processor = HBMProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&SummaryData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HBM));
    MOCKER_CPP(&SummaryData::empty).reset();
}
