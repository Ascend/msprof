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
#include "analysis/csrc/domain/data_process/system/biu_perf_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "reserve_mock_utils.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Domain::Environment;
using namespace Analysis::Test;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./biu_perf_processor_data";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_NAME = "biu_perf.db";
const std::string TABLE_NAME = "BiuInstrStatus";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string SQLITE_SUFFIX = "sqlite";

using OriBiuPerfFormat =
    std::vector<std::tuple<uint16_t, std::string, uint16_t, std::string, uint64_t, uint64_t, uint16_t>>;
using ProcessedFormat = std::vector<BiuPerfData>;

OriBiuPerfFormat DATA_A{{0, "AI_CORE", 1, "instruction_a", 3758215093862910, 450780, 0},
                        {0, "VECTOR", 2, "instruction_b", 3758215114581640, 250260, 1}};
}

class BiuPerfProcessorUTest : public testing::Test {
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
        CreateBiuPerfData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_NAME}), DATA_A);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"devCntvct", "36471130547330"},
            {"hostCntvct", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }
    static void CreateBiuPerfData(const std::string& dbPath, OriBiuPerfFormat data)
    {
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        dbRunner->CreateTable(TABLE_NAME, Analysis::Infra::BiuPerfDB().GetTableCols("BiuInstrStatus"));
        if (!data.empty()) {
            dbRunner->InsertData(TABLE_NAME, data);
        }
    }
};

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    auto res = dataInventory.GetPtr<ProcessedFormat>();
    EXPECT_EQ(DATA_A.size(), res->size());
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenGetProfTimeRecordInfoFailed)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnTrueWhenDbFileNotExist)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Utils::File::Exist).stubs().will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Utils::File::Exist).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenFileReaderCheckFailed)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Utils::FileReader::Check).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenSaveToDataInventoryFailed)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<BiuPerfData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<BiuPerfData>).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    StubReserveFailureForVector<ProcessedFormat>();
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    ResetReserveFailureForVector<ProcessedFormat>();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnTrueWhenNoDeviceFound)
{
    std::vector<std::string> emptyList;
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(emptyList));
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(BiuPerfProcessorUTest, TestRunShouldReturnFalseWhenLoadBiuPerfDataReturnsEmpty)
{
    auto processor = BiuPerfProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    if (File::Check(BASE_PATH)) {
        File::RemoveDir(BASE_PATH, DEPTH);
    }
    File::CreateDir(BASE_PATH);
    File::CreateDir(PROF_PATH_A);
    File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX}));
    File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX}));
    CreateBiuPerfData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_NAME}), OriBiuPerfFormat{});
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
}
