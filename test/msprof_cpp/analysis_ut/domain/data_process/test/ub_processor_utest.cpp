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
#include "analysis/csrc/domain/data_process/system/ub_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/application/database/db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Domain::Environment;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ub_processor_data";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_NAME = "ub.db";
const std::string TABLE_NAME = "UBBwData";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string SQLITE_SUFFIX = "sqlite";

using OriUbFormat = std::vector<std::tuple<uint16_t, uint16_t, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
using ProcessedFormat = std::vector<UbData>;

OriUbFormat DATA_A{{0, 0, 3758215093862910, 100, 200, 1000, 2000},
                   {0, 1, 3758215114581640, 150, 250, 1500, 2500}};
}

class UbProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateUbData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_NAME}), DATA_A);
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
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }
    static void CreateUbData(const std::string& dbPath, OriUbFormat data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        dbRunner->CreateTable(TABLE_NAME, Analysis::Infra::UbDB().GetTableCols("UBBwData"));
        if (!data.empty()) {
            dbRunner->InsertData(TABLE_NAME, data);
        }
    }
};

TEST_F(UbProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    auto res = dataInventory.GetPtr<ProcessedFormat>();
    EXPECT_EQ(DATA_A.size(), res->size());
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenGetClockMonotonicRawFailed)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnTrueWhenDbFileNotExist)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Utils::File::Exist).stubs().will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&Utils::File::Exist).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenFileReaderCheckFailed)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&Utils::FileReader::Check).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<UbData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<UbData>).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    StubReserveFailureForVector<ProcessedFormat>();
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    ResetReserveFailureForVector<ProcessedFormat>();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnTrueWhenNoDeviceFound)
{
    std::vector<std::string> emptyList;
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(emptyList));
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(UbProcessorUTest, TestRunShouldReturnFalseWhenLoadUbDataReturnsEmpty)
{
    auto processor = UbProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    // Recreate DB with empty data so LoadUbData returns empty
    if (File::Check(BASE_PATH)) {
        File::RemoveDir(BASE_PATH, DEPTH);
    }
    File::CreateDir(BASE_PATH);
    File::CreateDir(PROF_PATH_A);
    File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX}));
    File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX}));
    CreateUbData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_NAME}), OriUbFormat{});
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_UB));
}
